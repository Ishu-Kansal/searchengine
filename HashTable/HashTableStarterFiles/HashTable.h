// Simple hash table template.

// Nicole Hamilton  nham@umich.edu

#pragma once

#include <cassert>
#include <cstdint>
#include <iomanip>
#include <iostream>

using namespace std;

// You may add additional members or helper functions.

// Compare C-strings, return true if they are the same.

bool CompareEqual(const char *L, const char *R);

template <typename T>
struct Hash {
  size_t operator()(const T &t) const {
    assert(false);
    return 0;
  }
};

template <const char *>
struct Hash {
  // Uses Fowler-Noll-Vo hash function
  size_t operator()(const char *c) const {
    static const size_t FnvOffsetBasis = 146959810393466560UL;
    static const size_t FnvPrime = 1099511628211UL;
    size_t hash = FnvOffsetBasis;
    for (; *c; ++c) {
      hash *= FnvPrime;
      hash ^= *c;
    }
    return hash;
  }
};

template <typename T>
struct Equals {
  bool operator()(const T &t1, const T &t2) const {
    assert(false);
    return t1 == t2;
  }
};

template <const char *>
struct Equals {
  bool operator()(const char *c1, const char *c2) const {
    return CompareEqual(c1, c2);
  }
};

template <typename Key, typename Value>
class Tuple {
 public:
  Key key;
  Value value;

  Tuple(const Key &k, const Value v) : key(k), value(v) {}
};

template <typename Key, typename Value>
class Bucket {
 public:
  Bucket *next;
  uint32_t hashValue;
  Tuple<Key, Value> tuple;

  Bucket(const Key &k, uint32_t h, const Value v)
      : tuple(k, v), next(nullptr), hashValue(h) {}

  ~Bucket() { delete next; }
};

template <typename Key, typename Value>
class HashTable {
 private:
  // Your code here.

  Bucket<Key, Value> **buckets;
  size_t numberOfBuckets;
  size_t numberOfElements;

  friend class Iterator;
  friend class HashBlob;

 public:
  Tuple<Key, Value> *Find(const Key k, const Value initialValue) {
    // Search for the key k and return a pointer to the
    // ( key, value ) entry.  If the key is not already
    // in the hash, add it with the initial value.

    // Your code here.
    size_t hash = Hash<Key>()(k);
    size_t index = hash % numberOfBuckets;
    Bucket<Key, Value> *cur = buckets[index];
    if (!cur) {
      buckets[index] = new Bucket<Key, Value>(k, hash, initialValue);
      ++numberOfElements;
      return &(buckets[index]->tuple);
    }
    while (cur && cur->next && cur->hash != hash &&
           Equals<Key>()(cur->tuple.key, k)) {
      cur = cur->next;
    }
    if (cur->hash == hash && Equals<Key>()(cur->tuple.key, k)) {
      return &cur->tuple;
    } else {
      cur->next = new Bucket<Key, Value>(k, hash, initialValue);
      ++numberOfElements;
      return &(cur->next->tuple);
    }
    assert(false);
    return nullptr;
  }

  Tuple<Key, Value> *Find(const Key k) const {
    // Search for the key k and return a pointer to the
    // ( key, value ) enty.  If the key is not already
    // in the hash, return nullptr.

    // Your code here.

    return nullptr;
  }

  void Optimize(double loading = 1.5) {
    // Modify or rebuild the hash table as you see fit
    // to improve its performance now that you know
    // nothing more is to be added.

    // Your code here.
  }

  // Your constructor may take as many default arguments
  // as you like.

  HashTable() {
    // Your code here.
    numberOfBuckets = 1000;
    buckets = new Bucket<Key, Value> *[numberOfBuckets];
  }

  ~HashTable() {
    // Your code here.
    delete[] buckets;
  }

  class Iterator {
   private:
    friend class HashTable;

    // Your code here.
    HashTable *table;
    size_t bucket;
    Bucket<Key, Value> *b;

    Iterator(HashTable *table, size_t bucket, Bucket<Key, Value> *b) {
      // Your code here.
      this->table = table;
      this->bucket = bucket;
      this->b = b;
    }

   public:
    Iterator() : Iterator(nullptr, 0, nullptr) {}

    ~Iterator() {}

    Tuple<Key, Value> &operator*() {
      // Your code here.
      return b->tuple;
    }

    Tuple<Key, Value> *operator->() const {
      // Your code here.
      return &b->tuple;
    }

    // Prefix ++
    Iterator &operator++() {
      // Your code here.
      if (b && b->next) {
        b = b->next;
        return;
      }
      while (bucket < table->numberOfBuckets && !table->buckets[bucket])
        ++bucket;
      if (bucket != table->numberOfBuckets)
        b = table->buckets[bucket];
      else
        b = nullptr;
    }

    // Postfix ++
    Iterator operator++(int) {
      // Your code here.
      Iterator it = *this;
      ++(*this);
      return it;
    }

    bool operator==(const Iterator &rhs) const {
      // Your code here.
      return (table == rhs.table) && (bucket == rhs.bucket) && (b == rhs.b);
    }

    bool operator!=(const Iterator &rhs) const {
      // Your code here.
      return !(*this == rhs);
    }
  };

  Iterator begin() {
    // Your code here.
    return ++Iterator(this, 0, nullptr);
  }

  Iterator end() {
    // Your code here.
    return Iterator(this, numberOfBuckets, nullptr);
  }
};
