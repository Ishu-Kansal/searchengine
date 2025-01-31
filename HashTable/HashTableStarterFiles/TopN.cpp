// TopN.cpp
// Nicole Hamilton nham@umich.edu

// Given a hashtable, return a dynamically-allocated array
// of pointers to the top N pairs based on their values.
// Caller is responsible for deleting the array.

// Individual pairs are to be C-strings as keys and size_t's
// as values.

#include "TopN.h"

#include "HashTable.h"
#include <vector>
#include <algorithm>
#include <queue>

using namespace std;

using Hash = HashTable<const char *, size_t>;
using Pair = Tuple<const char *, size_t>;

struct PairComparator
{
  bool operator()(const Pair *a, const Pair *b)
  {
    return a->value > b->value;
  }
};

Pair **TopN(Hash *hashtable, int N)
{
  // Find the top N pairs based on the values and return
  // as a dynamically-allocated array of pointers.  If there
  // are less than N pairs in the hash, remaining pointers
  // will be null.
  std::priority_queue<Pair *, std::vector<Pair *>, PairComparator> minHeap;

  // Your code here.
  auto end = hashtable->end();
  for (auto it = hashtable->begin(); it != end; ++it)
  {
    if (minHeap.size() < N)
    {
      minHeap.push(&*it);
    }
    else if (it->value > minHeap.top()->value)
    {
      minHeap.pop();
      minHeap.push(&*it);
    }
  }

  Pair **topN = new Pair *[N]();
  int index = 0;
  while (!minHeap.empty())
  {
    topN[index++] = minHeap.top();
    minHeap.pop();
  }
  std::reverse(topN, topN + N);
  return topN;
}