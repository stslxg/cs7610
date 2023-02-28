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
size_t pack_up(unsigned char *buf, uint32_t receiver) {
  return pack_full(buf, UP_MSG_SIZE, UP_MSG_TYPE, receiver);
}

UpMessage unpack_up(unsigned char *buf) {
  UpMessage s;
  unpack_full(buf, UP_MSG_SIZE, &s.type, &s.receiver);
  return s;
}

/* pack and unpack for DataMessage */
size_t pack_data(unsigned char *buf, uint32_t sender, uint32_t msg_id, uint32_t data) {
  return pack_full(buf, DATA_MSG_SIZE, DATA_MSG_TYPE, sender, msg_id, data);
}

DataMessage unpack_data(unsigned char *buf) {
  DataMessage s;
  unpack_full(buf, DATA_MSG_SIZE, &s.type, &s.sender, &s.msg_id, &s.data);
  return s;
}

/* pack and unpack for AckMessage */
size_t pack_ack(unsigned char *buf, uint32_t sender, uint32_t msg_id, uint32_t proposed_seq, uint32_t proposer) {
  return pack_full(buf, ACK_MSG_SIZE, ACK_MSG_TYPE, sender, msg_id, proposed_seq, proposer);
}

AckMessage unpack_ack(unsigned char *buf) {
  AckMessage s;
  unpack_full(buf, ACK_MSG_SIZE, &s.type, &s.sender, &s.msg_id, &s.proposed_seq, &s.proposer);
  return s;
}

/* pack and unpack for SeqMessage */
size_t pack_seq(unsigned char *buf, uint32_t sender, uint32_t msg_id, uint32_t final_seq, uint32_t final_seq_proposer) {
  return pack_full(buf, SEQ_MSG_SIZE, SEQ_MSG_TYPE, sender, msg_id, final_seq, final_seq_proposer);
}

SeqMessage unpack_seq(unsigned char *buf) {
  SeqMessage s;
  unpack_full(buf, SEQ_MSG_SIZE, &s.type, &s.sender, &s.msg_id, &s.final_seq, &s.final_seq_proposer);
  return s;
}

/* pack and unpack for AckSeqMessage */
size_t pack_ack_seq(unsigned char *buf, uint32_t sender, uint32_t msg_id, uint32_t receiver) {
  return pack_full(buf, ACK_SEQ_MSG_SIZE, ACK_SEQ_MSG_TYPE, sender, msg_id, receiver);
}

AckSeqMessage unpack_ack_seq(unsigned char *buf) {
  AckSeqMessage s;
  unpack_full(buf, ACK_SEQ_MSG_SIZE, &s.type, &s.sender, &s.msg_id, &s.receiver);
  return s;
}

/* pack and unpack for MarkerMessage */
size_t pack_marker(unsigned char *buf, uint32_t initiator, uint32_t sender) {
  return pack_full(buf, MARKER_MSG_SIZE, MARKER_MSG_TYPE, initiator, sender);
}

MarkerMessage unpack_marker(unsigned char *buf) {
  MarkerMessage s;
  unpack_full(buf, MARKER_MSG_SIZE, &s.type, &s.initiator, &s.sender);
  return s;
}

/* pack and unpack for SnapshotMessage */
size_t pack_snapshot(unsigned char *buf, uint32_t channel) {
  return pack_full(buf, SNAPSHOT_MSG_SIZE, SNAPSHOT_MSG_TYPE, channel);
}

SnapshotMessage unpack_snapshot(unsigned char *buf) {
  SnapshotMessage s;
  unpack_full(buf, SNAPSHOT_MSG_SIZE, &s.type, &s.channel);
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
