#pragma once

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <cstring>
#include <cstdint>
#include <unistd.h>
#include <sys/mman.h>

#include<memory>
#include <unordered_map>

#include "../../HashTable/HashTableStarterFiles/HashBlob.h"
#include "../../HashTable/HashTableStarterFiles/HashTable.h"
#include "../../utils/utf_encoding.h"
#include "IndexFile.h"
#include "Index.h"

constexpr size_t ENTRY_SIZE = 16;
constexpr size_t URL_ENTRY_SIZE = 8;
constexpr size_t NUM_ENTRIES_SIZE_BYTES = 1;
constexpr size_t URL_ENTRY_HEADER_BYTES = 1;
constexpr size_t URL_ENTRY_RANK_BYTES = 1;
typedef size_t Location;

class FileDescriptor {
    public:
        explicit FileDescriptor(int fd) : fd_(fd) {}
        ~FileDescriptor() { 
            if (fd_ != -1) 
                close(fd_); 
        }
        int get() const { return fd_; }
        FileDescriptor(const FileDescriptor&) = delete;
        FileDescriptor& operator=(const FileDescriptor&) = delete;
    private:
        int fd_ = -1;
};

class MappedMemory {
    public:
        MappedMemory(void* ptr, size_t size) : ptr_(ptr), size_(size) {}
        ~MappedMemory() { 
            if (ptr_ && ptr_ != MAP_FAILED) 
                munmap(ptr_, size_); 
        }
        void* get() const { return ptr_; }
        size_t size() const { return size_; }
        MappedMemory(const MappedMemory&) = delete;
        MappedMemory& operator=(const MappedMemory&) = delete;
    private:
        void* ptr_;
        size_t size_;
};
struct SeekObj {
    Location offset;
    Location location;
    Location index;
};
struct MappedFile {
    void * map;
    size_t fileSize;
    MappedFile(void * map_, size_t fileSize_) : map(map_), fileSize(fileSize_) {}
};

class IndexFileReader {
private:

    std::vector<std::unique_ptr<MappedMemory>> mappedFiles;
    size_t FileSize(int f)
    {
        struct stat fileInfo;
        if (fstat(f, &fileInfo) == -1) {
            return 0;
        }
        return fileInfo.st_size;
    }
    static bool readUint64_t(const uint8_t*& currentPtr, const uint8_t* endPtr, uint64_t& value) 
    {
        if (currentPtr == nullptr || endPtr == nullptr || currentPtr + sizeof(uint64_t) > endPtr) {
            return false;
        }
        memcpy(&value, currentPtr, sizeof(uint64_t));
        currentPtr += sizeof(uint64_t);
        return true;
    }
public:
    IndexFileReader() = delete;
    IndexFileReader(uint32_t numChunks) : mappedFiles() 
    {
        mappedFiles.resize(numChunks);
        for (unsigned i = 0; i < numChunks; ++i)
        {
            char indexFilename[32];
            snprintf(indexFilename, sizeof(indexFilename), "IndexChunk_%05u", i);
            int fd = open(indexFilename, O_RDONLY, 0666);
            if (fd == -1) 
            {
                fprintf(stderr, "WARNING: Couldn't open index file '%s': %s\n", indexFilename, strerror(errno));
                continue; 
            }
            FileDescriptor file(fd);
            size_t fileSize = FileSize(file.get());
            if (fileSize == 0) 
            {
                fprintf(stderr, "WARNING: Index file '%s' is empty or stat failed.\n", indexFilename);
                continue;
            }
            void* map = mmap(NULL, fileSize, PROT_READ, MAP_PRIVATE, file.get(), 0); 
            if (map == MAP_FAILED) 
            {
                fprintf(stderr, "WARNING: Couldn't mmap index file '%s': %s\n", indexFilename, strerror(errno));
                continue;
            }
            mappedFiles[i] = std::make_unique<MappedMemory>(map, fileSize);
        }
    }

    std::unique_ptr<SeekObj> Find(const std::string& word, Location target, uint32_t chunkNum) const {
        if (chunkNum >= mappedFiles.size() || !mappedFiles[chunkNum]) 
        {
            fprintf(stderr, "DEBUG: invalid chunkNum or unmapped chunk: %u\n", chunkNum);
            return nullptr;
        }
        // Finds word offset using HashFile
        char hashFilename[32];
        snprintf(hashFilename, sizeof(hashFilename), "HashFile_%05u", chunkNum);
        HashFile hashFile(hashFilename);
        const HashBlob *hashblob = hashFile.Blob();
        if (!hashblob) 
        {
            fprintf(stderr, "DEBUG: HashBlob not found for chunk %u\n", chunkNum);
            return nullptr;
        }

        const SerialTuple * tup = hashblob->Find(word.c_str());
        if (!tup) 
        {
            fprintf(stderr, "DEBUG: '%s' not found in hashblob for chunk %u\n", word.c_str(), chunkNum);
            return nullptr;
        }

        // Get mapped file
        const void* mapPtr = mappedFiles[chunkNum]->get();
        size_t fileSize = mappedFiles[chunkNum]->size();
        const uint8_t* fileStart = static_cast<const uint8_t*>(mapPtr);
        const uint8_t* fileEnd = fileStart + fileSize;

        Location offsetIntoFile = tup->Value; // Start of posting list
        if (offsetIntoFile >= fileSize) 
        {
            // shouldn't happen
            fprintf(stderr, "ERROR: Offset into file (%zu) is out of bounds (%zu) for word '%s' in chunk %u.\n", offsetIntoFile, fileSize, word.c_str(), chunkNum);
            return nullptr;
        } 

        uint8_t numEntriesSize = fileStart[offsetIntoFile];
        size_t tableIndex = (target + 1) >> BLOCK_OFFSET_BITS;
        uint64_t numEntries = 0;

        // Start of seek table
        const uint8_t* seekTable = fileStart + offsetIntoFile + 1;
     
        if (numEntriesSize) 
        {
            if (seekTable + numEntriesSize > fileEnd) 
            {
                fprintf(stderr, "ERROR: Not enough space for numEntries varint encoding.\n");
                return nullptr;
            }
            const uint8_t* temp = fileStart + offsetIntoFile + 1;
            decodeVarint(temp, numEntries);
        }
        if (numEntriesSize && tableIndex != 0)
        {
            // Gets seek table entry at target index
            const uint8_t* entryPtr = seekTable + ((tableIndex - 1) * ENTRY_SIZE) + numEntriesSize;

            uint64_t entryOffset;
            uint64_t entryLocation;
            
            if (!readUint64_t(entryPtr, fileEnd, entryOffset)) { nullptr; }
            if (!readUint64_t(entryPtr, fileEnd, entryLocation)) { nullptr; }

            uint64_t index = (tableIndex * BLOCK_SIZE) - 1;
            // Returns if target was an entry in seek table
            if (entryLocation == target) 
            {

                auto obj = std::make_unique<SeekObj>();
                obj->offset = entryOffset;
                obj->location = entryLocation;
                obj->index = index;
                return obj;
            }
            // Need to linearly scan to find first entry with location >= target
            uint64_t currentLocation = entryLocation;
            uint64_t currentOffset = entryOffset;
            const uint8_t* postPtr = seekTable + (ENTRY_SIZE * numEntries) + entryOffset + numEntriesSize;
            while (currentLocation < target || target == 0)
            {
                ++index;
                uint64_t delta = 0;
                // decode varint automatically moves the buffer ahead
                postPtr = decodeVarint(postPtr, delta);
                currentLocation += delta;
                // current offset is the number of bytes into this posting list we are. I think
                currentOffset += SizeOf(delta);
                if (currentLocation >= target)
                {
                    auto obj = std::make_unique<SeekObj>();
                    obj->offset = currentOffset;
                    obj->location = currentLocation;
                    obj->index = index;
                    return obj;
                }
            }
        }
        else
        {
            // Linearly scan if we don't have a seek table
            const uint8_t* varintBuf = fileStart + offsetIntoFile + 1 + (ENTRY_SIZE * numEntries) + numEntriesSize;
            uint64_t currentLocation = 0;
            uint64_t currentOffset = 0;
            uint64_t index = 0;
            
            while (currentLocation < target || target == 0)
            {
                uint64_t delta = 0;
                varintBuf = decodeVarint(varintBuf, delta);
                currentLocation += delta;
                currentOffset += SizeOf(delta);
                if (currentLocation >= target)
                {
                    auto obj = std::make_unique<SeekObj>();
                    obj->offset = currentOffset;
                    obj->location = currentLocation;
                    obj->index = index;
                    return obj;
                }
                index++;
            }
            }

        return nullptr;;
    }

    std::unique_ptr<Doc> FindUrl(uint32_t index, uint32_t chunkNum)
    {
        if (!mappedFiles[chunkNum]) return nullptr;;

        const void* mapPtr = mappedFiles[chunkNum]->get();
        size_t fileSize = mappedFiles[chunkNum]->size();
        const uint8_t* fileStart = static_cast<const uint8_t*>(mapPtr);
        const uint8_t* fileEnd = fileStart + fileSize;

        uint8_t numEntriesSize = fileStart[0];
        size_t tableIndex = (index + 1) >> BLOCK_OFFSET_BITS;
        uint64_t numEntries = 0;
        if (numEntriesSize) 
        {
            const uint8_t* temp = fileStart + 1;
            decodeVarint(temp, numEntries);
        }
        if (numEntriesSize && tableIndex != 0)
        {
            const uint8_t* seekTable = fileStart + 1;
            const uint8_t* entryPtr = seekTable + ((tableIndex - 1) * URL_ENTRY_SIZE)+ numEntriesSize;

            uint64_t entryOffset;
            
            if (!readUint64_t(entryPtr, fileEnd, entryOffset)) { return nullptr;; }

            const uint8_t* urlPtr = seekTable + (URL_ENTRY_SIZE * numEntries) + entryOffset + numEntriesSize;
            uint64_t urlIndex = (tableIndex * BLOCK_SIZE) - 1;
            while (urlIndex < index)
            {
                uint64_t bytesToSkip = *urlPtr + 2;
                urlPtr += bytesToSkip;
                ++urlIndex;
            }
            if (urlIndex == index)
            {
                auto obj = std::make_unique<Doc>();
                uint8_t urlLen = *urlPtr;
                obj->url = std::string(reinterpret_cast<const char*>(urlPtr + 1), urlLen);
                urlPtr += urlLen + 1;
                obj->staticRank = *urlPtr;
                
                return obj;
            }
        }
        else
        {
            uint64_t urlIndex = 0;
            const uint8_t* urlPtr = fileStart + (URL_ENTRY_SIZE * numEntries) + 1 + numEntriesSize;
            
            while (urlIndex < index)
            {
                uint64_t bytesToSkip = *urlPtr + 2;
                urlPtr += bytesToSkip;
                ++urlIndex;
            }
            if (urlIndex == index)
            {
                auto obj = std::make_unique<Doc>();
                uint8_t urlLen = *urlPtr;
                obj->url = std::string(reinterpret_cast<const char*>(urlPtr + 1), urlLen);
                urlPtr += urlLen + 1;
                obj->staticRank = *urlPtr;
                
                return obj;
            }
        }
        return nullptr;
    }
};
