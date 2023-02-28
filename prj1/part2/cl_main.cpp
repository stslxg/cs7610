#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>
#include <queue>
#include <algorithm>
#include <utility>
#include <tuple>
#include <mutex>

using namespace std;

#include "common.h"
#include "cl_main.h"

extern bool im_initiator;
extern int initiator;
extern int my_id;
extern vector<string> host_names;
extern bool verbose;
extern uint32_t seq;
extern min_heap hold_back;

int snapshot_status = STATUS_WAIT;
vector<bool> recording;
vector<vector<string>> records;
vector<queue<string>> send_queues_tcp;
static vector<bool> up_tcp;
static uint32_t saved_seq;
static vector<message_info> saved_hold_back;
static vector<bool> receiving_internal, finish_receive;
static vector<ostringstream> snapshots;
static mutex parse_mtx;

void add_send_message_tcp(unsigned char *buf, size_t send_len, int id) {
  send_queues_tcp[id].push(string((char *)buf, send_len));
}
void add_send_message_tcp(string s, int id) {
  send_queues_tcp[id].push(s);
}

void add_record(unsigned char *buf, size_t send_len, int id) {
  if (recording[id])
    records[id].push_back(string((char *)buf, send_len));
}

void save_internal() {
  cout << "Saving internal states" << endl;
  saved_seq = seq;
  saved_hold_back = hold_back.save();
}

size_t print_internal(unsigned char *buf, int id) {
  int type = unpack_type(buf);
  switch (type) {
  case DATA_MSG_TYPE: {
    /* received seq */
    DataMessage s = unpack_data(buf);
    snapshots[id]  << "Sequence number " << s.data << "\n"; 
    return DATA_MSG_SIZE * sizeof(uint32_t);
  }
  case ACK_MSG_TYPE: {
    /* received undeliverable */
    AckMessage s = unpack_ack(buf);
    snapshots[id] << "Message sender " << host_names[s.sender] << " msg_id " << s.msg_id << " proposed_seq " << s.proposed_seq << " proposer " << host_names[s.proposer] << " Undeliverable\n";
    return ACK_MSG_SIZE * sizeof(uint32_t);
  }
  case SEQ_MSG_TYPE: {
    /* received deliverable */
    SeqMessage s = unpack_seq(buf);
    snapshots[id] << "Message sender " << host_names[s.sender] << " msg_id " << s.msg_id << " final_seq " << s.final_seq << " proposer " << host_names[s.final_seq_proposer] << " Deliverable\n";
    return SEQ_MSG_SIZE * sizeof(uint32_t);
  }
  }
}

size_t print_channel(unsigned char *buf, int id) {
  int type = unpack_type(buf);
  switch (type) {
  case DATA_MSG_TYPE: {
    DataMessage s = unpack_data(buf);
    snapshots[id]  << "DataMessage sender " << host_names[s.sender] << " msg_id " << s.msg_id << "\n";
    return DATA_MSG_SIZE * sizeof(uint32_t);
  }
  case ACK_MSG_TYPE: {
      AckMessage s = unpack_ack(buf);
      snapshots[id] << "AckMessage for sender " << host_names[s.sender] << " msg_id " << s.msg_id << " seq " << s.proposed_seq << " proposer " << host_names[s.proposer] << "\n";
     return ACK_MSG_SIZE * sizeof(uint32_t);
    }
  case SEQ_MSG_TYPE: {
    SeqMessage s = unpack_seq(buf);
    snapshots[id] << "SeqMessage for sender " << host_names[s.sender] << " msg_id " << s.msg_id << " final_seq " << s.final_seq << " proposer " << host_names[s.final_seq_proposer] << "\n";
    return SEQ_MSG_SIZE * sizeof(uint32_t);
  }
  case ACK_SEQ_MSG_TYPE: {
    AckSeqMessage s = unpack_ack_seq(buf);
    snapshots[id] << "AckSeqMessage for sender " << host_names[s.sender] << " msg_id " << s.msg_id << " from receiver " << host_names[s.receiver] << "\n";
    return ACK_SEQ_MSG_SIZE * sizeof(uint32_t);
  }
  }
}

size_t print_snapshot(unsigned char *buf, int id) {
  SnapshotMessage s = unpack_snapshot(buf);
  if (s.channel == MAX_HOST) {
    /* finish receiving snapshot */
    finish_receive[id] = true;
  } else {
    /* receving channel snapshot, instead of internal states */
    receiving_internal[id] = false;
    
    /* print the line to snapshot of id */
    snapshots[id] << "Channel from " << host_names[s.channel] << " to " << host_names[id] << " : \n";
  }
  return SNAPSHOT_MSG_SIZE * sizeof(uint32_t);
}

size_t parse_message_tcp(unsigned char *buf, int id) {
  lock_guard<mutex> lock(parse_mtx);
  int type = unpack_type(buf);
  if (type == MARKER_MSG_TYPE) {
    MarkerMessage s = unpack_marker(buf);
    
    if (not im_initiator and snapshot_status == STATUS_WAIT) {
      /* I'm not initiator and was waiting for the first marker, starts recording */
      cout << "Received marker from " << host_names[s.sender] << endl;
      snapshot_status = STATUS_RECORD;
      initiator = s.initiator;
      
      /* save current state */
      save_internal();
      
      /* turn on recording on other incoming channels, send markers */
      unsigned char send_buf[BUF_SIZE];
      for (int id = 0; id < host_names.size(); ++id)
	if (id != my_id) {
	  if (id != s.sender)
	    recording[id] = true;
	  int size = pack_marker(send_buf, initiator, my_id);
	  add_send_message_tcp(send_buf, size, id);
	}
    } else if (snapshot_status == STATUS_RECORD and recording[s.sender]) {
      /* already started recoding, now turn off recording of this channel */
      cout << "Received marker and stop recording from " << host_names[s.sender] << endl;
      recording[s.sender] = false;
      if (all_of(recording.begin(), recording.end(), [](bool b){return not b;})) {
	if (verbose)
	  cout << "Finished recording" << endl;
	snapshot_status = STATUS_SEND;
      }      
    }
    return MARKER_MSG_SIZE * sizeof(uint32_t);
  } else if (type == SNAPSHOT_MSG_TYPE) {
    return print_snapshot(buf, id);
  } else if (receiving_internal[id]) {
    /* receving internal state */
    return print_internal(buf, id);
  } else {
    /* receiving channel messages */
    return print_channel(buf, id);
  }
}

void receiver_tcp(int id) {
  unsigned char buf[BUF_SIZE];
  size_t size;
  while (true) {
    if ((size = receive_message_tcp(buf, id)) > 0) {
      size_t left = size;
      unsigned char *now = buf;
      while (left > 0) {
	size_t done = parse_message_tcp(now, id);
	left -= done;
	now += done;
      }
    }
  }
}

void sender_tcp(int id) {
  while (true) {
    if (not send_queues_tcp[id].empty()) {
      string s = send_queues_tcp[id].front();
      send_queues_tcp[id].pop();
      while (send_message_tcp((const unsigned char*)s.c_str(), s.size(), id) == -1) {};
    }
  }
}

void init_sender_receiver(int id) {
  up_tcp[id] = true;
  if (verbose)
    cout << "Connected with " << host_names[id] << endl;
  thread r(receiver_tcp, id);
  r.detach();
  thread s(sender_tcp, id);
  s.detach();
}

void connector(int id) {
  connect_to(id);
  init_sender_receiver(id);
}

void snapshot_manager() {
  while (true) {
    switch (snapshot_status) {
    case STATUS_RECORD: 
    /* check if finish recording incoming channels */
      break;
    case STATUS_SEND: {
      /* send my snapshot to the initiator */
      unsigned char buf[BUF_SIZE];

      /* use DataMessage to send saved_seq */
      int size = pack_data(buf, my_id, 0, saved_seq);
      if (im_initiator)
	print_internal(buf, my_id);
      else
	add_send_message_tcp(buf, size, initiator);

      /* sort the heap into a sorted list */
      sort(saved_hold_back.begin(), saved_hold_back.end(), [](message_info s1, message_info s2){
	  return (get<2>(s1) < get<2>(s2)  /* seq of s1 < seq of s2 */
		  or (get<2>(s1) == get<2>(s2) and get<3>(s1) < get<3>(s2)));
	});
      
      /* send the heap using AckMessage and SeqMessage */
      for (int i = 0; i < saved_hold_back.size(); ++i) {
	message_info s = saved_hold_back[i];
	/* if deliverable, use SeqMessage */
	if (get<4>(s)) {
	  size = pack_seq(buf, get<0>(s), get<1>(s), get<2>(s), get<3>(s));
	  if (im_initiator)
	    print_internal(buf, my_id);
	  else
	    add_send_message_tcp(buf, size, initiator);
	} else {
	  /* non-deliverable, use AckMessage */
	  size = pack_ack(buf, get<0>(s), get<1>(s), get<2>(s), get<3>(s));
	  if (im_initiator)
	    print_internal(buf, my_id);
	  else
	    add_send_message_tcp(buf, size, initiator);
	}
      }

      /* send record of each channel */
      for (int id = 0; id < host_names.size(); ++id)
	if (id != my_id and not records[id].empty()) {
	  size = pack_snapshot(buf, id);
	  if (im_initiator)
	    print_snapshot(buf, my_id);
	  else
	    add_send_message_tcp(buf, size, initiator);

	  for (string s: records[id]) {
	    if (im_initiator)
	      print_channel((unsigned char *)s.c_str(), my_id);
	    else
	      add_send_message_tcp(s, initiator);
	  }
	}

      size = pack_snapshot(buf, MAX_HOST);
      if (im_initiator)
	print_snapshot(buf, my_id);
      else
	add_send_message_tcp(buf, size, initiator);
      if (not im_initiator)
	cout << "Finished sending snapshot" << endl;

      if (im_initiator) {
	/* wait for other snapshot infos */
	while (true)
	  if (all_of(finish_receive.begin(), finish_receive.end(), [](bool b){return b;}))
	    break;

	/* print out the snapshot */
	for (int i = 0; i < host_names.size(); ++i) {
	  string s = snapshots[i].str();
	  if (s.size() > 0) {
	    cout << "-------------------------------------------------------" << endl;
	    cout << s;
	    cout << "-------------------------------------------------------" << endl;
	  }
	}
      }
      snapshot_status = STATUS_FINISH;
    }
    }
    this_thread::sleep_for(chrono::seconds(1));
  }
}

void init_snapshot() {
  if (im_initiator)
    initiator = my_id;
  up_tcp.resize(host_names.size(), false);
  up_tcp[my_id] = true;
  recording.resize(host_names.size(), false);
  records.resize(host_names.size());
  send_queues_tcp.resize(host_names.size());
  receiving_internal.resize(host_names.size(), true);
  finish_receive.resize(host_names.size(), false);
  snapshots.resize(host_names.size());
  if (im_initiator) {
    for (int id = 0; id < host_names.size(); ++id) {
      snapshots[id] << "Snapshot of " << host_names[id] << ":\n";
    }
  }

  init_socket_tcp();
  
  thread a(acceptor);
  a.detach();
  
  for (int id = my_id + 1; id < host_names.size(); ++id) {
    thread c(connector, id);
    c.detach();
  }

  /* block until all TCP connections are up */
  while (true) {
    this_thread::sleep_for(chrono::seconds(1));
    if (all_of(up_tcp.begin(), up_tcp.end(), [](bool b){return b;}))
      break;
  }

  thread m(snapshot_manager);
  m.detach();
}

void start_snapshot() {
  unsigned char buf[BUF_SIZE];
  /* save current state */
  save_internal();

  /* send markers and record */
  for (int id = 0; id < host_names.size(); ++id)
    if (id != my_id)
      recording[id] = true;
  snapshot_status = STATUS_RECORD;
  for (int id = 0; id < host_names.size(); ++id)
    if (id != my_id) {
      int size = pack_marker(buf, my_id, my_id);
      add_send_message_tcp(buf, size, id);
    }
}
