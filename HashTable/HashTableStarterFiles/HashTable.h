// Simple hash table template.

// Nicole Hamilton  nham@umich.edu

#pragma once

#include <cassert>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
using namespace std;

// You may add additional members or helper functions.

// Compare C-strings, return true if they are the same.

bool CompareEqual(const char *L, const char *R);

template <typename T>
struct HashT
{
  size_t operator()(const T &t) const
  {
    assert(false);
    return 0;
  }

};

template <>
struct HashT<const char *>
{
  // Uses Fowler-Noll-Vo hash function
  size_t operator()(const char *c) const
  {
    static constexpr size_t FnvOffsetBasis = 146959810393466560UL;
    static constexpr size_t FnvPrime = 1099511628211UL;
    size_t hash = FnvOffsetBasis;
    for (; *c; ++c)
    {
      hash *= FnvPrime;
      hash ^= *c;
    }
    return hash;
  }
};
template<>
struct HashT<const std::string>
{
  size_t operator()(const std::string &s) const {
    return HashT<const char *>()(s.c_str());
  }
};
template <typename T>
struct EqualsT
{
  bool operator()(const T &t1, const T &t2) const
  {
    assert(false);
    return t1 == t2;
  }
};

template <>
struct EqualsT<const char *>
{
  bool operator()(const char *L, const char *R) const
  {
    while (*L && *R && *L == *R)
    {
      ++L;
      ++R;
    }
    return *L == *R;
  }
};
template <>
struct EqualsT<const std::string>
{
  bool operator()(const std::string &L, const std::string &R) const
  {
    return EqualsT<const char *>()(L.c_str(), R.c_str());
  } 
};

template <typename Key, typename Value>
class Tuple
{
public:
  Key key;
  Value value;

  Tuple(const Key &k, const Value v) : key(k), value(v) {}
};

template <typename Key, typename Value>
class Bucket
{
public:
  Bucket *next;
  size_t hashValue;
  Tuple<Key, Value> tuple;

  Bucket(const Key &k, size_t h, const Value v)
      : next(nullptr), hashValue(h), tuple(k, v) {}

  ~Bucket() { delete next; }
};

template <typename Key, typename Value>
class HashTable
{
private:
  // Your code here.

  Bucket<Key, Value> **buckets = nullptr;
  size_t numberOfBuckets = 0;
  size_t numberOfElements = 0;

  friend class Iterator;
  friend class HashBlob;

public:
  Tuple<Key, Value> *Find(const Key k, const Value initialValue)
  {
    // Search for the key k and return a pointer to the
    // ( key, value ) entry.  If the key is not already
    // in the hash, add it with the initial value.

    // Your code here.
    size_t hash = HashT<Key>()(k);
    size_t index = hash % numberOfBuckets;
    Bucket<Key, Value> *cur = buckets[index];
    if (!cur)
    {
      buckets[index] = new Bucket<Key, Value>(k, hash, initialValue);
      ++numberOfElements;
      return &(buckets[index]->tuple);
    }
    while (cur->next)
    {
      if (cur->hashValue != hash)
        cur = cur->next;
      else if (!EqualsT<Key>()(cur->tuple.key, k))
        cur = cur->next;
      else
        return &cur->tuple;
    }
    if (cur->hashValue == hash && EqualsT<Key>()(cur->tuple.key, k))
    {
      return &cur->tuple;
    }
    else
    {
      cur->next = new Bucket<Key, Value>(k, hash, initialValue);
      ++numberOfElements;
      return &(cur->next->tuple);
    }
    assert(false);
    return nullptr;
  }

  Tuple<Key, Value> *Find(const Key k) const
  {
    // Search for the key k and return a pointer to the
    // ( key, value ) enty.  If the key is not already
    // in the hash, return nullptr.

    // Your code here.
    size_t hash = HashT<Key>()(k);
    size_t index = hash % numberOfBuckets;
    Bucket<Key, Value> *cur = buckets[index];
    if (!cur)
    {
      return nullptr;
    }
    while (cur->next)
    {
      if (cur->hashValue != hash)
        cur = cur->next;
      else if (!EqualsT<Key>()(cur->tuple.key, k))
        cur = cur->next;
      else
        return &cur->tuple;
    }
    if (cur->hashValue == hash && EqualsT<Key>()(cur->tuple.key, k))
    {
      return &cur->tuple;
    }
    else
    {
      return nullptr;
    }
    return nullptr;
  }

  bool Update(const Key &k, const Value newValue)
  {
    Tuple<Key, Value> *entry = Find(k);
    if (!entry)
    {
      return false;
    }
    entry->value = newValue;
    return true;
  }
  void Optimize(double loading = 1.1)
  {
    // Modify or rebuild the hash table as you see fit
    // to improve its performance now that you know
    // nothing more is to be added.

    size_t newNumberOfBuckets = static_cast<size_t>(numberOfElements / loading);
    if (newNumberOfBuckets == numberOfBuckets) return;
    
    Bucket<Key, Value>** newBuckets = new Bucket<Key, Value>*[newNumberOfBuckets]();

    for (size_t i = 0; i < numberOfBuckets; ++i) {
      Bucket<Key, Value>* current = buckets[i];
      buckets[i] = nullptr;

      while (current != nullptr) {
          Bucket<Key, Value>* nextNode = current->next; 

          size_t newIndex = current->hashValue % newNumberOfBuckets;

          current->next = newBuckets[newIndex];
          newBuckets[newIndex] = current;

          current = nextNode;
      }
    }

    delete[] buckets;

    buckets = newBuckets;
    numberOfBuckets = newNumberOfBuckets;
    
    /*
    HashTable<Key, Value> *t = new HashTable<Key, Value>(loading * numberOfElements);
    for (auto it = begin(); it != end(); ++it)
      t->Find(it->key, it->value);
    swap(t->numberOfBuckets, this->numberOfBuckets);
    swap(t->numberOfElements, this->numberOfElements);
    swap(t->buckets, this->buckets);
    delete t;
    */
  }

  // Your constructor may take as many default arguments
  // as you like.

  HashTable()
  {
    // Your code here.
    numberOfBuckets = 50000;
    buckets = new Bucket<Key, Value> *[numberOfBuckets]();
  }

  ~HashTable()
  {
    // Your code here.
    for (size_t i = 0; i < numberOfBuckets; ++i)
    {
      delete buckets[i];
    }
    delete[] buckets;
  }

  HashTable(size_t numberOfBuckets)
  {
    this->numberOfBuckets = numberOfBuckets;
    buckets = new Bucket<Key, Value> *[numberOfBuckets]();
  }

  class Iterator
  {
  private:
    friend class HashTable;

    // Your code here.
    HashTable *table;
    size_t bucket;
    Bucket<Key, Value> *b;

    Iterator(HashTable *table, size_t bucket, Bucket<Key, Value> *b)
    {
      // Your code here.
      this->table = table;
      this->bucket = bucket;
      this->b = b;
    }

  public:
    Iterator() : Iterator(nullptr, 0, nullptr) {}

    ~Iterator() {}

    Tuple<Key, Value> &operator*()
    {
      // Your code here.
      assert(b);
      return b->tuple;
    }

    Tuple<Key, Value> *operator->() const
    {
      // Your code here.
      assert(b);
      return &b->tuple;
    }

    // Prefix ++
    Iterator &operator++()
    {
      // Your code here.
      if (b && b->next)
      {
        b = b->next;
        return *this;
      }
      do
      {
        ++bucket;
      } while (bucket < table->numberOfBuckets && !table->buckets[bucket]);
      if (bucket != table->numberOfBuckets)
        b = table->buckets[bucket];
      else
        b = nullptr;
      return *this;
    }

    // Postfix ++
    Iterator operator++(int)
    {
      // Your code here.
      Iterator it = *this;
      ++(*this);
      return it;
    }

    bool operator==(const Iterator &rhs) const
    {
      // Your code here.
      return (table == rhs.table) && (bucket == rhs.bucket) && (b == rhs.b);
    }

    bool operator!=(const Iterator &rhs) const
    {
      // Your code here.
      return !(*this == rhs);
    }
  };

  Iterator begin()
  {
    // Your code here.
    if (buckets[0])
      return Iterator(this, 0, buckets[0]);
    else
      return ++Iterator(this, 0, nullptr);
  }

  Iterator end()
  {
    // Your code here.
    return Iterator(this, numberOfBuckets, nullptr);
  }
};
