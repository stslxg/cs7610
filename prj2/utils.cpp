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
extern int my_id;
extern bool verbose;

static addrinfo *my_addrinfo, *their_addrinfos[MAX_HOST];
static int sock_fd;

/* resolve address */
int get_addr_info(int id) {
  /* use -1 as id to get local address, o.w. get remote address */
  addrinfo hints = {0};

  /* set hints for addrinfo */
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;

  static in_addr *my_addr;
  
  if (id == ID_UNKNOWN) {
    /* use my own hostname to get my IP address */
    /* previously I used NULL but that would get 0.0.0.0, helpless for IP address comparision to get my_id */
    char buf[BUF_SIZE];

    if (gethostname(buf, BUF_SIZE) != 0) {
      cerr << "Failed to gethostname" << endl;
      exit(EXIT_FAILURE);
    }
    
    int res = getaddrinfo(buf, PORT, &hints, &my_addrinfo);
    if (res == 0) {
      /* record my IP address */
      my_addr = &((sockaddr_in *)my_addrinfo->ai_addr)->sin_addr;
    }
    return res;
  } else {
    int res = getaddrinfo(host_names[id].data(), PORT, &hints, &their_addrinfos[id]);
    if (res == 0 and my_id == ID_UNKNOWN) {
      /* check if it is my IP address */
      in_addr *their_addr = &((sockaddr_in *)their_addrinfos[id]->ai_addr)->sin_addr;
      
      if (memcmp(my_addr, their_addr, sizeof(struct in_addr)) == 0) {
	my_id = id;
	if (verbose)
	  cout << "I'm " << host_names[my_id] << endl;
      }
    }
    return res;
  }
}

/* initialize socket */
void init_socket() {
  /* get my IP address then compare to get my_id */
  int addrinfo_status = get_addr_info(ID_UNKNOWN);
  if (addrinfo_status) {
    cerr << "Failed to getaddrinfo" << gai_strerror(addrinfo_status) << endl;
    exit(EXIT_FAILURE);
  }
  for (int id = 0; id < host_names.size(); ++id)
    get_addr_info(id);
  
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

/* wrapper for sendto */
int send_message(const unsigned char *buf, size_t send_len, int id) {
  return sendto(sock_fd, buf, send_len, 0, their_addrinfos[id]->ai_addr, sizeof(struct sockaddr_in));
}
