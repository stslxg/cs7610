#include "type.h"

#ifndef HEAP_H
#define HEAP_H

/* min heap, specifically for this problem */
class min_heap {
private:
  vector<message_info> h; // min heap
  map<message_key, int> m; // to get position of <sender, msg_id> in the heap
  void swap(int i, int j);
  void sift_down(int pos);
  void sift_up();
public:
  min_heap(){};
  void push(uint32_t sender, uint32_t msg_id, uint32_t seq, uint32_t proposer, bool deliverable);
  message_info top();
  void pop();
  bool empty();
  void increase_key(uint32_t sender, uint32_t msg_id, uint32_t seq, uint32_t proposer, bool deliverable);
  vector<message_info> save();
};

#endif
