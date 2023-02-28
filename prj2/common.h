#ifndef COMMON_H
#define COMMON_H

/* config parameters */
static const char *PORT = "1153";
static const size_t BUF_SIZE = 2048;
static const int MAX_HOST = 10;
static const int PROGRESS_TIMER = 1000;
static const int VP_TIMER = 500;

typedef struct {
  uint32_t type;      // must be equal to 1
  uint32_t sender;
} UpMessage;

typedef struct {
  uint32_t type;      // must be equal to 2
  uint32_t server_id;
  uint32_t attempted;
} View_Change;

typedef struct {
  uint32_t type;      // must be equal to 3
  uint32_t server_id;
  uint32_t installed;
} VC_Proof;

/* type constants of messages */
#define UP_TYPE 1
#define VC_TYPE 2
#define VP_TYPE 3

/* size for each type of messages */
#define UP_SIZE 2
#define VC_SIZE 3
#define VP_SIZE 3

#define ID_UNKNOWN (-1)

#endif
