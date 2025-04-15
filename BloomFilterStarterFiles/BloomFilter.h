#ifndef BLOOMFILTER_H
#define BLOOMFILTER_H
#include <openssl/md5.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cassert>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

class Bloomfilter {
 public:
  Bloomfilter(uint64_t num_objects, double false_positive_rate) {
    // Determine the size of bits of our data vector, and resize.
    sizeInBits =
        -ceil((num_objects * log(false_positive_rate)) / (log(2) * log(2)));
    bloomFilter.reserve(sizeInBits);
    // Determine number of hash functions to use.
    numHashFuncs = round((sizeInBits / num_objects) * log(2));
  }

  Bloomfilter(int handle) {
    uint64_t header[2];
    read(handle, header, sizeof(header));
    sizeInBits = header[0];
    numHashFuncs = header[1];
    /*read(handle, reinterpret_cast<char *>(&sizeInBits), sizeof(sizeInBits));
    read(handle, reinterpret_cast<char *>(&numHashFuncs),
    sizeof(numHashFuncs));*/

    size_t numBytes = (sizeInBits + 7) / 8;

    const char *bytes = static_cast<const char *>(mmap(
        nullptr, sizeof(header) + numBytes, PROT_READ, MAP_PRIVATE, handle, 0));

    assert(bytes != MAP_FAILED);
    bytes += sizeof(header);

    // no need to memory allocate vector; instead just mmap file for
    // numBytes
    /*std::vector<unsigned char> bytes(numBytes);
    read(handle, reinterpret_cast<char *>(bytes.data()), numBytes);*/
    bloomFilter.reserve(sizeInBits);

    for (size_t i = 0; i < sizeInBits; ++i) {
      bloomFilter[i] = (bytes[i / 8] >> (i % 8)) & 1;
    }
    munmap((char *)bytes - sizeof(header), sizeof(header) + numBytes);
  }

  void writeBFtoFile(int handle) {
    const uint64_t header[] = {sizeInBits, numHashFuncs};
    write(handle, header, sizeof(header));

    // Pack bits into a byte then add to vector
    size_t numBytes = (sizeInBits + 7) / 8;
    ftruncate(handle, numBytes + sizeof(header));
    char *bytes = static_cast<char *>(mmap(nullptr, sizeof(header) + numBytes,
                                           PROT_READ | PROT_WRITE, MAP_PRIVATE,
                                           handle, 0));
    if (bytes == MAP_FAILED) {
      perror("");
    }
    assert(bytes != MAP_FAILED);

    bytes += sizeof(header);
    for (size_t i = 0; i < sizeInBits; i++) {
      if (bloomFilter[i]) bytes[i / 8] |= (1 << (i % 8));
    }

    munmap(bytes - sizeof(header), sizeof(header) + numBytes);
    /*std::vector<unsigned char> bytes(numBytes, 0);
    for (size_t i = 0; i < sizeInBits; i++) {
      if (bloomFilter[i]) bytes[i / 8] |= (1 << (i % 8));
    }
    write(handle, reinterpret_cast<const char *>(bytes.data()), bytes.size());*/
  }
  void insert(const std::string &s) {
    // Hash the string into two unique hashes.
    std::pair<uint64_t, uint64_t> hashVals = hash(s);
    uint64_t hash1 = hashVals.first;
    uint64_t hash2 = hashVals.second;

    // Use double hashing to get unique bit, and repeat for each hash function.

    for (int i = 0; i < numHashFuncs; ++i) {
      bloomFilter[(hash1 + i * hash2) % sizeInBits] = true;
    }
  }

  bool contains(const std::string &s) {
    // Hash the string into two unqiue hashes.

    // Use double hashing to get unique bit, and repeat for each hash function.
    // If bit is false, we know for certain this unique string has not been
    // inserted.

    // If all bits were true, the string is likely inserted, but false positive
    // is possible.

    // This line is for compiling, replace this with own code.
    std::pair<uint64_t, uint64_t> hashVals = hash(s);
    uint64_t hash1 = hashVals.first;
    uint64_t hash2 = hashVals.second;
    for (int i = 0; i < numHashFuncs; ++i) {
      if (!bloomFilter[(hash1 + i * hash2) % sizeInBits]) return false;
    }
    return true;
  }

 private:
  // Add any private member variables that may be neccessary.
  uint64_t sizeInBits;
  uint64_t numHashFuncs;
  std::vector<bool> bloomFilter;
  std::pair<uint64_t, uint64_t> hash(const std::string &datum) {
    // Use MD5 to hash the string into one 128 bit hash, and split into 2
    // hashes.
    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5(reinterpret_cast<const unsigned char *>(datum.c_str()), datum.length(),
        digest);

    uint64_t hash1, hash2;

    std::memcpy(&hash1, digest, 8);
    std::memcpy(&hash2, digest + 8, 8);

    return {hash1, hash2};
  }
};

#endif