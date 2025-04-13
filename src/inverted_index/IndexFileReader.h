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

#include <memory>
#include <unordered_map>

#include "../../HashTable/HashTableStarterFiles/HashBlob.h"
#include "../../HashTable/HashTableStarterFiles/HashTable.h"
#include "../../utils/utf_encoding.h"
#include "IndexFile.h"
#include "Index.h"

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
* @class FileDescriptor
* @brief RAII wrapper for a file descriptor
* Automatically closes the file descriptor when the object goes out of scope
*/
class FileDescriptor {
    public:
        /**
         * @brief Constructs a FileDescriptor, taking ownership of the provided file descriptor
         * @param fd The file descriptor to manage
         */
        explicit FileDescriptor(int fd) : fd_(fd) {}
        /**
         * @brief Destructor that closes the managed file descriptor if it's valid
         */
        ~FileDescriptor() { 
            if (fd_ != -1) 
                close(fd_); 
        }
        /**
         * @brief Gets the raw file descriptor value
         * @return The managed file descriptor
         */
        int get() const { return fd_; }
        FileDescriptor(const FileDescriptor&) = delete;
        FileDescriptor& operator=(const FileDescriptor&) = delete;
    private:
    
        int fd_ = -1;
};

/**
 * @class MappedMemory
 * @brief RAII wrapper for memory mapped using mmap
 * Automatically unmaps the memory region when the object goes out of scope
 */
class MappedMemory {
    public:
        /**
         * @brief Constructs a MappedMemory object, taking ownership of the mapped region
         * @param ptr Pointer to the start of the mapped memory region
         * @param size The size of the mapped memory region in bytes
         */
        MappedMemory(void* ptr, size_t size) : ptr_(ptr), size_(size) {}
        /**
         * @brief Destructor that unmaps the memory region if it's valid
         */
        ~MappedMemory() { 
            if (ptr_ && ptr_ != MAP_FAILED) 
                munmap(ptr_, size_); 
        }
        /**
         * @brief Gets the pointer to the start of the mapped memory region
         * @return Void pointer to the mapped memory
         */
        void* get() const { return ptr_; }
        /**
         * @brief Gets the size of the mapped memory region
         * @return The size in bytes
         */
        size_t size() const { return size_; }
        MappedMemory(const MappedMemory&) = delete;
        MappedMemory& operator=(const MappedMemory&) = delete;
    private:
        void* ptr_;
        size_t size_;
};
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
    SeekObj(Location off, Location loc, Location idx, Location d, unsigned num) 
    : offset(off), location(loc), index(idx), delta(d), numOccurrences(num) {}
};
/**
 * @class IndexFileReader
 * @brief Reads data from serialized index chunk files ("IndexChunk_XXXXX") and hash files ("HashFile_XXXXX")
 * Uses memory mapping to accessindex chunk data. Allows searching for postings
 * and retrieving URL information based on location/index
 */
class IndexFileReader {
private:
    /** @brief Vector holding unique pointers to memory-mapped regions for each index chunk file */
    std::vector<std::unique_ptr<MappedMemory>> mappedFiles;
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
    /** @brief Deleted default constructor. An IndexFileReader requires the number of chunks */
    IndexFileReader() = delete;
    /**
     * @brief Constructs an IndexFileReader and memory-maps the specified number of index chunk files
     * Attempts to open and mmap IndexChunk_00000, IndexChunk_00001, ..., up to numChunks-1
     * Warnings are printed for files that cannot be opened or mapped, but construction continues
     * @param numChunks The total number of index chunk files to manage
     */
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

        Location postingListOffset = tup->Value; // Start of posting list
        if (postingListOffset >= fileSize) 
        {
            // shouldn't happen
            fprintf(stderr, "ERROR: Offset into file (%zu) is out of bounds (%zu) for word '%s' in chunk %u.\n", postingListOffset, fileSize, word.c_str(), chunkNum);
            return nullptr;
        } 

        /*
            Format for Posting List:
            [ numSeekEntriesVarintSizeBytes (1 byte)]
            [ varint: numSeekTableEntries           ] (optional, only if size byte > 0)
            [ varint: numTotalElements              ]
            [ seekTable (numSeekTableEntries * ENTRY_SIZE bytes) ] (optional) 8 bytes for offset and 8 bytes for location
            [ postingListData (varint deltas)       ]
        */

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

        size_t tableIndex = (target + 1) >> BLOCK_OFFSET_BITS;
        if (numSeekTableEntriesSize && tableIndex != 0)
        {
            // Gets seek table entry at target index
            const uint8_t * seekEntry = postingListBuf + ((tableIndex - 1) * ENTRY_SIZE);

            uint64_t entryOffset;
            uint64_t entryLocation;
            
            if (!readUint64_t(seekEntry, fileEnd, entryOffset)) { nullptr; }
            if (!readUint64_t(seekEntry, fileEnd, entryLocation)) { nullptr; }

            uint64_t index = (tableIndex * BLOCK_SIZE) - 1;
      
            // Need to linearly scan to find first entry with location >= target
            uint64_t currentLocation = entryLocation;
            uint64_t currentOffset = entryOffset;
            postingListBuf += ((ENTRY_SIZE * numSeekTableEntries) + entryOffset);

            while (currentLocation < target || target == 0)
            {
                uint64_t delta = 0;
                // decode varint automatically moves the buffer ahead
                postingListBuf = decodeVarint(postingListBuf, delta);
                currentLocation += delta;
                currentOffset += SizeOf(delta);
                if (currentLocation >= target)
                {
                    return std::make_unique<SeekObj>(currentOffset, currentLocation, index, delta, numPosts);
                }
                ++index;
            }
        }
        else
        {
            // Linearly scan if we don't have a seek table
            // Or if index is 0 (means element is within the first 8192 entries)
            postingListBuf += (ENTRY_SIZE * numSeekTableEntries);
            uint64_t currentLocation = 0;
            uint64_t currentOffset = 0;
            uint64_t index = 0;
            
            while (currentLocation < target || target == 0)
            {
                uint64_t delta = 0;
                postingListBuf = decodeVarint(postingListBuf, delta);
                currentLocation += delta;
                currentOffset += SizeOf(delta);
                if (currentLocation >= target)
                {
                    return std::make_unique<SeekObj>(currentOffset, currentLocation, index, delta, numPosts);
                }
                index++;
            }
        }

        return nullptr;;
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
        if (!mappedFiles[chunkNum]) return nullptr;;

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
            
            if (!readUint64_t(entryPtr, fileEnd, entryOffset)) { return nullptr;; }

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
};
