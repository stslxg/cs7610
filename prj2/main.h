#include "utils.h"
#include "pack.h"

/* status for main thread */
#define STATUS_UP 0
#define LEADER_ELECTION 1
#define STATUS_OTHER 2

/* < message, receiver > */
typedef pair<string, int> send_pair;
