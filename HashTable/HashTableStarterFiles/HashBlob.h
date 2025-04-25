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
#include <unistd.h>
#include <sys/mman.h>

#include "HashTable.h"

#ifdef DEBUG
#define DEBUG_MSG(stream) std::cout << "[DEBUG] " << stream << std::endl
#else
#define DEBUG_MSG(stream) ((void)0)
#endif

using Hash = HashTable<const std::string, size_t>;
using Pair = Tuple<const std::string, size_t>;
using HashBucket = Bucket<const std::string, size_t>;

static const size_t Unknown = 0;

inline size_t RoundUp(size_t length, size_t boundary)
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
      if (!b || b->tuple.key.empty())
      {
         return 0;
      }
      size_t base = sizeof(Length) + sizeof(Value) + sizeof(size_t);
      size_t keyLen = b->tuple.key.size() + 1;
      
      size_t total = base + keyLen;
      return RoundUp(total, sizeof(size_t));
   }

   // Write the HashBucket out as a SerialTuple in the buffer,
   // returning a pointer to one past the last character written.

   static char *Write(char *buffer, char *bufferEnd,
                      const HashBucket *b)
   {
      // Error checks
      if (!b)
      {
         return buffer;
      }
      size_t len = BytesRequired(b);
      if (buffer + len > bufferEnd)
      {
         return buffer;
      }

      // Writes total len to buffer
      std::memcpy(buffer, &len, sizeof(len));
      buffer += sizeof(len);

      // Writes value to buffer
      std::memcpy(buffer, &(b->tuple.value), sizeof(b->tuple.value));
      buffer += sizeof(b->tuple.value);

      // Writes hash value to buffer
      std::memcpy(buffer, &(b->hashValue), sizeof(b->hashValue));
      buffer += sizeof(b->hashValue);

      // Writes key to buffer
      size_t keyLen = b->tuple.key.size() + 1;
      std::memcpy(buffer, b->tuple.key.c_str(), keyLen);
      buffer += keyLen;
      size_t total = keyLen + 24;
      // Aligns buffer
      size_t padding = len - total;
      if (padding > 0)
      {
         std::memset(buffer, 0, padding);
         buffer += padding;
      }

      return buffer;
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
      
       const SerialTuple *Find(const char *key) const
       {
          size_t hashValue = HashT<const char *>()(key);
          size_t bucketOffset = Buckets[hashValue % NumberOfBuckets];
          const char *blobEnd = reinterpret_cast<const char *>(this) + BlobSize;
          const char *bucket = reinterpret_cast<const char *>(this) + bucketOffset;
    
          while (bucket < blobEnd)
          {
             const SerialTuple *tuple = reinterpret_cast<const SerialTuple *>(bucket);
             if (tuple->Length == 0)
             {
                break;
             }
             if (tuple->HashValue == hashValue && strcmp(tuple->Key, key) == 0)
             {
                return tuple;
             }
             bucket += tuple->Length;
          }
          return nullptr;
       }
    
   // The SerialTuples will follow immediately after.


   static size_t BytesRequired(const Hash *hashTable)
   {
      // Calculate how much space it will take to
      // represent a HashTable as a HashBlob.

      // Need space for the header + buckets +
      // all the serialized tuples.
      size_t total = 4 * sizeof(size_t);
      total += hashTable->numberOfBuckets * sizeof(size_t);
      for (size_t i = 0; i < hashTable->numberOfBuckets; ++i)
      {
         const HashBucket *bucket = hashTable->buckets[i];
         while (bucket)
         {
            total += SerialTuple::BytesRequired(bucket);
            bucket = bucket->next;
         }
      }
      return RoundUp(total, sizeof(size_t));
      // Your code here.
   }

   // Write a HashBlob into a buffer, returning a
   // pointer to the blob.

   static HashBlob *Write(HashBlob *hb, size_t bytes,
                          const Hash *hashTable)
   {
      // Your code here.
      size_t hashTableBytes = BytesRequired(hashTable);
      if (hashTableBytes > bytes)
      {
         return nullptr;
      }
      size_t headerSize = 4 * sizeof(size_t) + hb->NumberOfBuckets * sizeof(size_t);
      char *buffer = reinterpret_cast<char *>(hb);
      char *cur = buffer + headerSize;
      char *bufferEnd = buffer + bytes;

      size_t *bucketOffsets = hb->Buckets;

      for (size_t i = 0; i < hashTable->numberOfBuckets; ++i)
      {
         bucketOffsets[i] = cur - buffer;
         HashBucket *bucket = hashTable->buckets[i];
         while (bucket)
         {
            cur = SerialTuple::Write(cur, bufferEnd, bucket);
            bucket = bucket->next;
         }
      }

      return hb;
   }

   // Create allocates memory for a HashBlob of required size
   // and then converts the HashTable into a HashBlob.
   // Caller is responsible for discarding when done.

   // (No easy way to override the new operator to create a
   // variable sized object.)

   static HashBlob *Create(const Hash *hashTable)
   {

      size_t bytes = BytesRequired(hashTable);

      void *mem = operator new(bytes);
      std::memset(mem, 0, bytes);
      HashBlob *hb = new (mem) HashBlob();
      hb->MagicNumber = 1;
      hb->Version = 1;
      hb->BlobSize = bytes;
      hb->NumberOfBuckets = hashTable->numberOfBuckets;
      hb = Write(hb, bytes, hashTable);
      return hb;
   }

   // Discard

   static void Discard(HashBlob *blob)
   {
      // Your code here.
      operator delete(blob);
   }
};

class HashFile
{
private:
   HashBlob *blob;
   int fileDescrip;
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

   HashFile(const char *filename) : blob(nullptr)
   {
      // Open the file for reading, map it, check the header,
      // and note the blob address.

      // Your code here.
      fileDescrip = open(filename, O_RDONLY);
      size_t fileSize = FileSize(fileDescrip);
      void *map = mmap(nullptr, fileSize, PROT_READ, MAP_SHARED, fileDescrip, 0);

      blob = reinterpret_cast<HashBlob *>(map);
   }

   HashFile(const char *filename, const Hash *hashtable) : blob(nullptr)
   {
      // Open the file for write, map it, write
      // the hashtable out as a HashBlob, and note
      // the blob address.
      // Your code here.
      HashBlob *tempBlob = HashBlob::Create(hashtable);
      if (!tempBlob) {
         std::cerr << "Failed to create HashBlob in memory" << std::endl;

         return;
      }
      fileDescrip = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0666);

      size_t requiredSize = HashBlob::BytesRequired(hashtable);
      ftruncate(fileDescrip, requiredSize);
      void *map = mmap(nullptr, requiredSize, PROT_WRITE, MAP_SHARED, fileDescrip, 0);
      if (map == MAP_FAILED) 
      {
         HashBlob::Discard(tempBlob);
         close(fileDescrip);
         return;
      }   
      std::memcpy(map, blob, requiredSize);
      HashBlob::Discard(tempBlob);
      tempBlob = nullptr;

      blob = reinterpret_cast<HashBlob *>(map);
   }

   ~HashFile()
   {
      if (blob)
      {
         munmap(blob, blob->BlobSize);
      }
      if (fileDescrip >= 0)
      {
         close(fileDescrip);
      }
   }
};
