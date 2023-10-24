#ifndef COMMON_QUEUE_H_
#define COMMON_QUEUE_H_
#include <atomic>

#include "type.h"

namespace common {
template <typename T>
class queue {
 private:
  typedef struct _Item {
    T data;
    _Item *next;
  } Item;

 public:
  queue() {
    head_ = NULL;
    tail_ = NULL;
    size_ = 0;
  }

  ~queue() {
    while (head_) {
      Item *node = head_;
      head_ = head_->next;
      delete node;
    }
  }

  void push(T value) {
    Item *node = new Item();
    node->data = value;
    node->next = NULL;

    if (!head_) {
      tail_->next = node;
    } else {
      head_ = node;
    }
    tail_ = node;

    size_++;
  }

  /*
   * Called by the consumer to get and remove the latest item
   */
  T pop() {
    if (size_ == 0) {
      return {0};
    } else {
      Item *node = head_;
      head_ = node->next;
      T data = node->data;

      delete node;
      size_--;

      return data;
    }
  }

  bool size() { return size_; }

  bool empty() { return size_ == 0; }

 private:
  Item *head_;
  Item *tail_;
  std::atomic<b32> size_;
};

};  // namespace common
#endif