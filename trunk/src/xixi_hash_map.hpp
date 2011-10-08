/*
   Copyright [2011] [Yao Yuan(yeaya@163.com)]

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef XIXI_HASH_MAP_H
#define XIXI_HASH_MAP_H

#include "util.h"
#include "defines.h"
#include <stdio.h>

namespace xixi {

template <class K, class T, class hash_value_type = uint32_t>
class hash_node_base {
public:
  hash_node_base() {
    hash_next_ = NULL;
    hash_value_ = 0;
  }

  inline void reset_hash_node_base() {
    hash_next_ = NULL;
    hash_value_ = 0;
  }

  inline bool is_key(const K* k) const;
  inline void set_hash_value(hash_value_type hash_value) { hash_value_ = hash_value; }
  inline hash_value_type get_hash_value() { return hash_value_; }

  T* hash_next_;    // hash chain next
  hash_value_type hash_value_;
};

template <class K, class T, class hash_value_type = uint32_t>
class hash_node_ext : public T {
public:
  hash_node_ext() {
    hash_next_ = NULL;
    hash_value_ = 0;
  }

  inline void reset_hash_node_ext() {
    hash_next_ = NULL;
    hash_value_ = 0;
  }

  inline bool is_key(const K* k) const;
  inline void set_hash_value(hash_value_type hash_value) { hash_value_ = hash_value; }
  inline hash_value_type get_hash_value() { return hash_value_; }

  hash_node_ext* hash_next_;    // hash chain next
  hash_value_type hash_value_;
};

template <class K, class  T>
struct default_PK {
  bool inline is_key(const K* k, const T* t) const { return t->is_key(k); }
};

template <class K, class T, class PK = default_PK<K, T>, class hash_value_type = uint32_t, class size_type = uint32_t>
class hash_map {
public:
  hash_map(size_type bucket_size = 7) {
    hash_table_ = NULL;
    size_ = 0;
    bucket_size_ = bucket_size;
    expand_size_ = bucket_size_ * 3 / 2;
    hash_table_ = (T**)malloc(bucket_size_ * sizeof(T*));
    if (hash_table_ == NULL) {
      printf("Failed to init hash map.\n");
      exit(EXIT_FAILURE);
    }
    memset(hash_table_, 0, sizeof(T*) * bucket_size_);
  }

  ~hash_map() {
    if (hash_table_ != NULL) {
      free(hash_table_);
      hash_table_ = NULL;
    }
  }

  static size_type next_bucket_size(size_type size) {
    static const size_type sizes[] = {
      3, 7, 13, 23, 53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593,
      49157, 98317, 196613, 393241, 786433, 1572869, 3145739, 6291469,
      12582917, 25165843, 0
    };
    for (int i = 0; sizes[i] > 0; ++i) {
      if (sizes[i] > size) {
        return sizes[i];
      }
    }
    return size * 2;
  }

  inline T* find(const K* k, hash_value_type hash_value) const {
    T* p = hash_table_[hash_value % bucket_size_];
    while (p != NULL) {
      if (pk_.is_key(k, p)) {
        return p;
      }
      p = p->hash_next_;
    }
    return NULL;
  }

  inline void insert(T* p, hash_value_type hash_value) {
    p->hash_value_ = hash_value;
    hash_value_type bucket = hash_value % bucket_size_;
    p->hash_next_ = hash_table_[bucket];
    hash_table_[bucket] = p;

    ++size_;
    if (size_ > expand_size_) {
      expand();
    }
  }
/*
  inline void insert(T* p) {
    ST bucket = p->hash_value_ % bucket_size_;
    p->hash_next_ = hash_table_[bucket];
    hash_table_[bucket] = p;

    ++size_;
    if (size_ > expand_size_) {
      expand();
    }
  }
*/
  inline T* remove(const T* t) {
    hash_value_type bucket = t->hash_value_ % bucket_size_;
    T* curr = hash_table_[bucket];
    T* prev = NULL;
    while (curr != NULL) {
      if (curr == t) {
        if (prev != NULL) {
          prev->hash_next_ = curr->hash_next_;
        } else {
          hash_table_[bucket] = curr->hash_next_;
        }
        curr->hash_next_ = NULL;
        --size_;
        return curr;
      }
      prev = curr;
      curr = curr->hash_next_;
    }
    return NULL;
  }

  inline T* remove(const K* k, hash_value_type hash_value) {
    hash_value_type bucket = hash_value % bucket_size_;
    T* curr = hash_table_[bucket];
    T* prev = NULL;
    while (curr != NULL) {
      if (pk_.is_key(k, curr)) {
        if (prev != NULL) {
          prev->hash_next_ = curr->hash_next_;
        } else {
          hash_table_[bucket] = curr->hash_next_;
        }
        curr->hash_next_ = NULL;
        --size_;
        return curr;
      }
      prev = curr;
      curr = curr->hash_next_;
    }
    return NULL;
  }

  inline size_type size() { return size_; }
  inline bool empty() { return size_ == 0; }

private:
  void expand() {
    size_type new_bucket_size = next_bucket_size(bucket_size_);
    T** new_hashtable = (T**)malloc(new_bucket_size * sizeof(T*));
    if (new_hashtable != NULL) {
      memset(new_hashtable, 0, sizeof(T*) * new_bucket_size);
      T* p;
      T* next;
      hash_value_type bucket;
      for (size_type i = 0; i < bucket_size_; ++i) {
        for (p = hash_table_[i]; NULL != p;) {
          bucket = p->hash_value_ % new_bucket_size;
          next = p->hash_next_;
          p->hash_next_ = new_hashtable[bucket];
          new_hashtable[bucket] = p;
          p = next;
        }
        hash_table_[i] = NULL;
      }
      bucket_size_ = new_bucket_size;
      expand_size_ = bucket_size_ * 3 / 2;
      free(hash_table_);
      hash_table_ = new_hashtable;
    }
  }

  PK pk_;
  size_type bucket_size_;
  size_type expand_size_;
  T** hash_table_;
  size_type size_;
};

} // namespace xixi

#endif // XIXI_HASH_MAP_H
