#include <vector>
#include <map>
#include <utility>
#include <iostream>

using namespace std;

#include "common.h"
#include "heap.h"

/* operator overload for message_info = <sender, msg_id, seq, proposer, deliverable> */
bool operator<(const message_info &s1, const message_info &s2) {
  return (get<2>(s1) < get<2>(s2)  /* seq of s1 < seq of s2 */
	  or (get<2>(s1) == get<2>(s2) and get<3>(s1) < get<3>(s2)));
	  /* or proposer of s1 < proposer of s2 */
}

bool min_heap::empty() {
  return h.empty();
}

message_info min_heap::top() {
  return h.front();
}

/* swap two elements in heap, maintain m */
void min_heap::swap(int i, int j) {
  message_key m_i = make_pair(get<0>(h[i]), get<1>(h[i]));
  message_key m_j = make_pair(get<0>(h[j]), get<1>(h[j]));
  m[m_i] = j;
  m[m_j] = i;
  
  message_info tmp = h[i];
  h[i] = h[j];
  h[j] = tmp;
}

void min_heap::sift_up() {
  int pos = h.size() - 1;
  while (pos > 0) {
    int parent = (pos - 1) / 2;
    if (h[pos] < h[parent]) {
      swap(pos, parent);
      pos = parent;
    } else {
      break;
    }
  }
}

void min_heap::sift_down(int pos) {
  int parent = pos;
  int child = 2 * parent + 1;
  while (child < h.size()) {
    if (child < h.size() - 1 and h[child + 1] < h[child])
      ++child;
    if (h[parent] < h[child])
      break;
    swap(parent, child);
    parent = child;
    child = 2 * parent + 1;
  }
}

void min_heap::pop() {
  message_key key = make_pair(get<0>(h[0]), get<1>(h[0]));
  m.erase(key);
  swap(0, h.size() - 1);
  h.pop_back();
  sift_down(0);
}

void min_heap::push(uint32_t sender, uint32_t msg_id, uint32_t seq, uint32_t proposer, bool deliverable) {
  h.emplace_back(sender, msg_id, seq, proposer, deliverable);
  message_key key = make_pair(sender, msg_id);
  m[key] = h.size() - 1;
  sift_up();
}

void min_heap::increase_key(uint32_t sender, uint32_t msg_id, uint32_t seq, uint32_t proposer, bool deliverable) {
  message_key key = make_pair(sender, msg_id);
  if (m.find(key) != m.end()) {
    int pos = m[key];
    h[pos] = make_tuple(sender, msg_id, seq, proposer, deliverable);
    sift_down(pos);
  } else {
    cerr << "No key in heap" << endl;
  }
}
