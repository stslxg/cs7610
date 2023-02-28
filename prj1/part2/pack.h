#ifndef PACK_H
#define PACK_H

uint32_t unpack_type(unsigned char *buf);

size_t pack_up(unsigned char *buf, uint32_t receiver);
UpMessage unpack_up(unsigned char *buf);

size_t pack_data(unsigned char *buf, uint32_t sender, uint32_t msg_id, uint32_t data);
DataMessage unpack_data(unsigned char *buf);

size_t pack_ack(unsigned char *buf, uint32_t sender, uint32_t msg_id, uint32_t proposed_seq, uint32_t proposer);
AckMessage unpack_ack(unsigned char *buf);

size_t pack_seq(unsigned char *buf, uint32_t sender, uint32_t msg_id, uint32_t final_seq, uint32_t final_seq_proposer);
SeqMessage unpack_seq(unsigned char *buf);

size_t pack_ack_seq(unsigned char *buf, uint32_t sender, uint32_t msg_id, uint32_t receiver);
AckSeqMessage unpack_ack_seq(unsigned char *buf);

size_t pack_marker(unsigned char *buf, uint32_t initiator, uint32_t sender);
MarkerMessage unpack_marker(unsigned char *buf);

size_t pack_snapshot(unsigned char *buf, uint32_t channel);
SnapshotMessage unpack_snapshot(unsigned char *buf);

#endif
