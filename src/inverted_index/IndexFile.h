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
#include "../../HashTable/HashTableStarterFiles/HashTable.h"
#include "../../HashTable/HashTableStarterFiles/HashBlob.h"
#include "Index.h"
#include "../../HashTable/HashTableStarterFiles/HashTable.h"

class IndexFile {
    private:
    void pushVarint(std::vector<uint8_t>& dataBuffer, uint64_t value) {

        uint8_t buf[10];
        uint8_t* ptr = encodeVarint(value, buf);
        dataBuffer.insert(dataBuffer.end(), buf, ptr);

    }
    void serializeUrlList(std::vector<uint8_t>& dataBuffer, const std::vector<Doc> &urlList)
    {
        // Every insert is at the end of the dataBuffer, so should be constant time
        pushVarint(dataBuffer, urlList.size());
        for (auto & doc : urlList)
        {
            pushVarint(dataBuffer, doc.staticRank);
            pushVarint(dataBuffer, doc.url.size());
            dataBuffer.insert(dataBuffer.end(), doc.url.begin(), doc.url.end());
        }
    }
    void serializePostingLists(std::vector<uint8_t>& dataBuffer, const vector<PostingList> &listOfPostingList, HashTable<const std::string, size_t> & dictionary)
    {
        // TODO: add size of listOfPostingList?
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
            pushVarint(dataBuffer, postingList.size());
            SeekTable seekTable;
            uint64_t prev = 0;
            size_t index = 0;
            for (const Post & entry : postingList)
            {
                /*
                if (index % SeekTable::OFFSET == 0)
                {
                    seekTable.addEntry(dataBuffer.size(), entry.location);
                }
                */

                const uint64_t delta = entry.location - prev;
                pushVarint(dataBuffer, delta);
                prev = entry.location;
                ++index;
            }
            //const size_t headerSize = seekTable.header_size();
            //const size_t dataSize = seekTable.data_size();
            //std::vector<uint8_t> seekBuffer;
            //seekBuffer.resize(headerSize + dataSize);
            //uint8_t *bufPtr = seekBuffer.data();
            //bufPtr = SeekTable::encode_table(bufPtr, seekTable);
            //dataBuffer.insert(dataBuffer.end(), seekBuffer.begin(), seekBuffer.end());
        }
    }
    void serializeChunk(const IndexChunk &indexChunk, std::vector<uint8_t>& dataBuffer, HashTable<const std::string, size_t> & dictionary)
    {
        dataBuffer.clear();

        // Serialize everything 
        const auto urlList = indexChunk.get_urls();
        const auto listOfPostingList = indexChunk.get_posting_lists();
        serializeUrlList(dataBuffer, urlList);
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
        Method 1:

        Don't need to reconstruct hash table: hash table has offset of word in vector of posting lists (listOfpostingList), use another vector (postingListOffsetVector) to store byte offset. Simplest way with our current structure.

        std::vector<size_t> postingListOffsetVector;
        postingListOffsetVector.reserve(indexChunk.get_posting_lists().size());
        
        */ 
        /*
        Method 2:
        Store word with posting list
        Once posting list is written out, update value to byte offset in file
        Added update func in hash table
        */
        HashTable<const std::string, size_t> & dictionary = indexChunk.get_dictionary();
        // TODO: used vector so that we didn't need to calculate size of buffer beforehand
        // Every insert is at the end of the dataBuffer, 
        // so should be amortized constant time (constant since we reserved 1 gb)
        std::vector<uint8_t> dataBuffer;
        // This is only for if we have more than one index chunk for each machine
        dataBuffer.reserve(1000000000UL); // Gigabyte worth of bytes
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
    private:
        int fileDescrip;
        size_t FileSize(int f)
        {
            struct stat fileInfo;
            fstat(f, &fileInfo);
            return fileInfo.st_size;
        }
};