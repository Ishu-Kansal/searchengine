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


#include <algorithm>
#include <memory>
#include <string_view>
#include <unordered_map>

#include "../../HashTable/HashTableStarterFiles/HashBlob.h"
#include "../../HashTable/HashTableStarterFiles/HashTable.h"
#include "../../utils/utf_encoding.h"
#include "IndexFile.h"
#include "Index.h"
#include "RAII_utils.h"

/** @brief Size in bytes of a seek table entry for posting list (offset + location) */
constexpr size_t ENTRY_SIZE = 16;
/** @brief Size in bytes of a seek table entry for URL lists (offset) */
constexpr size_t URL_ENTRY_SIZE = 8;
/** @brief Size in bytes used to store the size (in bytes) of the varint encoding the number of seek table entries */
constexpr size_t NUM_ENTRIES_SIZE_BYTES = 1;
/** @brief Size in bytes for the URL length within a URL entry. */
constexpr size_t URL_ENTRY_HEADER_BYTES = 1;
/** @brief Size in bytes for the static rank within a URL entry. */
constexpr size_t URL_ENTRY_RANK_BYTES = 1;
typedef size_t Location;

/**
 * @struct SeekObj
 * @brief Struct to hold the results of a search within a posting list
 * Contains information about the found posting, including its offset, location
 * delta, index, and total postings count
 */
struct SeekObj {
    Location offset;
    Location location;
    Location delta;
    Location index;
    unsigned numOccurrences;
    int seekTableIndex;

    SeekObj() = default;
    SeekObj(Location off, Location loc, Location d, Location idx, unsigned num, int seekIdx)
      : offset(off), location(loc), delta(d), index(idx), numOccurrences(num),seekTableIndex(seekIdx) {}
};

/**
 * @class IndexFileReader
 * @brief Reads data from serialized index chunk files ("IndexChunk_XXXXX") and hash files ("HashFile_XXXXX")
 * Uses memory mapping to accessindex chunk data. Allows searching for postings
 * and retrieving URL information based on location/index
 */
class IndexFileReader {
private:
    static constexpr size_t FILENAME_BUFFER_SIZE = 32;
    /** @brief Vector holding unique pointers to memory-mapped regions for each index chunk file */
    std::vector<std::unique_ptr<MappedMemory>> mappedFiles;

    std::vector<std::unique_ptr<MappedMemory>> mappedHashFiles;

    std::vector<const HashBlob*> hashBlobPtrs;   

    size_t FileSize(int f)
    {
        struct stat fileInfo;
        if (fstat(f, &fileInfo) == -1) {
            return 0;
        }
        return fileInfo.st_size;
    }
    /**
     * @brief Safely reads a uint64_t value from a memory buffer
     * Advances the pointer past the read value. Checks for buffer boundaries
     * @param currentPtr Reference to the pointer indicating the current read position in the buffer
     * @param endPtr Pointer to the end of the valid buffer region
     * @param value Reference to a uint64_t where the read value will be stored
     * @return True if the read was successful, false if there was not enough space in the buffer
     */
    inline static bool readUint64_t(const uint8_t*& currentPtr, const uint8_t* endPtr, uint64_t& value) 
    {
        if (currentPtr == nullptr || endPtr == nullptr || currentPtr + sizeof(uint64_t) > endPtr) {
            return false;
        }
        memcpy(&value, currentPtr, sizeof(uint64_t));
        currentPtr += sizeof(uint64_t);
        return true;
    }

    bool findBestSeekEntry(
        const uint8_t* seekTableStart,
        const uint8_t* fileEnd,
        size_t numSeekTableEntries,
        Location target,
        Location& outBestOffset,
        Location& outBestLocation,
        uint64_t& outIndex,
        int& outTableIndex)
        const
    {
        outIndex = 0;
        uint64_t entryOffset = 0;
        uint64_t entryLocation = 0;
        outBestOffset = entryOffset;
        outBestLocation = entryLocation;
        
        if (numSeekTableEntries > 0)
        {

            int cursor = (outTableIndex > 0) ? outTableIndex : 0;
            int jump = 1;
            const uint8_t* base = seekTableStart;
            const uint8_t * tempPtr = base + cursor * ENTRY_SIZE;

            if (!readUint64_t(tempPtr, fileEnd, entryOffset)) { return false; }
            if (!readUint64_t(tempPtr, fileEnd, entryLocation)) { return false; }

            while (cursor < numSeekTableEntries && target > entryLocation)
            {
                jump *= 2;
                cursor += jump;

                const uint8_t * readPtr = (ENTRY_SIZE * cursor) + seekTableStart;

                if (!readUint64_t(readPtr, fileEnd, entryOffset)) { return false; }
                if (!readUint64_t(readPtr, fileEnd, entryLocation)) { return false; }

            }

            int best = -1;
            if (cursor != 0)
            {
                int low = cursor - jump;
                int high = min(cursor, static_cast<int>(numSeekTableEntries - 1));

    
                while (low <= high)
                {
                    int mid = (low + high) / 2;
                    const uint8_t * readPtr = (ENTRY_SIZE * mid) + seekTableStart;
    
                    if (!readUint64_t(readPtr, fileEnd, entryOffset)) { return false; }
                    if (!readUint64_t(readPtr, fileEnd, entryLocation)) { return false; }
    
                    if (entryLocation <= target) 
                    {
                        best = mid;
                        outBestOffset = entryOffset;
                        outBestLocation = entryLocation;
                        low = mid + 1;
                    }
                    else 
                    {
                        high = mid - 1; 
                    }
                }
            }

            if (best == -1)
            {
                outBestOffset = 0;
                outBestLocation = 0;
                outTableIndex = -1;
                outIndex = 0;
                return true;
            }
            outIndex = (best + 1) * BLOCK_SIZE;
            outTableIndex = best;
        }
        return true;
    }
public:
    /** @brief Deleted default constructor. An IndexFileReader requires the number of chunks */
    IndexFileReader() = delete;
    /**
     * @brief Constructs an IndexFileReader and memory-maps the specified number of index chunk files
     * Attempts to open and mmap IndexChunk_00000, IndexChunk_00001, ..., up to numChunks-1
     * Warnings are printed for files that cannot be opened or mapped, but construction continues
     * @param numChunks The total number of index chunk files to manage
     */
    explicit IndexFileReader(uint32_t numChunks) : mappedFiles(numChunks), mappedHashFiles(numChunks), hashBlobPtrs(numChunks, nullptr) 
    {
        for (unsigned i = 0; i < numChunks; ++i)
        {
            char indexFilename[FILENAME_BUFFER_SIZE];
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
            if (madvise(map, fileSize, MADV_SEQUENTIAL) == -1) {
                fprintf(stderr, "WARNING: madvise(MADV_SEQUENTIAL) failed for index file '%s': %s\n", indexFilename, strerror(errno));
            }
            mappedFiles[i] = std::make_unique<MappedMemory>(map, fileSize);

            char hashFilename[FILENAME_BUFFER_SIZE];
            snprintf(hashFilename, sizeof(hashFilename), "HashFile_%05u", i);
            int hashFd = open(hashFilename, O_RDONLY, 0666);
            if (hashFd == -1) 
            {
                fprintf(stderr, "WARNING: Couldn't open hash file '%s': %s\n", hashFilename, strerror(errno));
                continue; 
            }

            FileDescriptor hashFileDesc(hashFd);
            size_t hashFileSize = FileSize(hashFileDesc.get());
            if (hashFileSize < sizeof(HashBlob)) 
            { 
                 fprintf(stderr, "WARNING: Hash file '%s' is too small (%zu bytes) or stat failed.\n", hashFilename, hashFileSize);
                 continue;
            }

            void* hashMap = mmap(NULL, hashFileSize, PROT_READ, MAP_PRIVATE, hashFileDesc.get(), 0);
            if (hashMap == MAP_FAILED) 
            {
                 fprintf(stderr, "WARNING: Couldn't mmap hash file '%s': %s\n", hashFilename, strerror(errno));
                 continue;
            }

            mappedHashFiles[i] = std::make_unique<MappedMemory>(hashMap, hashFileSize);

            hashBlobPtrs[i] = static_cast<const HashBlob*>(mappedHashFiles[i]->get());

        }
    }

    /**
      * @brief Finds the first posting in a specific chunk's posting list for a given word whose location is >= target
      * Uses the corresponding "HashFile_XXXXX" to find the offset of the word's posting list within the
      * memory-mapped "IndexChunk_XXXXX" file. Then searches for posts
      *
      * @param word The word whose posting list to search
      * @param target The target location to search for. The function finds the first posting with location >= target
      * @param chunkNum The index of the chunk file (e.g., 0 for "IndexChunk_00000") to search within
      * @return A unique_ptr to a SeekObj containing details of the found posting, or nullptr if:
      *         - The chunk number is invalid or the chunk file wasn't mapped
      *         - The word is not found in the hash file for that chunk
      *         - An error occurs during parsing (e.g., offset out of bounds, read error)
      *         - No posting with location >= target exists in the list
      */
    std::unique_ptr<SeekObj> Find(
        const std::string& word, 
        Location target, 
        uint32_t chunkNum,
        int tableIndex = -1) const {

        if (chunkNum >= mappedFiles.size() || !mappedFiles[chunkNum]) 
        {
            fprintf(stderr, "DEBUG: invalid chunkNum or unmapped chunk: %u\n", chunkNum);
            return nullptr;
        }
        if (!hashBlobPtrs[chunkNum])
        { 
            fprintf(stderr, "DEBUG: Hash file %u was not mapped successfully or is invalid.\n", chunkNum);
            return nullptr;
        }
        const HashBlob *hashblob = hashBlobPtrs[chunkNum]; 

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

        Location postingListOffset = tup->Value; // Start of posting list
        if (postingListOffset >= fileSize) 
        {
            // shouldn't happen
            fprintf(stderr, "ERROR: Offset into file (%zu) is out of bounds (%zu) for word '%s' in chunk %u.\n", postingListOffset, fileSize, word.c_str(), chunkNum);
            return nullptr;
        } 
        const uint8_t * postingListBuf = fileStart + postingListOffset;
        uint8_t numSeekTableEntriesSize = *postingListBuf;
        postingListBuf += NUM_ENTRIES_SIZE_BYTES;

        uint64_t numSeekTableEntries = 0;
        uint64_t numPosts = 0;
     
        if (numSeekTableEntriesSize) 
        {
            if (postingListBuf + numSeekTableEntriesSize > fileEnd) 
            {
                fprintf(stderr, "ERROR: Not enough space for numSeekTableEntries varint encoding.\n");
                return nullptr;
            }
            postingListBuf = decodeVarint(postingListBuf, numSeekTableEntries);
            postingListBuf = decodeVarint(postingListBuf, numPosts);
        }
        else 
        {
            postingListBuf = decodeVarint(postingListBuf, numPosts);
        }

        if (numPosts == 0) return nullptr;

        const uint8_t * seekTableStart = postingListBuf;

        uint64_t index = 0;
        Location bestOffset = 0;
        Location bestLocation = 0;

        bool found = findBestSeekEntry(seekTableStart, fileEnd, numSeekTableEntries, target, bestOffset, bestLocation, index, tableIndex);

        if (!found) return nullptr;
        postingListBuf += (ENTRY_SIZE * numSeekTableEntries) + bestOffset;
    
        uint64_t currentLocation = bestLocation;
        uint64_t currentOffset = bestOffset;

        Location decodeCounter = 0;
        while (currentLocation <= target || target == 0)
        {
            decodeCounter++;
            uint64_t delta = 0;
            postingListBuf = decodeVarint(postingListBuf, delta);
            currentLocation += delta;
            currentOffset += SizeOf(delta);
            if (currentLocation >= target)
            {
                // cout << "Deltas decoded: " << decodeCounter << "\n";
                // cout << decodeCounter << "\n";
                return std::make_unique<SeekObj>(currentOffset, currentLocation, delta, index, numPosts, tableIndex);
          
            }
            index++;
            if (index > numPosts) return nullptr;
        }

        return nullptr;
    }

    /**
     * @brief Retrieves URL and static rank information for a document by its index within a chunk
     *
     * @param index The zero-based index of the doc to retrieve within the chunk's URL list.
     * @param chunkNum The index of the chunk file (e.g., 0 for "IndexChunk_00000") to search within
     * @return A unique_ptr to a Doc object containing the URL and static rank, or nullptr if:
     *         - The chunk number is invalid or the chunk file wasn't mapped
     *         - The requested index is out of bounds for the URL list in that chunk
     *         - An error occurs during parsing (e.g., offset out of bounds, read error)
     */
    std::unique_ptr<Doc> FindUrl(uint32_t index, uint32_t chunkNum)
    {
        if (!mappedFiles[chunkNum]) return nullptr;

        const void* mapPtr = mappedFiles[chunkNum]->get();
        size_t fileSize = mappedFiles[chunkNum]->size();
        const uint8_t* fileStart = static_cast<const uint8_t*>(mapPtr);
        const uint8_t* fileEnd = fileStart + fileSize;

        uint8_t numSeekTableEntriesSize = fileStart[0];
        size_t tableIndex = (index + 1) >> BLOCK_OFFSET_BITS;
        uint64_t numSeekTableEntries = 0;
        if (numSeekTableEntriesSize) 
        {
            const uint8_t* temp = fileStart + 1;
            decodeVarint(temp, numSeekTableEntries);
        }
        if (numSeekTableEntriesSize && tableIndex != 0)
        {
            const uint8_t* seekTable = fileStart + 1;
            const uint8_t* entryPtr = seekTable + ((tableIndex - 1) * URL_ENTRY_SIZE)+ numSeekTableEntriesSize;

            uint64_t entryOffset;
            
            if (!readUint64_t(entryPtr, fileEnd, entryOffset)) { return nullptr; }

            const uint8_t* urlPtr = seekTable + (URL_ENTRY_SIZE * numSeekTableEntries) + entryOffset + numSeekTableEntriesSize;
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
            const uint8_t* urlPtr = fileStart + (URL_ENTRY_SIZE * numSeekTableEntries) + 1 + numSeekTableEntriesSize;
            
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

    std::vector<Location> LoadChunkOfPostingList(
        const std::string& word, 
        uint32_t chunkNum,
        uint64_t startLoc,
        uint64_t endLoc,
        int tableIndex = -1) const
    {
        std::vector<Location> results;
        if (chunkNum >= mappedFiles.size() || !mappedFiles[chunkNum]) 
        {
            return results; 
        }

        if (!hashBlobPtrs[chunkNum])
        { 
            fprintf(stderr, "DEBUG: Hash file %u was not mapped successfully or is invalid.\n", chunkNum);
            return results;
        }
        const HashBlob *hashblob = hashBlobPtrs[chunkNum]; 
        if (!hashblob) 
        {
             return results;
        }

        const SerialTuple * tup = hashblob->Find(word.c_str());
        if (!tup) 
        {
             return results;
        }

        const void* mapPtr = mappedFiles[chunkNum]->get();
        size_t fileSize = mappedFiles[chunkNum]->size();

        const uint8_t* fileStart = static_cast<const uint8_t*>(mapPtr);
        const uint8_t* fileEnd = fileStart + fileSize;

        Location postingListOffset = tup->Value; 
        if (postingListOffset >= fileSize) 
        {
            fprintf(stderr, "ERROR: Offset into file (%zu) is out of bounds (%zu) for word '%s' in chunk %u.\n", postingListOffset, fileSize, word.c_str(), chunkNum);
            return results;
        } 

        const uint8_t * postingListBuf = fileStart + postingListOffset;
        if (postingListBuf >= fileEnd) return results;

        uint8_t numSeekTableEntriesSize = *postingListBuf;
        postingListBuf += NUM_ENTRIES_SIZE_BYTES;
         if (postingListBuf > fileEnd) return results;

        uint64_t numSeekTableEntries = 0;
        uint64_t numPosts = 0;
     
        const uint8_t* nextPtr = nullptr;

        if (numSeekTableEntriesSize) 
        {
            if (postingListBuf + numSeekTableEntriesSize > fileEnd) 
            {
                fprintf(stderr, "ERROR: Not enough space for numSeekTableEntries varint encoding.\n");
                return results;
            }
            nextPtr = decodeVarint(postingListBuf, numSeekTableEntries);
            if (!nextPtr || nextPtr > fileEnd) return results;
            postingListBuf = nextPtr;

            nextPtr = decodeVarint(postingListBuf, numPosts);
            if (!nextPtr || nextPtr > fileEnd) return results;
            postingListBuf = nextPtr;
        }
        else 
        {
            nextPtr = decodeVarint(postingListBuf, numPosts);
            if (!nextPtr || nextPtr > fileEnd) return results;
            postingListBuf = nextPtr;
        }

        if (numPosts == 0) return results;

        Location currentLocation = 0;
        Location currentOffset = 0;
        uint64_t currentIndex = 0;
        const uint8_t * seekTableStart = postingListBuf;
        bool found = findBestSeekEntry(seekTableStart, fileEnd, numSeekTableEntries, startLoc, currentOffset, currentLocation, currentIndex, tableIndex);

        if (!found) return results;
        postingListBuf += (ENTRY_SIZE * numSeekTableEntries) + currentOffset;
        while (currentIndex < numPosts && currentLocation < startLoc)
        {
             if (postingListBuf >= fileEnd) return results;
             uint64_t delta = 0;
             nextPtr = decodeVarint(postingListBuf, delta);
             if (!nextPtr || nextPtr > fileEnd) return results;

             postingListBuf = nextPtr;
             currentLocation += delta;
             currentIndex++;
        }
        
        // load locations within the [startLoc, endLoc) range
        while (currentIndex < numPosts && currentLocation < endLoc)
        {
             if (postingListBuf >= fileEnd) break;

             uint64_t delta = 0;
             nextPtr = decodeVarint(postingListBuf, delta);
             if (!nextPtr || nextPtr > fileEnd) break; 

             postingListBuf = nextPtr;
             currentLocation += delta;
             if (currentLocation < endLoc)
             {
                results.push_back(currentLocation);
             }
             currentIndex++;
             
        }

        return results;
    }
};
