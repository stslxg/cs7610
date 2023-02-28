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
#include "utils.h"

extern vector<string> host_names;
extern int delay;
extern double drop_rate;
extern int my_id;

static addrinfo *my_addrinfo, *their_addrinfos[MAX_HOST];
static int sock_fd;

static default_random_engine generator;
static bernoulli_distribution dist;

/* resolve address */
int get_addr_info(int id) {
  /* use -1 as id to get local address, o.w. get remote address */
  addrinfo hints = {0};

  /* set hints for addrinfo */
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;

  if (id == ID_UNKNOWN)
    return getaddrinfo(NULL, PORT, &hints, &my_addrinfo);
  else
    return getaddrinfo(host_names[id].c_str(), PORT, &hints, &their_addrinfos[id]);
}

/* initialize socket */
void init_socket() {
  int addrinfo_status = get_addr_info(ID_UNKNOWN);
  if (addrinfo_status) {
    cerr << "Failed to getaddrinfo" << gai_strerror(addrinfo_status) << endl;
    exit(EXIT_FAILURE);
  }

  /* get socket and bind */
  sock_fd = socket(my_addrinfo->ai_family, my_addrinfo->ai_socktype, my_addrinfo->ai_protocol);
  if (sock_fd < 0) {
    cerr << "Cannot create socket" << endl;
    exit(EXIT_FAILURE);
  }	
  if (bind(sock_fd, my_addrinfo->ai_addr, my_addrinfo->ai_addrlen)) {
    cerr << "Failed to bind" << endl;
    exit(EXIT_FAILURE);
  }
}

/* close socket and release resources */
void close_socket() {
  freeaddrinfo(my_addrinfo);
  for (int i = 0; i < MAX_HOST; ++i)
    freeaddrinfo(their_addrinfos[i]);
  close(sock_fd);
  cout << "Socket closed" << endl;
}

/* wrapper for recvfrom */
int receive_message(unsigned char *buf) {
  sockaddr_in rem_addr;
  socklen_t addr_len = sizeof(rem_addr);
  
  int recv_len = recvfrom(sock_fd, buf, BUF_SIZE, 0, (sockaddr *)&rem_addr, &addr_len);
  if (recv_len > 0)
    buf[recv_len] = 0;
  
  return recv_len;
}

/* wrapper for sendto, implement drop and delay, except for self-delivery */
int send_message(const unsigned char *buf, size_t send_len, int id) {
  if (id != my_id) 
    this_thread::sleep_for(chrono::milliseconds(delay));
  if (id == my_id or dist(generator)) {
    return sendto(sock_fd, buf, send_len, 0, their_addrinfos[id]->ai_addr, sizeof(struct sockaddr_in));
  }
  return -1;
}

/* initialize dist, set prob of being true to be 1 - drop_rate */
void init_random() {
  dist = bernoulli_distribution(1 - drop_rate);
}
  
