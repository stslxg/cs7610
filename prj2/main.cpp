#include <unistd.h>
#include <vector>
#include <set>
#include <utility>
#include <queue>
#include <thread>
#include <chrono>
#include <iostream>
#include <sstream>
#include <fstream>
#include <netdb.h>
#include <netinet/in.h>
#include <getopt.h>
#include <mutex>
#include <atomic>

using namespace std;
using namespace std::chrono_literals;

#include "common.h"
#include "main.h"

/* parameters read from command line */
vector<string> host_names;
bool verbose = false;
static int test_case;

static atomic<int> status = STATUS_UP;
int my_id = ID_UNKNOWN;

/* data structures for up */
static atomic<bool> up[MAX_HOST] = {false};  // record if received UpMessage from each server
static atomic<int> up_count = 1;  // number of servers that are already up, including myself

/* data structures for leader election */
static atomic<int> last_attempted = 0, last_installed = 0;
static int progress_timer = PROGRESS_TIMER, vp_timer = VP_TIMER;
static atomic<bool> progress_timer_set = false;  // record if the progress timer is set; not really used since we have Simplification 3
static vector<View_Change> view_changes;   // store View Change messages from each server
static set<uint32_t> vc_received;          // record if received View Change message from a server in one election
static set<pair<uint32_t, uint32_t>> vp_seen;  // for checking duplicate VC_Proof messages

/* send queues to each peer and corresponding mutexes */
static vector<queue<string>> send_queues;
static mutex send_mtxes[MAX_HOST];

/* initialize all the arrays to suitable lengths */
void init() {
  send_queues.resize(host_names.size());
  view_changes.resize(host_names.size());
  up[my_id] = true;
}

/* parse command line arguments */
void parse_command(int argc, char **argv) {
  string usage = "-h <hostfile> -t <test case> [-p <progress timer> -r <vp timer> -v]";
  
  /* require -h <hostfile> -t <test case>*/
  if (argc < 5) {
    cerr << "Usage: " << argv[0] << usage << endl;
    exit(EXIT_FAILURE);
  }

  char opt;
  while ((opt = getopt(argc, argv, "h:p:t:r:v")) != -1) {
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
      case 'p':     // -p <progress timer>
	ss >> progress_timer;
	break;
      case 'r':     // -r <VP timer>
	ss >> vp_timer;
	break;
      case 't':     // -t <test case>
	ss >> test_case;
	break;
      }
    }
  }
}

/* add message to the right queue; may have multiple writer at the same time so need mutex */
void add_send_message(unsigned char *buf, size_t send_len, int id) {
  if (id == my_id) {
    cerr << "Don't send to myself!" << endl;
    return;
  }
  lock_guard<mutex> lock(send_mtxes[id]);
  send_queues[id].push(string((char *)buf, send_len));
}

/* check conflicts of messages */
bool conflict(int message_type, unsigned char *buf) {
  switch (message_type) {
  case VC_TYPE: {
    View_Change s = unpack_vc(buf);
    if (s.server_id == my_id
	or status != LEADER_ELECTION
	or progress_timer_set
	or s.attempted <= last_installed)
      return true;
    break;
  }
  case VP_TYPE: {
    VC_Proof s = unpack_vp(buf);
    if (s.server_id == my_id
	or status != LEADER_ELECTION)
      return true;
    break;
  }
  default:
    return false;
  }
  return false;
}

/* implement the crash functionality according the test cases */
void check_and_crash() {
  if (test_case >= 3 and my_id == 1 and last_attempted >= 1
      or (test_case >= 4 and my_id == 2 and last_attempted >= 2)
      or (test_case == 5 and my_id == 3 and last_attempted >= 3)) {
    cout << host_names[my_id] << " crashes" << endl;
    exit(EXIT_FAILURE);
  }
}

/* shift to leader */
static inline void shift_to_prepare_phase() {
  cout << host_names[my_id] << ": Server " << host_names[my_id] << " is the new leader of view " << last_attempted << endl;
  if (verbose)
    cout << "Shift to prepare phase" << endl;

  /* install the view and change status */
  int temp = last_attempted;
  last_installed = temp;
  status = STATUS_OTHER;
}

/* shift to non-leader */
static inline void shift_to_reg_non_leader() {
  cout << host_names[my_id] << ": Server " << host_names[last_attempted % host_names.size()] << " is the new leader of view " << last_attempted << endl;
  if (verbose)
    cout << "Shift to reg non leader" << endl;

  /* install the view, change status */
  int temp = last_attempted;
  last_installed = temp;
  status = STATUS_OTHER;
}

/* apply View Change message to data structures */
static inline void apply_vc(View_Change s) {
  if (vc_received.find(s.server_id) == vc_received.end()) {
    view_changes[s.server_id] = s;
    vc_received.insert(s.server_id);
  }
}

/* attempt a new leader election */
static void shift_to_leader_election(int view) {
  unsigned char buf[BUF_SIZE];
  
  if (verbose)
    cout << "Shift to leader election for view " << view << endl;
  vc_received.clear();
  last_attempted = view;

  /* send out View Change message to others, apply the message to data structure, change status */
  int send_len = pack_vc(buf, my_id, view);
  View_Change s = unpack_vc(buf);
  for (int id = 0; id < host_names.size(); ++id)
    if (id != my_id)
      add_send_message(buf, send_len, id);
  apply_vc(s);
  status = LEADER_ELECTION;
}

/* check if received enough View Change messages for this view in this election */
static bool preinstall_ready(int view) {
  int count = 0;
  for (int id = 0; id < host_names.size(); ++id)
    if (vc_received.find(id) != vc_received.end() and view_changes[id].attempted == view)
      count++;
  return (count >= host_names.size() / 2 + 1);
}

/* main functions for the progress timer */
void progress_timer_func() {
  progress_timer_set = true;
  this_thread::sleep_for(chrono::milliseconds(progress_timer));
  progress_timer_set = false;
  cout << "Progress timer expires" << endl;

  /* start next leader election */
  shift_to_leader_election(last_attempted + 1);
}
static inline void set_progress_timer() {
  thread p(progress_timer_func);
  p.detach();
}

/* Periodically send VC_Proof to everyone other than myself */
void VP_sender() {
  unsigned char buf[BUF_SIZE];
  while (true) {
    this_thread::sleep_for(chrono::milliseconds(vp_timer));
    int send_len = pack_vp(buf, my_id, last_installed);
    for (int id = 0; id < host_names.size(); ++id)
      if (id != my_id) {
	add_send_message(buf, send_len, id);
      }
  }  
}

/* one of the main parts: parse message and act accordingly */
void parse_message(unsigned char *buf) {
  int message_type = unpack_type(buf);
  unsigned char send_buf[BUF_SIZE];

  /* in the up status, only deal with UpMessage */
  if (status == STATUS_UP) {
    if (message_type == UP_TYPE) {
      UpMessage s = unpack_up(buf);
      int id = s.sender;
      if (not up[id]) {
	up[id] = true;
	up_count++;
	if (verbose)
	  cout << host_names[id] << " is up!" << endl;
	
	/* if every one is up, go into next status */
	if (up_count == host_names.size()) {
	  if (verbose)
	    cout << "All process is up!" << endl;

	  /* set up VP sender */
	  thread vp_s(VP_sender);
	  vp_s.detach();

	  /* set the first progress timer */
	  cout << host_names[my_id] << ": Server " << host_names[0] << " is the leader of view 0" << endl;
	  set_progress_timer();
	}

	/* send my heartbeat since I know that process id is up */
	if (my_id != ID_UNKNOWN) {
	  get_addr_info(id);
	  int send_len = pack_up(send_buf, my_id);
	  add_send_message(send_buf, send_len, id);
	}
      }
    }
    return;
  }

  /* check for conflict */
  if (conflict(message_type, buf))
    return;

  switch (message_type) {
    /* now we can ignore up message */
  case UP_TYPE:
    break;

    /* deal with View Change messge */
  case VC_TYPE: {
    View_Change s = unpack_vc(buf);
    if (verbose)
      cout << "Received VC attempted = " << s.attempted << " from " << host_names[s.server_id] << endl;

    if (s.attempted == last_attempted) {
      apply_vc(s);
      if (preinstall_ready(s.attempted)) {
	check_and_crash();
	progress_timer *= 2;

	/* for test case 1, do not need to set progress timer */
	if (test_case >= 2)
	  set_progress_timer();

	/* shift to the corresponding phase */
	if (last_attempted % host_names.size() == my_id)
	  shift_to_prepare_phase();
	else
	  shift_to_reg_non_leader();
      }
    }
    break;
  }
    /* deal with VC_Proof message */
  case VP_TYPE: {
    VC_Proof s = unpack_vp(buf);

    /* check for duplicate VP messages for better printing in verbose mode */
    auto p = make_pair(s.server_id, s.installed);
    if (vp_seen.find(p) == vp_seen.end()) {
      if (verbose)
	cout << "Received VP installed = " << s.installed << " from " << host_names[s.server_id] << endl;
      vp_seen.insert(p);
    }

    if (s.installed > last_installed) {
      last_attempted = s.installed;
      check_and_crash();
      if (last_attempted % host_names.size() == my_id)
	shift_to_prepare_phase();
      else
	shift_to_reg_non_leader();

      /* set progress timer after changing state; note that in the paper there is no this part */
      progress_timer *= 2;
      if (not progress_timer_set and test_case >= 2)
	set_progress_timer();
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
void broadcast_up() {
  unsigned char buf[BUF_SIZE];
  
  for (int id = 0; id < host_names.size(); ++id) {
    /* check if that host name is up, if so send my heartbeat */
    if (get_addr_info(id) == 0) {
      if (id != my_id) {
	int send_len = pack_up(buf, my_id);
	add_send_message(buf, send_len, id);
      }
    }
  }
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

  /* repeatively send to other servers that I'm up */
  while (status == STATUS_UP) {        
    broadcast_up();
    this_thread::sleep_for(500ms);
  }
  
  for(;;){};    

  /* will never run to this step, but I feel I still need to add some "closure" ;) */
  if (verbose)
    cout << "Finished up" << endl;
  close_socket();
  return 0;
}

