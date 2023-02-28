#ifndef UTILS_H
#define UTILS_H

int get_addr_info(int id);
void init_socket();
void close_socket();
int receive_message(unsigned char *buf);
int send_message(const unsigned char *buf, size_t send_len, int id);
void init_random();

#endif
