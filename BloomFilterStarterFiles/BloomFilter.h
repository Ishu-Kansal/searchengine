   #ifndef BLOOMFILTER_H
   #define BLOOMFILTER_H
   #include <vector>
   #include <cmath>
   #include <string>
   #include <string.h>
   #include <cstring>
   #include <openssl/md5.h>
   #include <unistd.h>

   class Bloomfilter
      {
      public:
         Bloomfilter( int num_objects, double false_positive_rate )
            {
            // Determine the size of bits of our data vector, and resize.
            sizeInBits = -ceil((num_objects * log(false_positive_rate)) / (log(2) * log(2)));
            bloomFilter.reserve(sizeInBits);
            // Determine number of hash functions to use.
            numHashFuncs = round((sizeInBits/num_objects) * log(2));
            }

         Bloomfilter(int handle)
         {

            read(handle, reinterpret_cast<char*>(&sizeInBits), sizeof(sizeInBits));
            read(handle, reinterpret_cast<char*>(&numHashFuncs), sizeof(numHashFuncs));
            
            size_t numBytes = (sizeInBits + 7) / 8;
            std::vector<unsigned char> bytes(numBytes);
            read(handle, reinterpret_cast<char*>(bytes.data()), numBytes);
            int bitIndex = 0;
            bloomFilter.reserve(sizeInBits);
            for (size_t i = 0; i < numBytes; i++)
            {
               for (int bit = 0; bit < 8; bit++)
               {
                  if (i * 8 + bit >= sizeInBits)
                     break;
                  bool currentBit = (bytes[i] >> bit) & 1;
                  bloomFilter[bitIndex++] = currentBit;
               }
            }
         }
         
         void writeBFtoFile(int handle)
         {
            write(handle, reinterpret_cast<const char*>(&sizeInBits), sizeof(sizeInBits));
            write(handle, reinterpret_cast<const char*>(&numHashFuncs), sizeof(numHashFuncs));
            
            // Pack bits into a byte then add to vector
            std::vector<unsigned char> bytes;
            int bitCount = 0;
            unsigned char cur = 0;
            for (bool bit : bloomFilter)
            {
               cur |= bit << bitCount;
               bitCount ++;
               if (bitCount == 8)
               {
                  bytes.push_back(cur);
                  cur = 0;
                  bitCount = 0;
               }
            }
            // Add remaining bits.
            if(bitCount > 0)
            {
               bytes.push_back(cur);
            }
            write(handle, reinterpret_cast<const char*>(bytes.data()), bytes.size());
         }
         void insert( const std::string &s)
            {
            // Hash the string into two unique hashes.
            std::pair<uint64_t, uint64_t> hashVals = hash(s);
            uint64_t hash1 = hashVals.first;
            uint64_t hash2 = hashVals.second;

            // Use double hashing to get unique bit, and repeat for each hash function.

            for (int i = 0;i < numHashFuncs; ++i)
            {
               bloomFilter[(hash1 + i * hash2) % sizeInBits] = true;
            }

            }

         bool contains( const std::string &s )
            {
            // Hash the string into two unqiue hashes.

            // Use double hashing to get unique bit, and repeat for each hash function.
            // If bit is false, we know for certain this unique string has not been inserted.

            // If all bits were true, the string is likely inserted, but false positive is possible.

            // This line is for compiling, replace this with own code.
            std::pair<uint64_t, uint64_t> hashVals = hash(s);
            uint64_t hash1 = hashVals.first;
            uint64_t hash2 = hashVals.second;
            for (int i = 0; i < numHashFuncs; ++i)
            {
               if (!bloomFilter[(hash1 + i * hash2) % sizeInBits]) return false;
            }
            return true;
            }

      private:
         // Add any private member variables that may be neccessary.
         uint64_t sizeInBits;
         uint64_t numHashFuncs;
         std::vector<bool> bloomFilter;
         std::pair<uint64_t, uint64_t> hash( const std::string &datum )
            {
            //Use MD5 to hash the string into one 128 bit hash, and split into 2 hashes.
            unsigned char digest[MD5_DIGEST_LENGTH];
            MD5(reinterpret_cast<const unsigned char *>(datum.c_str()), datum.length(), digest);

            uint64_t hash1, hash2;
               
            std::memcpy(&hash1, digest, 8);
            std::memcpy(&hash2, digest + 8, 8);

            return {hash1, hash2};
            }
      };

   #endif