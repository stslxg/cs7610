#include "utils.h"
#include "pack.h"
#include "heap.h"
#include "cl_main.h"
#include "utils_tcp.h"
#include "type.h"

/* status for main thread */
#define STATUS_UP 0
#define STATUS_MULTICAST 1

/* < message, receiver > */
typedef pair<string, int> send_pair;
