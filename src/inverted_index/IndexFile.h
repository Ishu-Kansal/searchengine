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

constexpr size_t BLOCK_OFFSET_BITS = 13; // 2^13 or 8192
constexpr size_t BLOCK_SIZE = 1 << BLOCK_OFFSET_BITS;

// Didn't use pushVarint for some of them, because some of the values are only in the range of an unsigned byte.

class IndexFile {
    private:
    int fileDescrip;
    size_t FileSize(int f)
    {
        struct stat fileInfo;
        fstat(f, &fileInfo);
        return fileInfo.st_size;
    }
    void pushVarint(std::vector<uint8_t>& dataBuffer, uint64_t value) {

        uint8_t buf[10];
        uint8_t* ptr = encodeVarint(value, buf);
        dataBuffer.insert(dataBuffer.end(), buf, ptr);

    }
    void serializeUrlList(std::vector<uint8_t>& dataBuffer, const std::vector<Doc> &urlList, uint64_t urlListBytes)
    {
        // Every insert is at the end of the dataBuffer, so should be constant time
        uint8_t size = SizeOf(urlListBytes);
        dataBuffer.push_back(size);
        pushVarint(dataBuffer, urlListBytes);
        for (auto & doc : urlList)
        {
            dataBuffer.push_back(uint8_t(doc.url.size()));
            dataBuffer.insert(dataBuffer.end(), doc.url.begin(), doc.url.end());
            dataBuffer.push_back(doc.staticRank);
        }
    }
    void serializePostingLists(std::vector<uint8_t>& dataBuffer, const vector<PostingList> &listOfPostingList, HashTable<const std::string, size_t> & dictionary)
    {   
        for (const PostingList & postingList : listOfPostingList)
        {
            const std::string & word = postingList.get_word();
            // dataBuffer.size() is the byte offset for the start of this posting list
            bool success = dictionary.Update(word, dataBuffer.size());
            if (!success) [[unlikely]]
            {
                cerr << word << " not found in dictionary.";
                exit(EXIT_FAILURE);
            }
            if (postingList.size() < BLOCK_SIZE) 
            {
                dataBuffer.push_back(uint8_t(0)); // No seek table
            }
            else
            {
                uint32_t numEntries = postingList.size() >> BLOCK_OFFSET_BITS;
                // One byte to get size
                // dataBuffer.push_back(SizeOf(numEntries)); 
                pushVarint(dataBuffer, numEntries);
            }
            uint64_t prev = 0;
            uint32_t index = 0;
            uint64_t pos = 0;
            uint64_t offset = 0; // offset from start of posting list
            std::vector<uint8_t> tempBuffer;
            tempBuffer.reserve(10000000000UL); // 1 gb
            for (const Post & entry : postingList)
            {
                uint64_t delta = entry.location - prev;
                pos += delta;
                uint8_t deltaSize = SizeOf(delta);
                offset += deltaSize; 
                if (postingList.size() >= BLOCK_SIZE && index % BLOCK_SIZE == 0)
                {
                    // Adds offset and absolute location
                    uint8_t* bytes = reinterpret_cast<uint8_t*>(&offset);
                    dataBuffer.insert(dataBuffer.end(), bytes, bytes + sizeof(offset));
                    bytes = reinterpret_cast<uint8_t*>(&pos);
                    dataBuffer.insert(dataBuffer.end(), bytes, bytes + sizeof(pos));
                }
                pushVarint(tempBuffer, delta);
                prev = entry.location;
                ++index;
            }
            // adds all posts after seek table
            dataBuffer.insert(dataBuffer.end(), tempBuffer.begin(), tempBuffer.end()); 

        }
    }
    void serializeChunk(const IndexChunk &indexChunk, std::vector<uint8_t>& dataBuffer, HashTable<const std::string, size_t> & dictionary)
    {
        dataBuffer.clear();

        // Serialize everything 
        const auto urlList = indexChunk.get_urls();
        const auto listOfPostingList = indexChunk.get_posting_lists();
        serializeUrlList(dataBuffer, urlList, indexChunk.get_url_list_size_bytes());
        serializePostingLists(dataBuffer, listOfPostingList, dictionary);
        
    }
    public:
    IndexFile(uint32_t chunkNum, IndexChunk &indexChunk)
    {
        if (chunkNum > 99999) exit(EXIT_FAILURE);
        char indexFilename[32];
        snprintf(indexFilename, sizeof(indexFilename), "IndexChunk_%05u", chunkNum);
        fileDescrip = open(indexFilename, O_RDWR | O_CREAT | O_TRUNC, 0666);
        if ( fileDescrip == -1 )
        {
            cerr << "Could not open " << indexFilename;
            exit(EXIT_FAILURE);
        }
        /*
        Store word with posting list
        Once posting list is written out, update value to byte offset in file
        Added update func in hash table
        */
        HashTable<const std::string, size_t> & dictionary = indexChunk.get_dictionary();
        // used vector so that we didn't need to calculate size of buffer beforehand
        // Every insert is at the end of the dataBuffer
        // no reallocation since we reserved mem
        std::vector<uint8_t> dataBuffer;
        dataBuffer.reserve(8000000000UL); // 8 Gigabyte worth of bytes
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
    ~IndexFile()
    {
        if (fileDescrip >= 0)
        {
            close(fileDescrip);
        }
    }
};