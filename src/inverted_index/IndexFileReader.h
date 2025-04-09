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

constexpr size_t BLOCK_OFFSET_BITS = 13; // 2^13 or 8192
constexpr size_t BLOCK_SIZE = 1 << BLOCK_OFFSET_BITS;
constexpr size_t ENTRY_SIZE = 16;
typedef size_t Location;

struct SeekObj {
    Location offset;
    Location location;
};
struct MappedFile {
    void * map;
    size_t fileSize;
    MappedFile() = delete;
    MappedFile(void * map_, size_t fileSize_) : map(map_), fileSize(fileSize_) {}
};
class IndexFileReader {
private:

    std::vector<bool> mappedFileVec;
    std::unordered_map<int, MappedFile> mappedFilemap;
    size_t FileSize(int f)
    {
        struct stat fileInfo;
        if (fstat(f, &fileInfo) == -1) {
            return 0;
        }
        return fileInfo.st_size;
    }
public:
    IndexFileReader() = delete;
    IndexFileReader(uint32_t numChunks) : mappedFileVec(numChunks, 0)
    {
        for (unsigned i = 0; i < numChunks; ++i)
        {
            char indexFilename[32];
            snprintf(indexFilename, sizeof(indexFilename), "IndexChunk_%05u", i);
            int fd = -1;    
            fd = open(indexFilename, O_RDWR, 0666);
            if (fd == -1) continue; 
            size_t fileSize = FileSize(fd);
            if (fileSize == 0) 
            {
                close(fd);
                continue;
            }
            void * map = mmap(NULL, fileSize, PROT_READ, MAP_PRIVATE, fd, 0); 
            if (map == MAP_FAILED) 
            {
                close(fd);
                continue;
            }
            mappedFileVec[i] = 1;
            mappedFilemap[i] = MappedFile(map, fileSize);            
            close(fd);
        }
    }
    ~IndexFileReader()
    {
        for (size_t i = 0; i < mappedFileVec.size(); ++i)
        {
            if (mappedFileVec[i])
            {
                auto it = mappedFilemap.find(i);
                if (it == mappedFilemap.end()) continue;
                munmap(it->second.map, it->second.fileSize);
            }
        }
    }
    SeekObj * Find(const std::string& word, Location target, uint32_t chunkNum) const {
        char hashFilename[32];
        snprintf(hashFilename, sizeof(hashFilename), "HashFile_%05u", chunkNum);
        HashFile hashFile(hashFilename);
        const HashBlob *hashblob = hashFile.Blob();
        const SerialTuple * tup = hashblob->Find(word.c_str());
        if (!tup) return nullptr;
        if (!mappedFileVec[chunkNum]) return nullptr;
        auto it = mappedFilemap.find(chunkNum);
        if (it == mappedFilemap.end()) return nullptr;
        const MappedFile& mf = it->second; 
        Location fileOffset = tup->Value;
        if (fileOffset >= mf.fileSize) return nullptr; // shouldn't happen
        uint8_t* fileStart = static_cast<uint8_t*>(mf.map);
        uint8_t seekTableSize = fileStart[fileOffset];
        if (seekTableSize != 0)
        {
            // Start of seek table
            const uint8_t* varintBuf = fileStart + fileOffset + 1;
            // Need number of entries to know how many bytes to skip ahead
            uint64_t numEntries = 0;
            // Seek table buffer
            const uint8_t* seekTable = decodeVarint(varintBuf, numEntries);
            // Index into the table
            size_t tableIndex = target >> BLOCK_OFFSET_BITS;
            // Gets seek table entry at target index
            const uint8_t* entryPtr = seekTable + tableIndex * ENTRY_SIZE;
            uint64_t entryOffset = *reinterpret_cast<const uint64_t*>(entryPtr);
            uint64_t entryLocation = *reinterpret_cast<const uint64_t*>(entryPtr + sizeof(uint64_t));
            // Returns if target was an entry in seek table
            if (entryLocation == target) 
            {
                auto obj = new SeekObj;
                obj->offset = entryOffset;
                obj->location = entryLocation;
                return obj;
            }
            // Need to linearly scan to find first entry with location >= target
            uint64_t currentLocation = entryLocation;
            uint64_t currentOffset = entryOffset;
            while (currentLocation < target)
            {
                uint64_t delta = 0;
                // decode varint automatically moves the buffer ahead
                varintBuf = decodeVarint(varintBuf, delta);
                currentLocation += delta;
                // current offset is the number of bytes into this posting list we are. I think
                currentOffset += SizeOf(delta);
                if (currentLocation >= target)
                {
                    auto obj = new SeekObj;
                    obj->offset = currentOffset;
                    obj->location = currentLocation;
                    return obj;
                }
            }
        }
        else
        {
            // Linearly scan if we don't have a seek table
            auto fileStart = static_cast<uint8_t*>(mf.map);
            const uint8_t* varintBuf = fileStart + fileOffset + 1;
            uint64_t currentLocation = 0;
            uint64_t currentOffset = 0;
            while (currentLocation < target)
            {
                uint64_t delta = 0;
                varintBuf = decodeVarint(varintBuf, delta);
                currentLocation += delta;
                currentOffset += SizeOf(delta);
                if (currentLocation >= target)
                {
                    auto obj = new SeekObj;
                    obj->offset = currentOffset;
                    obj->location = currentLocation;
                    return obj;
                }
            }
            }

        return nullptr;
    }
};
