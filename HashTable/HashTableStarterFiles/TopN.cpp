// TopN.cpp
// Nicole Hamilton nham@umich.edu

// Given a hashtable, return a dynamically-allocated array
// of pointers to the top N pairs based on their values.
// Caller is responsible for deleting the array.

// Individual pairs are to be C-strings as keys and size_t's
// as values.

#include "TopN.h"

#include "HashTable.h"

using namespace std;

using Hash = HashTable<const char *, size_t>;
using Pair = Tuple<const char *, size_t>;

Pair **TopN(Hash *hashtable, int N) {
  // Find the top N pairs based on the values and return
  // as a dynamically-allocated array of pointers.  If there
  // are less than N pairs in the hash, remaining pointers
  // will be null.

  // Your code here.
  Pair **topN = new Pair *[N];
  for (int i = 0; i < N; ++i) topN[i] = nullptr;
  int found = 0;
  for (auto it = hashtable->begin(); it != hashtable->end(); ++it) {
    if (found < N) {
      topN[found] = &*it;
      ++found;
    } else {
      for (int i = 0; i < N; ++i) {
        if (it->value > topN[i]->value) {
          topN[i] = &*it;
          break;
        }
      }
    }
  }
  return topN;
}
