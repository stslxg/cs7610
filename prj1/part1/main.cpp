#include <unistd.h>
#include <vector>
#include <utility>
#include <tuple>
#include <map>
#include <set>
#include <queue>
#include <thread>
#include <chrono>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <fstream>
#include <netdb.h>
#include <netinet/in.h>
#include <getopt.h>
#include <mutex>

using namespace std;

#include "common.h"
#include "main.h"

/* parameters read from command line */
vector<string> host_names;
int delay = 0;
double drop_rate = 0.0;
static int message_count = 0;
static bool verbose = false;
static int timeout = MIN_TIME_OUT;
static int try_limit = TRY_LIMIT;

static int status = 0;
int my_id = ID_UNKNOWN;
static uint32_t message_id = 0, seq = 0;

/* mssages_acked[msg_id][receiver] is true iff received AckMessage for my message msg_id from receiver
   seq_message_acked[msg_id][receiver] is true iff received AckSeqMessage for my message msg_id from receiver */
static vector<vector<bool>> messages_acked, seq_messages_acked;

/* current final_seq and final_seq_proposer for each messsage msg_id I sent */
static vector<uint32_t> final_seqs;
static vector<uint32_t> final_seqs_proposer;

/* my proposed seq for each (sender, msg_id), used for duplicate DataMessage */
static map<message_key, uint32_t> proposed_seqs;

/* hold back priority queue, implemented in heap.cpp */
static min_heap hold_back;

/* used for duplicate SeqMessage */
static set<message_key> seq_messages_recved;

/* send queues to each peer and corresponding mutexes */
static vector<queue<string>> send_queues;
static mutex send_mtxes[MAX_HOST];

/* initialize all the arrays to suitable lengths */
void init() {
  messages_acked.resize(message_count);
  for (int i = 0; i < message_count; ++i)
    messages_acked[i].resize(host_names.size(), false);
  final_seqs.resize(message_count, 0);
  final_seqs_proposer.resize(message_count, ID_UNKNOWN);
  seq_messages_acked.resize(message_count);
  for (int i = 0; i < message_count; ++i)
    seq_messages_acked[i].resize(host_names.size(), false);
  send_queues.resize(host_names.size());
  init_random();
}

/* parse command line arguments */
void parse_command(int argc, char **argv) {
  string usage = "-h <hostfile> -c <count> [-d <drop rate> -t <delay> -o <timeout> -l <try limit> -v]";
  
  /* require -h <hostfile> -c <count> */
  if (argc < 5) {
    cerr << "Usage: " << argv[0] << usage << endl;
    exit(EXIT_FAILURE);
  }

  char opt;
  while ((opt = getopt(argc, argv, "h:c:d:t:o:l:v")) != -1) {
    if (opt == 'h') {  // -h <hostfile>
      string file_name(optarg), host_name;
      ifstream host_file(file_name);

      if (host_file.fail()) {
	cerr << "Failed to open hostfile" << endl;
	exit(EXIT_FAILURE);
      }
      
      while (host_file >> host_name)
	host_names.push_back(host_name);

      if (host_names.size() > MAX_HOST) {
	cerr << "Sorry, can't have more than " << MAX_HOST << " hosts " << endl;
	exit(EXIT_FAILURE);
      }
    } else if (opt == ':' or opt == '?') { // failure
      cerr << "Usage: " << argv[0] << usage << endl;
      exit(EXIT_FAILURE);
    } else if (opt == 'v') {  // -v for verbose
      verbose = true;
    } else {
      istringstream ss(optarg);
      switch (opt) {
      case 'c':  // -c <count>
	ss >> message_count;
	break;
      case 'd':  // -d <drop rate>
	ss >> drop_rate;
	if (drop_rate > 1) {
	  cerr << "Drop rate cannot be larger than 1" << endl;
	  exit(EXIT_FAILURE);
	}
	break;
      case 't':  // -t <delay>
	ss >> delay;
	timeout = max(MIN_TIME_OUT, 2 * delay);
	break;
      case 'o':  // -o <timeout>
	ss >> timeout;
	break;
      case 'l':  // -l <try limit>
	ss >> try_limit;
	break;
      }
    }
  }
}

/* add message to the right queue; may have multiple writer at the same time so need mutex */
void add_send_message(unsigned char *buf, size_t send_len, int id) {
  lock_guard<mutex> lock(send_mtxes[id]);
  send_queues[id].push(string((char *)buf, send_len));
}

/* one of the main parts: parse message and act accordingly */
void parse_message(unsigned char *buf) {
  int message_type = unpack_type(buf);

  /* in the up status, drop all other messages, only deal with up message */
  if (status == STATUS_UP) {
    if (message_type == UP_MSG_TYPE) {
      UpMessage s = unpack_up(buf);
      /* get my id from up message */
      if (my_id == ID_UNKNOWN) {
	my_id = s.receiver;
	if (verbose)
	  cout << "I'm " << host_names[my_id] << endl;
      }
    }
    return;
  }
  
  switch (message_type) {
    /* now we can ignore up message */
  case UP_MSG_TYPE:
    break;

    /* deal with data message */
  case DATA_MSG_TYPE: {
    DataMessage s = unpack_data(buf);
    message_key key = make_pair(s.sender, s.msg_id);
    unsigned char send_buf[BUF_SIZE];

    /* check if seen before */
    if (proposed_seqs.find(key) == proposed_seqs.end()) {
      /* record proposed seq and put in hold back queue */
      seq++;
      proposed_seqs[key] = seq;
      hold_back.push(s.sender, s.msg_id, seq, (uint32_t) my_id, false);

      if (verbose)
	cout << "Received data message " << s.msg_id << " from " << host_names[s.sender] << " , propose seq " << seq << endl;
    }

    /* send ack message */
    int send_len = pack_ack(send_buf, s.sender, s.msg_id, proposed_seqs[key], my_id);
    add_send_message(send_buf, send_len, s.sender);
    break;
  }

    /* deal with ack message */
  case ACK_MSG_TYPE: {
    AckMessage s = unpack_ack(buf);
    /* check if seen before */
    if (s.sender == my_id and (not messages_acked[s.msg_id][s.proposer])) {
      messages_acked[s.msg_id][s.proposer] = true;
      if (verbose)
	cout << "Received ack from " << host_names[s.proposer] << " for message " << s.msg_id << " with proposed seq " << s.proposed_seq << endl;
      
      /* update final_seq and final_seq_proposer for my message msg_id
	 Note here our rule guarentee that (final_seq, final_seq_proposer) as a pair will only increase */
      if (s.proposed_seq > final_seqs[s.msg_id] or (s.proposed_seq == final_seqs[s.msg_id] and s.proposer > final_seqs_proposer[s.msg_id])) {
	  final_seqs[s.msg_id] = s.proposed_seq;
	  final_seqs_proposer[s.msg_id] = s.proposer;
      }
    }
    break;
  }

    /* deal with seq message */
  case SEQ_MSG_TYPE: {
    SeqMessage s = unpack_seq(buf);
    message_key key = make_pair(s.sender, s.msg_id);

    /* check if seen before */
    if (seq_messages_recved.find(key) == seq_messages_recved.end()) {
      if (verbose)
	cout << "Received seq for message " << s.msg_id << " from " << host_names[s.sender] << " with seq ( " << s.final_seq << " , " << host_names[s.final_seq_proposer] << " ) " << endl;
      
      /* update seq and set the message as deliverable */
      seq = max(seq, s.final_seq);
      seq_messages_recved.insert(key);
      hold_back.increase_key(s.sender, s.msg_id, s.final_seq, s.final_seq_proposer, true);

      /* check for deliverable messages, deliver if possible */
      while (not hold_back.empty()) {
	message_info s = hold_back.top();
	if (get<4>(s)) {
	  cout << host_names[my_id] << ": Processed message " << get<1>(s) << " from sender " << host_names[get<0>(s)] << " with seq ( " << get<2>(s)  << " , " << host_names[get<3>(s)] << " )" << endl;
	  hold_back.pop();
	} else {
	  break;
	}
      }
    }

    /* send ack seq message */
    unsigned char send_buf[BUF_SIZE];
    int send_len = pack_ack_seq(send_buf, s.sender, s.msg_id, my_id);
    add_send_message(send_buf, send_len, s.sender);
    break;
  }

    /* received ack seq message */
  case ACK_SEQ_MSG_TYPE: {
    AckSeqMessage s = unpack_ack_seq(buf);
    if (s.sender == my_id and (not seq_messages_acked[s.msg_id][s.receiver])) {
      seq_messages_acked[s.msg_id][s.receiver] = true;
      if (verbose)
	cout << "Received ack seq for message " << s.msg_id << " from " << host_names[s.receiver] << endl;
    }
    break;
  }
    
    /* cannot decode message type */
  default:
    cerr << "Failed to unpack message type" << endl;
    exit(EXIT_FAILURE);
  }
}

/* main function for the reciever thread */
void receiver() {
  unsigned char buf[BUF_SIZE];
  while (true) {
    if (receive_message(buf) > 0)
      parse_message(buf);
  }
}

/* main function for each sender thread */
void sender(int id) {
  while (true) {
    if (not send_queues[id].empty()) {
      string s = send_queues[id].front();
      send_queues[id].pop();
      send_message((const unsigned char*)s.c_str(), s.size(), id);
    }
  }
}

/* check if all the containers are up */
void check_up() {
  bool all_is_up = true;
  unsigned char buf[BUF_SIZE];
  
  for (int id = 0; id < host_names.size(); ++id) {
    /* check if that host name is up; if so, send them their id (index in <hostfile>) */
    if (get_addr_info(id) == 0) {
      int send_len = pack_up(buf, id);
      add_send_message(buf, send_len, id);
    } else {
      all_is_up = false;
    }
  }

  /* everyone is up, go to next status */
  if (all_is_up and my_id != ID_UNKNOWN) {
    if (verbose)
      cout << "All containers are up!" << endl;
    status = STATUS_MULTICAST;
  }
}

/* main function for each message thread */
void message_manager(int message_id) {
  unsigned char buf[BUF_SIZE];
  bool recv_all_ack = false, recv_all_ack_seq = false;
  
  for (int i = 0; i < try_limit; ++i) {
    /* multicast DataMessage */
    for (int id = 0; id < host_names.size(); ++id)
      if (not messages_acked[message_id][id]) {
	int send_len = pack_data(buf, my_id, message_id);
	add_send_message(buf, send_len, id);
      }
    this_thread::sleep_for(chrono::milliseconds(timeout));
    
    /* check for receipts of AckMessage */
    if (recv_all_ack = all_of(messages_acked[message_id].begin(), messages_acked[message_id].end(), [](bool b){return b;}))
      break;
  }
  /* reach try limit, do not send data message again, just wait for ack */
  while (not recv_all_ack) {
    this_thread::sleep_for(chrono::milliseconds(timeout));
    recv_all_ack = all_of(messages_acked[message_id].begin(), messages_acked[message_id].end(), [](bool b){return b;});
  }
  if (verbose)
    cout << "Received all ack for message " << message_id << endl;

  
  for (int i = 0; i < try_limit; ++i) {
    /* send SeqMessage */
    for (int id = 0; id < host_names.size(); ++id)
      if (not seq_messages_acked[message_id][id]) {
	int send_len = pack_seq(buf, my_id, message_id, final_seqs[message_id], final_seqs_proposer[message_id]);
	add_send_message(buf, send_len, id);
      }
    this_thread::sleep_for(chrono::milliseconds(timeout));

    /* check for receipts of AckSeqMessage */
    if (recv_all_ack_seq = all_of(seq_messages_acked[message_id].begin(), seq_messages_acked[message_id].end(), [](bool b){return b;}))
      break;
  }
  /* reach try limit, do not send seq message again, just wait for ack seq */
  while (not recv_all_ack_seq) {
    this_thread::sleep_for(chrono::milliseconds(timeout));
    recv_all_ack_seq = all_of(seq_messages_acked[message_id].begin(), seq_messages_acked[message_id].end(), [](bool b){return b;});
  }
  if (verbose)
    cout << "Finished message " << message_id << endl;
}

int main(int argc, char **argv) {
  parse_command(argc, argv);
  init_socket();
  init();

  /* create receiver thread, and sender threads to each peer */
  thread r(receiver);
  r.detach();
  for (int id = 0; id < host_names.size(); ++id) {
    thread s(sender, id);
    s.detach();
  }

  /* get my id and check that every container is up */
  while (status == STATUS_UP) {        
    check_up();
    this_thread::sleep_for(chrono::seconds(1));
  }

  /* create threads for each message to multicast */
  for (int message_id = 0; message_id < message_count; ++message_id) {
    thread msg_mngr(message_manager, message_id);
    msg_mngr.detach();
    this_thread::sleep_for(chrono::milliseconds(100));
  }
  
  for(;;){};    

  /* will never run to this step, but I feel I still need to add some "closure" ;) */
  if (verbose)
    cout << "Finished up" << endl;
  close_socket();
  return 0;
}

