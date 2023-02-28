#include <unistd.h>
#include <iostream>
#include <cstring>
#include <vector>
#include <thread>
#include <chrono>
#include <random>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

using namespace std;

#include "common.h"
#include "utils_tcp.h"

extern vector<string> host_names;
extern int delay;
extern double drop_rate;
extern int my_id;
extern bool verbose;

extern void init_sender_receiver(int id);

static vector<int> socks_tcp;
static addrinfo *my_addrinfo_tcp, *their_addrinfos_tcp[MAX_HOST];
static int sock_fd_tcp;

/* resolve TCP address */
int get_addr_info_tcp(int id) {
  addrinfo hints = {0};

  /* set hints for addrinfo */
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  if (id == ID_UNKNOWN)
    return getaddrinfo(NULL, TCP_PORT, &hints, &my_addrinfo_tcp);
  else
    return getaddrinfo(host_names[id].c_str(), TCP_PORT, &hints, &their_addrinfos_tcp[id]);
}

/* initialize TCP socket for receiving */
void init_socket_tcp() {
  int addrinfo_status = get_addr_info_tcp(ID_UNKNOWN);
  if (addrinfo_status) {
    cerr << "Failed to getaddrinfo" << gai_strerror(addrinfo_status) << endl;
    exit(EXIT_FAILURE);
  }

  /* get socket and bind */
  sock_fd_tcp = socket(my_addrinfo_tcp->ai_family, my_addrinfo_tcp->ai_socktype, my_addrinfo_tcp->ai_protocol);
  if (sock_fd_tcp < 0) {
    cerr << "Cannot create socket" << endl;
    exit(EXIT_FAILURE);
  }	
  if (bind(sock_fd_tcp, my_addrinfo_tcp->ai_addr, my_addrinfo_tcp->ai_addrlen)) {
    cerr << "Failed to bind" << endl;
    exit(EXIT_FAILURE);
  }
  
  /* init socket vectors */
  socks_tcp.resize(host_names.size());
  for (int i = 0; i < my_id; ++i) {
      int addrinfo_status = get_addr_info_tcp(i);
      if (addrinfo_status) {
	cerr << "Failed to getaddrinfo for TCP " << host_names[i] << gai_strerror(addrinfo_status) << endl;
	exit(EXIT_FAILURE);
      }
  }
}

/* initialize TCP sockets for sending and receiving */
void connect_to(int id) {
  int addrinfo_status = get_addr_info_tcp(id);
  if (addrinfo_status) {
    cerr << "Failed to getaddrinfo for TCP remote " << host_names[id] << " , "  << gai_strerror(addrinfo_status) << endl;
    exit(EXIT_FAILURE);
  }

  /* get TCP socket, bind, and connect */
  socks_tcp[id] = socket(their_addrinfos_tcp[id]->ai_family, their_addrinfos_tcp[id]->ai_socktype, their_addrinfos_tcp[id]->ai_protocol);
  if (socks_tcp[id] < 0) {
    cerr << "Cannot create TCP socket to connect to" << host_names[id] <<  endl;
    exit(EXIT_FAILURE);
  }	

  if (verbose)
    cout << "Connecting to " << host_names[id] << endl;
  while (true) {
    if (connect(socks_tcp[id], their_addrinfos_tcp[id]->ai_addr, their_addrinfos_tcp[id]->ai_addrlen) != -1)
      break;
    this_thread::sleep_for(chrono::seconds(1));
  }
}

/* close socket and release resources */
void close_socket_tcp() {
  freeaddrinfo(my_addrinfo_tcp);
  for (int i = 0; i < MAX_HOST; ++i)
    freeaddrinfo(their_addrinfos_tcp[i]);
  for (auto i: socks_tcp)
    close(i);
  cout << "TCP sockets closed" << endl;
}

/* wrapper for recv */
int receive_message_tcp(unsigned char *buf, int id) {
  int recv_len = recv(socks_tcp[id], buf, BUF_SIZE, 0);
  if (recv_len > 0)
    buf[recv_len] = 0;
  return recv_len;
}

/* wrapper for send */
int send_message_tcp(const unsigned char *buf, size_t send_len, int id) {
  size_t b_left = send_len;
  while (b_left > 0) {
    size_t size = send(socks_tcp[id], buf, send_len, 0);
    if (size == -1)
      return size;
    b_left -= size;
    buf += size;
  }
}

void *get_in_addr(sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void acceptor() {
  if (listen(sock_fd_tcp, host_names.size()) == -1) {
    cerr << "Failed to listen" << endl;
    exit(EXIT_FAILURE);
  }

  sockaddr_storage their_addr;
  socklen_t sin_size;
  while (true) {
    sin_size = sizeof(their_addr);
    int new_fd = accept(sock_fd_tcp, (sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1) {
      cerr << "Failed to accept" << endl;
      exit(EXIT_FAILURE);
    }

    if (verbose)
      cout << "Incoming connection" << endl;
    int id = ID_UNKNOWN;
    for (int i = 0; i < my_id; ++i) {
      if (memcmp(get_in_addr((sockaddr *)&their_addr),
		 get_in_addr((sockaddr *)their_addrinfos_tcp[i]->ai_addr),
		 sizeof(struct in_addr)) == 0) {
	id = i;
	break;
      }
    }
    
    if (id == ID_UNKNOWN) {
      cerr << "Failed to get host id" << endl;
      exit(EXIT_FAILURE);
    }
    if (verbose)
      cout << "It is " << host_names[id] << endl;
    socks_tcp[id] = new_fd;
    thread t(init_sender_receiver, id);
    t.detach();
  }
}
