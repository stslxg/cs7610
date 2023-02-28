#include <cstdio>
#include <unistd.h>
#include <thread>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

const char *port = "1153";
const size_t buf_size = 2048;

struct addrinfo hints, *my_res, *rem_res;
struct sockaddr_in remaddr;     
socklen_t addrlen = sizeof(remaddr);
int recvlen;
int fd;
char buf[buf_size];
int status;

void recv_and_print() {
  /* now loop, receiving data and printing what we received */
  for (;;) {
    cout << "waiting on port " << port << endl;
    recvlen = recvfrom(fd, buf, buf_size, 0, (struct sockaddr *)&remaddr, &addrlen);
    cout << "received " << recvlen << " bytes" << endl;
    if (recvlen > 0) {
      buf[recvlen] = 0;
      cout << "received message: " << buf << endl;
    }
  }
}

int main(int argc, char **argv)
{
  char ipstr[INET6_ADDRSTRLEN];
  void *addr;
  
  gethostname(buf, buf_size);
  cout << "Host name: " << buf << endl;

  /* get hints for local addrinfo */
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;

  status = getaddrinfo(buf, port, &hints, &my_res);
  if (status) {
    cerr << "getaddrinfo local failed: " << gai_strerror(status) << endl;
    exit(1);
  }
  struct sockaddr_in *ipv4 = (struct sockaddr_in *)my_res->ai_addr;
  addr = &(ipv4->sin_addr);
  inet_ntop(my_res->ai_family, addr, ipstr, sizeof ipstr);
  cout << ipstr << endl;
  
  status = getaddrinfo("p2", port, &hints, &rem_res);
  if (status) {
    cerr << "getaddrinfo remote failed: " << gai_strerror(status) << endl;
    exit(1);
  }
  ipv4 = (struct sockaddr_in *)rem_res->ai_addr;
  addr = &(ipv4->sin_addr);
  inet_ntop(rem_res->ai_family, addr, ipstr, sizeof ipstr);
  cout << ipstr << endl;


  /* get socket */
  fd = socket(my_res->ai_family, my_res->ai_socktype, my_res->ai_protocol);
  if (fd < 0) {
    cerr << "cannot create socket." << endl;
    exit(1);
  }
	
  if (bind(fd, my_res->ai_addr, my_res->ai_addrlen)) {
    cerr << "bind failed" << endl;
    exit(1);
  }

  
  
  thread t(recv_and_print);
  t.detach();

  string s;
  while (getline(cin, s)) {
    if (sendto(fd, s.c_str(), s.size(), 0, rem_res->ai_addr, addrlen) < 0) {
      cerr << "sendto failed." << endl;
    }
  }

  /* close socket and release resources */
  freeaddrinfo(my_res);
  freeaddrinfo(rem_res);
  close(fd);
  cout << "Socket closed." << endl;
  for (;;) {};
}
