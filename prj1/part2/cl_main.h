#include "utils_tcp.h"
#include "pack.h"
#include "type.h"
#include "heap.h"

#ifndef CL_MAIN_H
#define CL_MAIN_H

void add_record(unsigned char *buf, size_t send_len, int id);
void init_snapshot();
void start_snapshot();

#endif
