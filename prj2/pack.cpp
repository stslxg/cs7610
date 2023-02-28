#include <cstdint>
#include <cstdarg>

using namespace std;

#include "common.h"
#include "pack.h"

static inline size_t pack_u32(unsigned char *buf, uint32_t u);
static inline uint32_t unpack_u32(unsigned char *buf);
static size_t pack_full(unsigned char *buf, int count, ...);
static void unpack_full(unsigned char *buf, int count, ...);

/* unpack message type */
uint32_t unpack_type(unsigned char *buf) {
  return unpack_u32(buf);
}

/* pack and unpack for UpMessage */
size_t pack_up(unsigned char *buf, uint32_t sender) {
  return pack_full(buf, UP_SIZE, UP_TYPE, sender);
}

UpMessage unpack_up(unsigned char *buf) {
  UpMessage s;
  unpack_full(buf, UP_SIZE, &s.type, &s.sender);
  return s;
}

/* pack and unpack for View_Change */
size_t pack_vc(unsigned char *buf, uint32_t server_id, uint32_t attempted) {
  return pack_full(buf, VC_SIZE, VC_TYPE, server_id, attempted);
}

View_Change unpack_vc(unsigned char *buf) {
  View_Change s;
  unpack_full(buf, VC_SIZE, &s.type, &s.server_id, &s.attempted);
  return s;
}

/* pack and unpack for VC_Proof */
size_t pack_vp(unsigned char *buf, uint32_t server_id, uint32_t installed) {
  return pack_full(buf, VP_SIZE, VP_TYPE, server_id, installed);
}

VC_Proof unpack_vp(unsigned char *buf) {
  VC_Proof s;
  unpack_full(buf, VP_SIZE, &s.type, &s.server_id, &s.installed);
  return s;
}

/* The following codes are modified from Beej's Guide */
/*
** pack_i32() -- store a 32-bit unsigned int into a char buffer (like htonl())
*/
size_t pack_u32(unsigned char *buf, uint32_t u) {
  *buf++ = u >> 24;
  *buf++ = u >> 16;
  *buf++ = u >> 8;
  *buf++ = u;
  return sizeof(uint32_t);
}

/*
** unpack_u32() -- unpack a 32-bit unsigned from a char buffer (like ntohl())
*/
uint32_t unpack_u32(unsigned char *buf) {
  return ((uint32_t) buf[0] << 24) |
    ((uint32_t) buf[1] << 16) |
    ((uint32_t) buf[2] << 8) |
    buf[3];
}

/* variadic function, will receive "count"-many uint32_t vars and pack */
size_t pack_full(unsigned char *buf, int count, ...) {
  va_list args;
  size_t size = 0;
  
  va_start(args, count);
  for (int i = 0; i < count; i++) {
    size += sizeof(uint32_t);
    uint32_t L = va_arg(args, uint32_t);
    pack_u32(buf, L);
    buf += sizeof(uint32_t);
  }
  va_end(args);
  return size;
}

/* variadic function, will unpack to "count"-many uint32_t vars */
void unpack_full(unsigned char *buf, int count, ...) {
  va_list args;

  va_start(args, count);
  for (int i = 0; i < count; i++) {
    uint32_t *L = va_arg(args, uint32_t*);
    *L = unpack_u32(buf);
    buf += sizeof(uint32_t);
  }
  va_end(args);
}
