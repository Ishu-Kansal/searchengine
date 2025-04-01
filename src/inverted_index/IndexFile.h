#pragma once

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

#include <cmath>
#include <cassert>
#include <cstring>
#include <cstdint>
#include <algorithm>
#include "HashTable.h"
#include "Index.h"


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
    void serializePostingLists(std::vector<uint8_t>& dataBuffer, const vector<PostingList> &listOfPostingList)
    {
        // TODO: add size of listOfPostingList?
        for (const PostingList & postingList : listOfPostingList)
        {
            // TODO: Need to serialize seek table
            pushVarint(dataBuffer, postingList.size());
            uint64_t prev = 0;
            for (const Post & entry : postingList)
            {
                const uint64_t delta = entry.location - prev;
                pushVarint(dataBuffer, delta);
                prev = entry.location;
            }
        }
    }
    void serializeChunk(const IndexChunk &indexChunk, std::vector<uint8_t>& dataBuffer)
    {
        dataBuffer.clear();

        // Serialize everything 
        const auto urlList = indexChunk.get_docs();
        const auto listOfPostingList = indexChunk.get_posting_lists();
        serializeUrlList(dataBuffer, urlList);
        serializePostingLists(dataBuffer, listOfPostingList);
        
    }
    int fileDescrip;
    size_t FileSize(int f)
    {
        struct stat fileInfo;
        fstat(f, &fileInfo);
        return fileInfo.st_size;
    }
    public:
    IndexFile(const char* filename, const IndexChunk &indexChunk)
    {
        
        fileDescrip = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
        if ( fileDescrip == -1 )
        {
            cerr << "Could not open " << filename;
            exit(EXIT_FAILURE);
        }
        // TODO: used vector so that we didn't need to calculate size of buffer beforehand
        // Every insert is at the end of the dataBuffer, 
        // so should be amortized constant time
        std::vector<uint8_t> dataBuffer;
        serializeChunk(indexChunk, dataBuffer);

        ssize_t bytesWritten = write(fileDescrip, dataBuffer.data(), dataBuffer.size());
        if (bytesWritten == -1) 
        {
            cerr << "Error writing to " << filename;
            exit(EXIT_FAILURE);
        }
        else if (static_cast<size_t>(bytesWritten) != dataBuffer.size()) 
        {
            cerr << "Incomplete write to " << filename;
            exit(EXIT_FAILURE);
        }
          
    }
    ~IndexFile()
    {
        if (fileDescrip >= 0)
        {
            close(fileDescrip);
        }
    }
};