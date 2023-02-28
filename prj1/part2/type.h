#ifndef TYPE_H
#define TYPE_H

/* < sender, message id > */
typedef pair<uint32_t, uint32_t> message_key;

/* < sender, message id, seq, seq_proposer, deliverable > */
typedef tuple<uint32_t, uint32_t, uint32_t, uint32_t, bool> message_info;

#endif
