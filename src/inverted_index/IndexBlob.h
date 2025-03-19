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

#include "HashTable.h"
#include "Index.h"

static const size_t Unknown = 0;

size_t RoundUp(size_t length, size_t boundary)
{
   // Round up to the next multiple of the boundary, which
   // must be a power of 2.
static const size_t oneless = boundary - 1,
                    mask = ~(oneless);
return (length + oneless) & mask;
}

struct SerialPost {

    static uint8_t BytesRequired(const Post &post)
    {
        size_t total = sizeof(post.flags) + post.numBytes + sizeof(uint8_t);
        return RoundUp(total, sizeof(size_t));
    }
    static char *Write(char *buffer, char *bufferEnd,
        const Post &post)
    {

        uint8_t totalLen = BytesRequired(post);
        if (buffer + totalLen > bufferEnd)
        {
            return buffer;
        }
        // Writes total len to buffer
        std::memcpy(buffer, &totalLen, sizeof(totalLen));
        buffer += sizeof(totalLen);

        // Writes flags to buffer
        std::memcpy(buffer, &post.flags, sizeof(post.flags));
        buffer += sizeof(post.flags);

        // Writes delta to buffer
        std::memcpy(buffer, &post.delta, post.numBytes);
        buffer += post.numBytes;

        return buffer;
    }
};

struct SeekObject {
    size_t postOffset;
    size_t location;
    SeekObject(size_t postOffset, size_t location) : postOffset(postOffset), location(location) {}
};
class PostingListBlob {
    public:
        size_t BlobSize;

    static size_t CalcSyncPointsPerXPosts(PostingList &postingList)
    {   
        // Finds close to optimal numSyncPoints to min disk reads
        size_t numPosts = postingList.size();
        if(numPosts < 1024) {return 1;}
        // Need to change
        return ceil(sqrt(numPosts));
    }
    static size_t BytesRequired(PostingList &postingList)
    {
        // Calculates the num of bytes required for PostingListBlob
        size_t total = postingList.header_size();
        // Adds seekTable bytes to total
        // TODO align?
        size_t syncInterval = CalcSyncPointsPerXPosts(postingList);
        size_t seekTableSize = ceil(postingList.size() / syncInterval); 
        total += RoundUp(seekTableSize * sizeof(SeekObject), sizeof(size_t));

        auto it = postingList.begin();
        auto end = postingList.end();
        
        while(it != end)
        {   
            total += SerialPost::BytesRequired(*it);
            ++it;
        }
        return RoundUp(total, sizeof(size_t));
    }

    static PostingListBlob* Write(PostingListBlob * plb, size_t bytes, PostingList &postingList)
    {
        size_t size = BytesRequired(postingList);
        if (size > bytes)
        {
            return nullptr;
        }

        char *buffer = reinterpret_cast<char *>(plb);
        char *bufferEnd = buffer + bytes;

        // TODO need to write header information to buffer

        // Reserves buffer space at start for seekTable   
        size_t syncInterval = CalcSyncPointsPerXPosts(postingList);
        size_t seekTableSize = floor(postingList.size() / syncInterval);
        char * bufferPostStart = buffer;

        if (syncInterval > 1)
        {
            bufferPostStart = buffer + RoundUp(seekTableSize * sizeof(SeekObject), sizeof(size_t));
        }
        
        auto it = postingList.begin();
        auto end = postingList.end();
        
        uint64_t index = 0;
        uint64_t pos = 0;

        while(it != end)
        {
            auto post = *it;
            uint64_t delta;
            decodeVarint(post.delta.get(), delta);
            pos += delta;
            
            if (syncInterval > 1 && index % syncInterval == 0)
            {
                std::memcpy(buffer, &SeekObject(index, pos), sizeof(SeekObject));
                buffer += sizeof(SeekObject);
            }
            SerialPost::Write(bufferPostStart, bufferEnd, post);
            index++;
        }
        return plb;
    }

    static PostingListBlob *Create(PostingList &postingList) {
        size_t bytes = BytesRequired(postingList);
        void *mem = operator new(bytes);
        std::memset(mem, 0, bytes);
        PostingListBlob * plb = new(mem) PostingListBlob();
        plb->BlobSize = bytes;
        plb = Write(plb, bytes, postingList);

        return plb;
    }
    
    static void Discard(PostingListBlob * plb)
    {
        operator delete(plb);
    }
};

class PostingListFile {


};