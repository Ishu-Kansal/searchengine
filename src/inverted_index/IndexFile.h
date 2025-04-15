#pragma once

#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>

#include "../../HashTable/HashTableStarterFiles/HashBlob.h"
#include "Index.h"
#include "../../HashTable/HashTableStarterFiles/HashTable.h"
#include "../../utils/utf_encoding.h"

/**
 * @brief Number of bits used for byteOffset within block
 * Determines size of a block (2^BLOCK_OFFSET_BITS)
 */
constexpr size_t BLOCK_OFFSET_BITS = 13;
/**
 * @brief Used in seek table creation
 * New seek table entry every BLOCKS_SIZE elements in posting list
 */
constexpr size_t BLOCK_SIZE = 1 << BLOCK_OFFSET_BITS;
/**
 * @brief Number of bytes used for the header of each URL entry 
 * (size byte + static rank byte)
 */
constexpr size_t URL_HEADER_BYTES = 2;

/**
 * @brief Initial capacity reservation for chunk data buffer (8 GB)
 */
constexpr size_t CHUNK_BUFFER_INITIAL_RESERVE = 8'000'000'000ULL;

/**
 * @brief Capacity reservation for temporary posting list buffer
 */
constexpr size_t POSTING_LIST_BUFFER_INITIAL_RESERVE = 1'000'000'000ULL;

/**
 * @brief Maximum chunk number allowed for filename (ensures %05u fits)
 */
constexpr uint32_t MAX_CHUNK_NUM = 99999;

/**
 * @brief Marker indicating no seek table is present
 */
constexpr uint8_t NO_SEEK_TABLE_MARKER = 0;

// Didn't use pushVarint for some of them, because some of the values are one byte and I want the full range from 0 - 255.

/**
 * @class IndexFile
 * @brief Handles the serialization and writing of an IndexChunk and its dictionary to disk
 *
 * Class takes an IndexChunk, serializes its URL list and posting lists,
 * updates the dictionary with the byte offsets of the posting lists within the serialized data,
 * writes the serialized chunk data to a file named "IndexChunk_XXXXX", and writes the dictionary to hash table file named "HashFile_XXXXX"
 */
class IndexFile {
    private:

    /**
    * @brief File descriptor for the main postingIndex chunk data file ("IndexChunk_XXXXX").
    */
    int fileDescrip;

    /**
    * @brief Encodes a uint64_t value using variable-length encoding (Varint) and appends it to a data buffer
    * @param dataBuffer The vector of bytes to append the encoded value to
    * @param value The uint64_t value to encode
    */
    void pushVarint(std::vector<uint8_t>& dataBuffer, uint64_t value) {

        uint8_t buf[10];
        uint8_t* ptr = encodeVarint(value, buf);
        dataBuffer.insert(dataBuffer.end(), buf, ptr);

    }

    /**
    * @brief Serializes the list of URLs (Doc objects) into the provided data buffer
    *
    * Format:
    * [Seek Table Size (Varint Size Byte)]  // 1 byte for size of next field, 0 if no seek table
    * [Number of Seek Table Entries (Varint)] // Only if Seek Table Size > 0
    * [Seek Table Entries...]             // Only if Seek Table Size > 0
    * [Offset (uint64_t)]               // Byte byteOffset of the start of the block
    * [URL Data...]
    * [URL Length (uint8_t)]            // Length of the URL string
    * [URL String (bytes)]              // URL characters
    * [Static Rank (uint8_t)]           // Static rank of the document
    *
    * A seek table is only included if the number of URLs is >= BLOCK_SIZE.
    * Each seek table entry points to the start of a block containing BLOCK_SIZE URLs
    *
    * @param dataBuffer The vector of bytes to append the serialized URL list to
    * @param urlList The vector of Doc objects to serialize
    */

    void serializeUrlList(
        std::vector<uint8_t>& dataBuffer,
        const std::vector<Doc> &urlList)
    {
        if (urlList.size() < BLOCK_SIZE) 
        {
            dataBuffer.push_back(NO_SEEK_TABLE_MARKER); // No seek table
        }
        else
        {
            uint32_t numSeekTableEntries = urlList.size() >> BLOCK_OFFSET_BITS;
            // One byte to get size
            dataBuffer.push_back(SizeOf(numSeekTableEntries)); 
            pushVarint(dataBuffer, numSeekTableEntries);
        }
        uint32_t postingIndex = 0;
        uint64_t byteOffset = 0;
        std::vector<uint8_t> tempBuffer;
        tempBuffer.reserve(POSTING_LIST_BUFFER_INITIAL_RESERVE);
        for (auto & doc : urlList)
        {
            if (urlList.size() >= BLOCK_SIZE && (postingIndex + 1) % BLOCK_SIZE == 0 && postingIndex != 0)
            {
                uint8_t* bytes = reinterpret_cast<uint8_t*>(&byteOffset);
                dataBuffer.insert(dataBuffer.end(), bytes, bytes + sizeof(byteOffset));
                //bytes = reinterpret_cast<uint8_t*>(&postingIndex);
                //dataBuffer.insert(dataBuffer.end(), bytes, bytes + sizeof(postingIndex));
            }
        
            byteOffset += doc.url.size() + URL_HEADER_BYTES;
            postingIndex++;

            tempBuffer.push_back(uint8_t(doc.url.size()));
            tempBuffer.insert(tempBuffer.end(), doc.url.begin(), doc.url.end());
            tempBuffer.push_back(doc.staticRank);
        }
        dataBuffer.insert(dataBuffer.end(), tempBuffer.begin(), tempBuffer.end()); 
    }
    
    /**
    * @brief Serializes the list of PostingLists into the data buffer and updates the dictionary.
    *
    * For each PostingList:
    * 1. Updates the corresponding word's entry in the dictionary with the current byte byteOffset in the data buffer. This byteOffset marks the start of the serialized data for this posting list.
    * 2. Serializes the PostingList data:
    *    Format:
    *    [Seek Table Size (Varint Size Byte)]    // 1 byte for size of next field, 0 if no seek table
    *    [Number of Seek Table Entries (Varint)] // Only if Seek Table Size > 0
    *    [Number of Postings (Varint)]           // Total number of postings in the list
    *    [Seek Table Entries...]                 // Only if Seek Table Size > 0
    *    [Offset (uint64_t)]                     // Byte byteOffset within the posting data (after headers/seek table)
    *    [Absolute Location (uint64_t)]
    *    [Posting Data...]
    *    [Delta (Varint)]                        // Difference from the prevLocationious post's location
    *
    * A seek table is included if the number of postings is >= BLOCK_SIZE
    * Each seek table entry points to the start of a block containing BLOCK_SIZE postings
    *
    * @param dataBuffer The vector of bytes to append the serialized posting lists to
    * @param listOfPostingList The vector of PostingList objects to serialize
    * @param dictionary The HashTable mapping words to offsets, which will be updated
    */

    void serializePostingLists(
        std::vector<uint8_t>& dataBuffer,
        const vector<PostingList> &listOfPostingList,
        HashTable<const std::string, size_t> & dictionary)
    {   
        for (const PostingList & postingList : listOfPostingList)
        {
            const std::string & word = postingList.get_word();

            // dataBuffer.size() is the byte byteOffset for the start of this posting list
            bool success = dictionary.Update(word, dataBuffer.size());
            if (!success) [[unlikely]]
            {
                cerr << word << " not found in dictionary.";
                exit(EXIT_FAILURE);
            }
            if (postingList.size() < BLOCK_SIZE) 
            {
                dataBuffer.push_back(NO_SEEK_TABLE_MARKER); // No seek table
            }
            else
            {
                uint32_t numSeekTableEntries = postingList.size() >> BLOCK_OFFSET_BITS;
                // One byte to get size
                dataBuffer.push_back(SizeOf(numSeekTableEntries)); 
                pushVarint(dataBuffer, numSeekTableEntries);
            }
            
            // # of elements in posting list
            pushVarint(dataBuffer, postingList.size());

            uint64_t prevLocation = 0;
            uint32_t postingIndex = 0;
            uint64_t absoluteLocation = 0;
            uint64_t byteOffset = 0;
            std::vector<uint8_t> tempBuffer;
            tempBuffer.reserve(POSTING_LIST_BUFFER_INITIAL_RESERVE); 
            for (const Post & entry : postingList)
            {
                // We're making a seek table entry every BLOCK_SIZE words we encountered
                // but since postingIndex is 0-indexed we have to add 1
                if (postingList.size() >= BLOCK_SIZE && (postingIndex + 1) % BLOCK_SIZE == 0 && postingIndex != 0)
                {
                    // Offset into posting list
                    uint8_t* bytes = reinterpret_cast<uint8_t*>(&byteOffset);
                    dataBuffer.insert(dataBuffer.end(), bytes, bytes + sizeof(byteOffset));
                    // absolute location of element at postingIndex
                    bytes = reinterpret_cast<uint8_t*>(&absoluteLocation);
                    dataBuffer.insert(dataBuffer.end(), bytes, bytes + sizeof(absoluteLocation));
                }

                uint64_t delta = entry.location - prevLocation;
                absoluteLocation += delta;
                uint8_t deltaSize = SizeOf(delta);
       
                byteOffset += deltaSize;
                pushVarint(tempBuffer, delta);
                prevLocation = entry.location;
                ++postingIndex;
            }
            // adds all posts after seek table
            dataBuffer.insert(dataBuffer.end(), tempBuffer.begin(), tempBuffer.end()); 

        }
    }

    /**
    * @brief Serializes the entire IndexChunk into the provided data buffer
    *
    * Clears the buffer, then serializes the URL list and the posting lists
    * The dictionary is updated with new byteOffset of the word's posting list
    *
    * @param indexChunk The IndexChunk object to serialize
    * @param dataBuffer The vector of bytes to store the serialized chunk data
    * @param dictionary The HashTable associated with the chunk to be updated with offsets
    */
    void serializeChunk(
        const IndexChunk &indexChunk,
        std::vector<uint8_t>& dataBuffer,
        HashTable<const std::string, size_t> & dictionary)
    {
        dataBuffer.clear();

        const auto urlList = indexChunk.get_urls();
        const auto listOfPostingList = indexChunk.get_posting_lists();
        serializeUrlList(dataBuffer, urlList);
        serializePostingLists(dataBuffer, listOfPostingList, dictionary);
        
    }
    public:
    /**
    * @brief Constructs an IndexFile, serializing and writing an IndexChunk and its dictionary
    *
    * Takes a chunk number and an IndexChunk reference. It performs the following steps:
    * 1. Creates or truncates a file named "IndexChunk_XXXXX" (where XXXXX is the zero-padded chunk number)
    * 2. Creates a large buffer to hold the serialized data
    * 3. Calls serializeChunk to fill the buffer with the serialized URL list and posting lists
    * During serialization, the dictionary within indexChunk is updated with the byte offsets of each posting list within the buffer
    * 4. Writes the contents of buffer to the "IndexChunk_XXXXX" file
    * 5. Creates a HashFile named "HashFile_XXXXX", passing the updated dictionary to it
    * This HashFile constructor handles writing the dictionary to disk
    *
    * @param chunkNum The identifier for this chunk, used in filenames. Max 99999
    * @param indexChunk A reference to the IndexChunk object containing the data to be serialized and written
    *  
    */
    IndexFile(uint32_t chunkNum, IndexChunk &indexChunk)
    {
        if (chunkNum > MAX_CHUNK_NUM) exit(EXIT_FAILURE);
        char indexFilename[32];
        snprintf(indexFilename, sizeof(indexFilename), "IndexChunk_%05u", chunkNum);
        fileDescrip = open(indexFilename, O_RDWR | O_CREAT | O_TRUNC, 0666);
        if ( fileDescrip == -1 )
        {
            cerr << "Could not open " << indexFilename;
            exit(EXIT_FAILURE);
        }
        HashTable<const std::string, size_t> & dictionary = indexChunk.get_dictionary();
        // used vector so that we didn't need to calculate size of buffer beforehand
        // Every insert is at the end of the dataBuffer
        // no reallocation since we reserved mem
        std::vector<uint8_t> dataBuffer;
        dataBuffer.reserve(CHUNK_BUFFER_INITIAL_RESERVE); // 8 Gigabyte worth of bytes
        serializeChunk(indexChunk, dataBuffer, dictionary);

        ssize_t bytesWritten = write(fileDescrip, dataBuffer.data(), dataBuffer.size());
        if (bytesWritten == -1) 
        {
            cerr << "Error writing to " << indexFilename;
            exit(EXIT_FAILURE);
        }
        else if (static_cast<size_t>(bytesWritten) != dataBuffer.size()) 
        {
            cerr << "Incomplete write to " << indexFilename;
            exit(EXIT_FAILURE);
        }

        char hashFilename[32];
        snprintf(hashFilename, sizeof(hashFilename), "HashFile_%05u", chunkNum);
        HashFile hashfile(hashFilename, &dictionary);
          
    }
    /**
    * @brief Destructor for IndexFile.
    *
    * Closes the file descriptor associated with the "IndexChunk_XXXXX" file if it was successfully opened.
    */
    ~IndexFile()
    {
        if (fileDescrip >= 0)
        {
            close(fileDescrip);
        }
    }
};