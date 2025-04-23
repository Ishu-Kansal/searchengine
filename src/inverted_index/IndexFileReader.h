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
/** @brief Penalty value used in FindClosestPostingDistancesToAnchor when a word is not found or no close posting exists. */
constexpr Location NO_OCCURENCE_PENALTY = 1000;

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
         * @brief Checks if the requested chunk is valid.
         * @param chunkNum The chunk index.
         * @return True if the requested mapped files are valid, false otherwise.
         */
        bool checkChunkValidity(uint32_t chunkNum) const 
        {
            if (chunkNum >= mappedFiles.size() || !mappedFiles[chunkNum]) 
            {
                fprintf(stderr, "DEBUG: Chunk %u not loaded or out of bounds.\n", chunkNum);
                return false;
            }
            if (!hashBlobPtrs[chunkNum]) 
            {
                fprintf(stderr, "DEBUG: Hash file %u was not mapped or is invalid.\n", chunkNum);
                return false;
            }
            return true;
        }
        struct ValidTuple
        {
            const SerialTuple* tup;
            bool isValid;

            ValidTuple() = default;
            ValidTuple( const SerialTuple* inTup, bool valid) : tup(inTup), isValid(valid) {}
        };
        /**
         * @brief Checks if hashblob is valid.
         * @param chunkNum the chunk index.
         * @param word word we're searching for.
         * @return ValidTuple that has Serial Tuple and a boolean to see if hashblob is valid or not
         */
        ValidTuple checkHashBlobValidity(uint32_t chunkNum, const std::string & word) const 
        {
            const HashBlob *hashblob = hashBlobPtrs[chunkNum];
            if (!hashblob) 
            {
                fprintf(stderr, "DEBUG: Hash blob pointer null for chunk %u.\n", chunkNum);
                return ValidTuple(nullptr, false);
            }

            const SerialTuple* tup = hashblob->Find(word.c_str());
            if (!tup) 
            {
                return ValidTuple(nullptr, false);
            }
            return ValidTuple(tup, true);
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
        /**
         * @brief Searches the seek table for the entry closest to target location.
         * Uses a galloping search followed by binary search
         * @param seekTableStart Pointer to the beginning of the seek table data.
         * @param fileEnd Pointer to the end of the memory-mapped file (for bounds checking).
         * @param numSeekTableEntries The total number of entries in the seek table.
         * @param target The target location to search for.
         * @param[out] outBestOffset The offset associated with the best found seek table entry.
         * @param[out] outBestLocation The location associated with the best found seek table entry.
         * @param[out] outIndex The posting index for the best entry.
         * @param[in,out] outTableIndex A hint for the starting index in the seek table; updated with the index of the best entry found.
         * @return True if a suitable entry was found or target is before the first entry, false on read error.
         */
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

                    if (cursor >= numSeekTableEntries) break;

                    const uint8_t * readPtr = (ENTRY_SIZE * cursor) + seekTableStart;

                    if (!readUint64_t(readPtr, fileEnd, entryOffset)) { return false; }
                    if (!readUint64_t(readPtr, fileEnd, entryLocation)) { return false; }

                }

                int best = -1;
                if (cursor != 0)
                {
                    int low = cursor - jump;
                    int high = cursor;
                    if (cursor > (numSeekTableEntries - 1))
                    {
                        high = numSeekTableEntries - 1;
                
                    }
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
                if (mlock(map, fileSize) == -1) 
                {
                    fprintf(stderr, "WARNING: mlock failed for index file '%s' (size %zu): %s. \n", indexFilename, fileSize, strerror(errno));
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
                if (mlock(hashMap,hashFileSize) == -1)
                {
                    fprintf(stderr, "WARNING: mlock failed for hash file '%s' (size %zu): %s. \n", hashFilename, hashFileSize, strerror(errno));
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
    std::shared_ptr<SeekObj> Find(
        const std::string& word, 
        Location target, 
        uint32_t chunkNum,
        int tableIndex = -1) const {

        if (!checkChunkValidity(chunkNum))
        {
            return nullptr;
        }
        const ValidTuple & validTup = checkHashBlobValidity(chunkNum, word);
        if (!validTup.isValid)
        {
            return nullptr;
        }
        // Get mapped file
        const void* mapPtr = mappedFiles[chunkNum]->get();
        size_t fileSize = mappedFiles[chunkNum]->size();

        const uint8_t* fileStart = static_cast<const uint8_t*>(mapPtr);
        const uint8_t* fileEnd = fileStart + fileSize;

        Location postingListOffset = validTup.tup->Value; // Start of posting list
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
            // cout << currentLocation << '\n';
            if (currentLocation >= target)
            {
                // cout << "Deltas decoded: " << decodeCounter << "\n";
                // cout << decodeCounter << "\n";
                return std::make_shared<SeekObj>(currentOffset, currentLocation, delta, index, numPosts, tableIndex);
          
            }
            index++;
            if (index > numPosts) 
            {
                return nullptr;
            }
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
        size_t tableIndex = (index) >> BLOCK_OFFSET_BITS;

        uint64_t numSeekTableEntries = 0;

        if (numSeekTableEntriesSize > 0 && tableIndex > 0)
        {
            const uint8_t* seekTable = fileStart + 1;
            decodeVarint(seekTable, numSeekTableEntries);
            tableIndex = std::min(uint64_t(tableIndex), numSeekTableEntries);
            const uint8_t* entryPtr = seekTable + ((tableIndex - 1) * URL_ENTRY_SIZE) + numSeekTableEntriesSize;

            uint64_t entryOffset;
            
            if (!readUint64_t(entryPtr, fileEnd, entryOffset)) { return nullptr; }

            const uint8_t* urlPtr = seekTable + (URL_ENTRY_SIZE * numSeekTableEntries) + entryOffset + numSeekTableEntriesSize;
            uint64_t urlIndex = (tableIndex << BLOCK_OFFSET_BITS);
            
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
                obj->staticRank = static_cast<uint8_t>(*urlPtr);
   
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
    /**
    * @brief Loads a specific range of locations from a word's posting list within a given chunk.
    * Retrieves all locations L such that startLoc < L < endOfDocLocation. 
    * Used for loading anchor locations for span calculations.
    * @param word The word whose posting list chunk to load.
    * @param chunkNum The index of the chunk file containing the posting list.
    * @param startLoc Start location of the document.
    * @param endOfDocLocation End location of the document.
    * @param tableIndex hint for the initial seek table search index.
    * @return A vector containing the locations strictly within the specified range (startLoc, endOfDocLocation).
    * Returns an empty vector if the word is not found, the chunk is invalid, the range is invalid.
    */
    std::vector<Location> LoadChunkOfPostingList(
        const std::string& word, 
        uint32_t chunkNum,
        uint64_t startLoc,
        uint64_t endOfDocLocation,
        int tableIndex = -1) const
    {
        std::vector<Location> results;
        // Avoid growth cost
        results.reserve(64);
        if (!checkChunkValidity(chunkNum))
        {
            return results;
        }
        const ValidTuple & validTup = checkHashBlobValidity(chunkNum, word);
        if (!validTup.isValid)
        {
            return results;
        }

        const void* mapPtr = mappedFiles[chunkNum]->get();
        size_t fileSize = mappedFiles[chunkNum]->size();

        const uint8_t* fileStart = static_cast<const uint8_t*>(mapPtr);
        const uint8_t* fileEnd = fileStart + fileSize;

        Location postingListOffset = validTup.tup->Value; 
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
        
        // load locations within the [startLoc, endOfDocLocation) range
        while (currentIndex < numPosts && currentLocation < endOfDocLocation)
        {
             if (postingListBuf >= fileEnd) break;

             uint64_t delta = 0;
             nextPtr = decodeVarint(postingListBuf, delta);
             if (!nextPtr || nextPtr > fileEnd) break; 

             postingListBuf = nextPtr;
             currentLocation += delta;
             if (currentLocation < endOfDocLocation && currentLocation > startLoc)
             {
                results.push_back(currentLocation);
             }
             currentIndex++;
             
        }

        return results;
    }
    /**
     * @brief Finds the closest occurrence of a word to each anchor term location
     * For each location in anchorLocations, this function finds the location L in the posting list
     * for word (where L < endOfDocLocation) that minimizes abs(L - targetLocation). It returns the minimum absolute *difference* found for each target location.
     * 
     * Since both lists are sorted, we can basically use two pointers to calculate min diffs.
     * 
     * @param word The word whose posting list to search.
     * @param chunkNum The index of the chunk file.
     * @param anchorLocations Anchor locations.
     * @param endOfDocLocation location where matched document ends.
     * @param tableIndex hint for seek table search index.
     * @return A vector of the same size as anchorLocations. Each element i contains the minimum absolute difference found between anchorLocations[i] and an actual posting location for word.
     */
    std::vector<Location> FindClosestPostingDistancesToAnchor(
        const std::string& word,
        uint32_t chunkNum,
        const std::vector<Location>& anchorLocations,
        Location endOfDocLocation,
        int tableIndex = -1) const
    {      
        if (anchorLocations.empty()) 
        {
            // Caller should not call this function with empty anchor vector
            fprintf(stderr, "DEBUG: EMPTY target locations\n");
            return std::vector<Location>(anchorLocations.size(), NO_OCCURENCE_PENALTY);
        }
        if (!checkChunkValidity(chunkNum))
        {
            return std::vector<Location>(anchorLocations.size(), NO_OCCURENCE_PENALTY);
        }
        const ValidTuple & validTup = checkHashBlobValidity(chunkNum, word);
        if (!validTup.isValid)
        {
            return std::vector<Location>(anchorLocations.size(), NO_OCCURENCE_PENALTY);
        }

        const void* mapPtr = mappedFiles[chunkNum]->get();
        size_t fileSize = mappedFiles[chunkNum]->size();
        if (!mapPtr || fileSize == 0) 
        {
             fprintf(stderr, "ERROR: Mapped file for chunk %u is invalid or empty.\n", chunkNum);
             return std::vector<Location>(anchorLocations.size(), NO_OCCURENCE_PENALTY);
        }

        const uint8_t* fileStart = static_cast<const uint8_t*>(mapPtr);
        const uint8_t* fileEnd = fileStart + fileSize;

        Location postingListOffset = validTup.tup->Value;
        if (postingListOffset >= fileSize) 
        {
            fprintf(stderr, "ERROR: Offset %llu out of bounds (%zu) for word '%s' chunk %u.\n",
            (unsigned long long)postingListOffset, fileSize, word.c_str(), chunkNum);
            return std::vector<Location>(anchorLocations.size(), NO_OCCURENCE_PENALTY);
        }

        const uint8_t* postingListBuf = fileStart + postingListOffset;
        if (postingListBuf >= fileEnd) 
        {
            fprintf(stderr, "ERROR: Posting list offset points beyond file end.\n");
            return std::vector<Location>(anchorLocations.size(), NO_OCCURENCE_PENALTY);
        }

        if (postingListBuf + NUM_ENTRIES_SIZE_BYTES > fileEnd) 
        {
             fprintf(stderr, "ERROR: Buffer too small for numSeekTableEntriesSize byte.\n");
             return std::vector<Location>(anchorLocations.size(), NO_OCCURENCE_PENALTY);
        }
        
        uint8_t numSeekTableEntriesSize = *postingListBuf;
        postingListBuf += NUM_ENTRIES_SIZE_BYTES;

        uint64_t numSeekTableEntries = 0;
        uint64_t numPosts = 0;
     
        const uint8_t* nextPtr = nullptr;

        if (numSeekTableEntriesSize) 
        {
            if (postingListBuf + numSeekTableEntriesSize > fileEnd) 
            {
                fprintf(stderr, "ERROR: Not enough space for numSeekTableEntries varint encoding.\n");
                return std::vector<Location>(anchorLocations.size(), NO_OCCURENCE_PENALTY);
            }
            nextPtr = decodeVarint(postingListBuf, numSeekTableEntries);
            if (!nextPtr || nextPtr > fileEnd) return std::vector<Location>(anchorLocations.size(), NO_OCCURENCE_PENALTY);
            postingListBuf = nextPtr;

            nextPtr = decodeVarint(postingListBuf, numPosts);
            if (!nextPtr || nextPtr > fileEnd) return std::vector<Location>(anchorLocations.size(), NO_OCCURENCE_PENALTY);
            postingListBuf = nextPtr;
        }
        else 
        {
            nextPtr = decodeVarint(postingListBuf, numPosts);
            if (!nextPtr || nextPtr > fileEnd) return std::vector<Location>(anchorLocations.size(), NO_OCCURENCE_PENALTY);
            postingListBuf = nextPtr;
        }

        if (numPosts == 0) return std::vector<Location>(anchorLocations.size(), NO_OCCURENCE_PENALTY);

        Location currentLocation = 0;
        Location currentOffset = 0;
        uint64_t currentIndex = 0;
        const uint8_t * seekTableStart = postingListBuf;

        // Don't want to calculate min distances if they're far away from first anchor location or last anchor location.
        Location startLoc = (anchorLocations[0] >= 32) ? anchorLocations[0] - 32 : 0;
        endOfDocLocation = std::min(endOfDocLocation, anchorLocations.back() + 32);

        bool found = findBestSeekEntry(seekTableStart, fileEnd, numSeekTableEntries, startLoc, currentOffset, currentLocation, currentIndex, tableIndex);

        if (!found) return std::vector<Location>(anchorLocations.size(), NO_OCCURENCE_PENALTY);;

        size_t numTargets = anchorLocations.size();
        std::vector<Location> minDifference(numTargets, std::numeric_limits<Location>::max());

        postingListBuf += (ENTRY_SIZE * numSeekTableEntries) + currentOffset;

        size_t targetIdx = 0;
        // Loop until we run out of posts or done calculating min differences for each anchor location
        while (currentIndex < numPosts && targetIdx < numTargets)
        {
            if (postingListBuf >= fileEnd) break;
            Location target = anchorLocations[targetIdx];
            Location &minDist = minDifference[targetIdx];
            int64_t distCurr = std::abs(static_cast<int64_t>(currentLocation) - static_cast<int64_t>(target));
            // Basic concept is that since its each list is always increasing. 
            // If the current distance is greatly than the min difference, that means the next post's distance will also be greater, and we should move the targetIdx forward, 
            if (distCurr < minDist)
            {
                minDist = distCurr;
                uint64_t delta = 0;
                nextPtr = decodeVarint(postingListBuf, delta);
                if (!nextPtr || nextPtr > fileEnd) break; 
                postingListBuf = nextPtr;
                currentLocation += delta;
                currentIndex++;
                if (currentLocation >= endOfDocLocation)
                {
                    currentLocation -= delta;
                    break;
                }
            }
            else
            {
                ++targetIdx;
            }
        }
        // fill out the rest of the minimal distance spans if there are less posts for the current word than the anchor. Fills the remaining with the currentLocation's distance.
        for (; targetIdx < numTargets; ++targetIdx) 
        {
            Location target = anchorLocations[targetIdx];
            Location &minDist = minDifference[targetIdx];
            int64_t distCurr = std::abs(static_cast<int64_t>(currentLocation) - static_cast<int64_t>(target));

            minDist = distCurr;
        }
        return minDifference;
    }
};
