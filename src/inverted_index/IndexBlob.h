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

#include "HashTable.h"
#include "Index.h"
#include <cmath>
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

    bool title;
    bool bold;

    static size_t BytesRequired(const Post &post)
    {
        size_t total = sizeof(title) + sizeof(bold) + sizeof(post.numBytes);
        return RoundUp(total, sizeof(size_t));
    }
    static char *Write(char *buffer, char *bufferEnd,
        const Post post)
    {
        size_t len = BytesRequired(post);
        if (buffer + len > bufferEnd)
        {
            return buffer;
        }
        // Writes total len to buffer
        std::memcpy(buffer, &len, sizeof(len));
        buffer += sizeof(len);

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
};
class PostingListBlob {
    static size_t CalcNumSyncPoints(PostingList &postingList)
    {   
        // Finds close to optimal numSyncPoints to min disk reads
        size_t numPosts = postingList.size();
        size_t numSyncPoints = ceil(sqrt(numPosts));
        return numSyncPoints;
    }
    static size_t BytesRequired(PostingList &postingList)
    {
        // Calculates the num of bytes required for PostingListBlob
        size_t total = postingList.header_size();
        // Adds seekTable bytes to total
        // TODO align?
        total += CalcNumSyncPoints(postingList) * sizeof(SeekObject);

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

    }

    static PostingListBlob *Create(PostingList &postingList) {
        size_t bytes = BytesRequired(postingList);
        void *mem = operator new(bytes);
        std::memset(mem, 0, bytes);
        PostingListBlob * plb = new(mem) PostingListBlob();
        size_t numSyncPoints = CalcNumSyncPoints(postingList);
        plb->seekTable = new SeekObject[numSyncPoints];
        plb->Write(plb, bytes, postingList);

        return plb;
    }
    private:
    SeekObject * seekTable = nullptr;
};

class PostingListFile {


};