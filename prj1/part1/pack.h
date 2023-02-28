#ifndef PACK_H
#define PACK_H

uint32_t unpack_type(unsigned char *buf);

size_t pack_up(unsigned char *buf, uint32_t receiver);
UpMessage unpack_up(unsigned char *buf);

size_t pack_data(unsigned char *buf, uint32_t sender, uint32_t msg_id);
DataMessage unpack_data(unsigned char *buf);

size_t pack_ack(unsigned char *buf, uint32_t sender, uint32_t msg_id, uint32_t proposed_seq, uint32_t proposer);
AckMessage unpack_ack(unsigned char *buf);

size_t pack_seq(unsigned char *buf, uint32_t sender, uint32_t msg_id, uint32_t final_seq, uint32_t final_seq_proposer);
SeqMessage unpack_seq(unsigned char *buf);

size_t pack_ack_seq(unsigned char *buf, uint32_t sender, uint32_t msg_id, uint32_t receiver);
AckSeqMessage unpack_ack_seq(unsigned char *buf);
#endif
