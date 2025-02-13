#pragma once

// HashBlob, a serialization of a HashTable into one contiguous
// block of memory, possibly memory-mapped to a HashFile.

// Nicole Hamilton  nham@umich.edu

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <cstring>
#include <cstdint>
#include <malloc.h>
#include <unistd.h>
#include <sys/mman.h>

#include "HashTable.h"
#include "Common.h"

using Hash = HashTable<const char *, size_t>;
using Pair = Tuple<const char *, size_t>;
using HashBucket = Bucket<const char *, size_t>;

static const size_t Unknown = 0;

size_t RoundUp(size_t length, size_t boundary)
{
  // Round up to the next multiple of the boundary, which
  // must be a power of 2.

  static const size_t oneless = boundary - 1,
                      mask = ~(oneless);
  return (length + oneless) & mask;
}

struct SerialTuple
{
  // This is a serialization of a HashTable< char *, size_t >::Bucket.
  // One is packed up against the next in a HashBlob.

  // Since this struct includes size_t and uint32_t members, we'll
  // require that it be sizeof( size_t ) aligned to avoid unaligned
  // accesses.

public:
  // SerialTupleLength = 0 is a sentinel indicating
  // this is the last SerialTuple chained in this list.
  // (Actual length is not given but not needed.)

  size_t Length, Value;
  size_t HashValue;

  // The Key will be a C-string of whatever length.
  char Key[Unknown];

  // Calculate the bytes required to encode a HashBucket as a
  // SerialTuple.

  static size_t BytesRequired(const HashBucket *b)
  {
    // Your code here.
    size_t ans = 0;
    for (; b; b = b->next)
    {
      ans += sizeof(SerialTuple);
    }
    return ans;
  }

  // Write the HashBucket out as a SerialTuple in the buffer,
  // returning a pointer to one past the last character written.

  static char *Write(char *buffer, char *bufferEnd,
                     const HashBucket *b)
  {
    // Your code here.

    return nullptr;
  }
};

class HashBlob
{
  // This will be a hash specifically designed to hold an
  // entire hash table as a single contiguous blob of bytes.
  // Pointers are disallowed so that the blob can be
  // relocated to anywhere in memory

  // The basic structure should consist of some header
  // information including the number of buckets and other
  // details followed by a concatenated list of all the
  // individual lists of tuples within each bucket.

public:
  // Define a MagicNumber and Version so you can validate
  // a HashBlob really is one of your HashBlobs.

  size_t MagicNumber,
      Version,
      BlobSize,
      NumberOfBuckets,
      Buckets[Unknown];

  // The SerialTuples will follow immediately after.

  const SerialTuple *Find(const char *key) const
  {
    // Search for the key k and return a pointer to the
    // ( key, value ) entry.  If the key is not found,
    // return nullptr.

    // Your code here.

    return nullptr;
  }

  static size_t BytesRequired(const Hash *hashTable)
  {
    // Calculate how much space it will take to
    // represent a HashTable as a HashBlob.
    static const size_t HEADERSIZE = sizeof(size_t) + sizeof(size_t);
    // Need space for the header + buckets +
    // all the serialized tuples.

    // Your code here.

    return 0;
  }

  // Write a HashBlob into a buffer, returning a
  // pointer to the blob.

  static HashBlob *Write(HashBlob *hb, size_t bytes,
                         const Hash *hashTable)
  {
    // Your code here.

    return nullptr;
  }

  // Create allocates memory for a HashBlob of required size
  // and then converts the HashTable into a HashBlob.
  // Caller is responsible for discarding when done.

  // (No easy way to override the new operator to create a
  // variable sized object.)

  static HashBlob *Create(const Hash *hashTable)
  {
    // Your code here.
    const size_t size = HashBlob::BytesRequired(hashTable);
    HashBlob *mem = (HashBlob *)malloc(size);

    return nullptr;
  }

  // Discard

  static void Discard(HashBlob *blob)
  {
    // Your code here.
  }
};

class HashFile
{
private:
  HashBlob *blob;

  size_t FileSize(int f)
  {
    struct stat fileInfo;
    fstat(f, &fileInfo);
    return fileInfo.st_size;
  }

public:
  const HashBlob *Blob()
  {
    return blob;
  }

  HashFile(const char *filename)
  {
    // Open the file for reading, map it, check the header,
    // and note the blob address.

    // Your code here.
    int fd = open(filename, O_RDONLY);
    assert(fd != -1);
    const size_t fileSize = FileSize(fd);
    const uint8_t *bytes = (uint8_t *)mmap(nullptr, fileSize, PROT_READ, MAP_PRIVATE, fd, 0);

    munmap(bytes, fileSize);
  }

  HashFile(const char *filename, const Hash *hashtable)
  {
    // Open the file for write, map it, write
    // the hashtable out as a HashBlob, and note
    // the blob address.

    // Your code here.
    int fd = open(filename, O_WRONLY);
    assert(fd != -1);
    HashBlob *b = HashBlob::Create(hashtable);
    size_t fileSize = HashBlob::BytesRequired(hashtable);
    uint8_t *bytes = (uint8_t *)mmap(nullptr, fileSize, PROT_WRITE, MAP_PRIVATE, fd, 0);

    munmap(bytes, fileSize);
  }

  ~HashFile()
  {
    // Your code here.
  }
};
