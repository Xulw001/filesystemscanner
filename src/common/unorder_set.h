#ifndef _COMMON_UNORDERED_SET_H_
#define _COMMON_UNORDERED_SET_H_
#ifndef CXX11_NOT_SUPPORT
#include <unordered_set>
namespace common {
using std::unordered_set;
#else
#include <list>
#include <vector>

#include "type.h"
template <typename K>
class unordered_set {
 private:
  using _Mylist = typename std::list<K>;
  using _MyListIterator = typename _Mylist::iterator;
  using _MyConstListIterator = typename _Mylist::const_iterator;
  using _MyPairib = typename std::pair<_MyListIterator, bool>;
  using _MyVector = typename std::vector<_MyListIterator>;
  using _MyVectorIterator = typename _MyVector::iterator;

#define MIN_BUCKET (8)
 public:
  using iterator = _MyListIterator;
  using const_iterator = _MyConstListIterator;
  unordered_set() { _Init(MIN_BUCKET); }

  template <typename _Valty>
  _MyPairib emplace(_Valty&& val) {
    hash_list_.emplace_front(val);
    return _Insert(hash_list_.front(), begin());
  }

  _MyListIterator begin() { return hash_list_.begin(); }

  _MyListIterator end() { return hash_list_.end(); }

  _MyConstListIterator begin() const { return hash_list_.begin(); }

  _MyConstListIterator end() const { return hash_list_.end(); }

  _MyListIterator find(const K& key) {
    size_t buckets = _Hash(key);
    for (_MyListIterator it = _Begin(buckets); it != _End(buckets); ++it) {
      if (*it == key) {
        return it;
      }
    }
    return end();
  }

  _MyConstListIterator find(const K& key) const {
    size_t buckets = _Hash(key);
    for (_MyConstListIterator it = _Begin(buckets); it != _End(buckets); ++it) {
      if (*it == key) {
        return it;
      }
    }
    return end();
  }
  void clear() {
    hash_list_.clear();
    _Init(MIN_BUCKET);
  }

 private:
  void _Init(b32 buckets) {
    // store begin and end iterator for each buckets
    hash_table_.reserve(buckets * 2);
    hash_table_.assign(buckets * 2, hash_list_.end());
    buckets_ = buckets;
    mask_ = buckets_ - 1;
  }

  _MyListIterator _Begin(b32 buckets) { return hash_table_[buckets * 2]; }

  _MyConstListIterator _Begin(b32 buckets) const {
    return (_MyConstListIterator&)hash_table_[buckets * 2];
  }

  _MyListIterator _End(b32 buckets) {
    if (hash_table_[buckets * 2] == hash_list_.end()) {
      return hash_list_.end();
    } else {
      _MyListIterator end = hash_table_[buckets * 2 + 1];
      return (++end);
    }
  }

  _MyConstListIterator _End(b32 buckets) const {
    if (hash_table_[buckets * 2] == hash_list_.end()) {
      return hash_list_.end();
    } else {
      _MyConstListIterator end =
          (_MyConstListIterator&)hash_table_[buckets * 2 + 1];
      return (++end);
    }
  }

  void _Reinsert() {
    _MyListIterator last = end();
    if (last != begin()) {
      for (--last;;) {
        _MyListIterator cur = begin();
        bool _Done = cur == last;
        _Insert(*cur, cur);
        if (_Done) break;
      }
    }
  }

  size_t _Hash(const K& key) const { return std::hash<K>{}(key)&mask_; }

  void _Insert_Bucket(_MyListIterator data, _MyListIterator where,
                      size_t buckets) {
    if (hash_table_[buckets * 2] == hash_list_.end()) {
      hash_table_[buckets * 2] = data;
      hash_table_[buckets * 2 + 1] = data;
    } else if (hash_table_[buckets * 2] == where) {
      hash_table_[buckets * 2] = data;
    } else {
      ;  // condition
    }
  }

  _MyPairib _Insert(K& val, _MyListIterator node) {
    size_t buckets = _Hash(val);
    _MyListIterator where = _End(buckets);
    while (where != _Begin(buckets)) {
      if (*(--where) == val) {
        return _MyPairib(where, false);
      }
    }

    _MyListIterator next = node;
    if (where != ++next) {
      hash_list_.splice(where, hash_list_, node, next);
    }

    _Insert_Bucket(node, where, buckets);

    if (load_factor() > 0.75) {
      _Init(buckets_ * 2);
      _Reinsert();
    }

    return _MyPairib(node, true);
  }

  b32 bucket_count() { return buckets_; }

  float load_factor() { return hash_list_.size() / (float)buckets_; }

 private:
  _MyVector hash_table_;
  _Mylist hash_list_;
  b32 mask_;
  b32 buckets_;
};

#endif
};  // namespace common
#endif