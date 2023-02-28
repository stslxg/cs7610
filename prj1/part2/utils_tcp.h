#ifndef UTILS_TCP_H
#define UTILS_TCP_H

void init_socket_tcp();
void connect_to(int id);
void close_socket_tcp();
int receive_message_tcp(unsigned char *buf, int id);
int send_message_tcp(const unsigned char *buf, size_t send_len, int id);
void acceptor();

#endif
