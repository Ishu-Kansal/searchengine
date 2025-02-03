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

void insertionSort(Pair **first, Pair **last, Pair *current, int N)
{
  if (last - first == N && current->value <= (*(last - 1))->value)
    return;
  Pair **marker = first;
  while (marker != last && (*marker)->value >= current->value)
    ++marker;
  for (auto it = last < first + N - 1 ? last : first + N - 1; it > marker; --it)
  {
    *it = *(it - 1);
  }
  *marker = current;
}

Pair **TopN(Hash *hashtable, int N)
{
  // Find the top N pairs based on the values and return
  // as a dynamically-allocated array of pointers.  If there
  // are less than N pairs in the hash, remaining pointers
  // will be null.

  // Your code here.
  Pair **topN = new Pair *[N]();
  int found = 0;
  const auto last = hashtable->end();
  for (auto it = hashtable->begin(); it != last; ++it)
  {
    insertionSort(topN, topN + found, &*it, N);
    found += found < N;
  }
  return topN;
}