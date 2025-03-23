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

[[nodiscard]] size_t RoundUp(size_t length, size_t boundary)
{
   // Round up to the next multiple of the boundary, which
   // must be a power of 2.
static const size_t oneless = boundary - 1,
                    mask = ~(oneless);
return (length + oneless) & mask;
}

struct SerialPost {

    uint8_t totalLen;
    unsigned char flags; 
    uint8_t delta[];

    [[nodiscard]] static uint8_t BytesRequired(const Post &post)
    {
        size_t total = sizeof(post.flags) + post.numBytes + sizeof(uint8_t);
        return RoundUp(total, sizeof(size_t));
    }
    static char *Write(char *buffer, char *bufferEnd,
        const Post &post)
    {

        uint8_t totalLen = BytesRequired(post);
        if (buffer + totalLen > bufferEnd) [[unlikely]]
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

struct SeekEntry {
    uint8_t* postOffset;
    uint8_t* location;
    SeekEntry(uint8_t* postOffset, uint8_t* location) : postOffset(postOffset), location(location) {}
};


class PostingListBlob {
    public:
        struct BlobHeader 
        {
            size_t BlobSize;
            size_t SeekTableSize;
            size_t SeekTableCount;

            BlobHeader() = default;
            BlobHeader(size_t blobSize, size_t seekTableSize, size_t seekTableCount) : BlobSize(blobSize), SeekTableSize(seekTableSize), SeekTableCount(seekTableCount) {}
        };
        BlobHeader blobHeader;
    [[nodiscard]] const SerialPost *Find(size_t seek) const
    {}
    [[nodiscard]] static size_t CalcSyncPointsPerXPosts(const PostingList &postingList)
    {   
        // Finds close to optimal numSyncPoints to min disk reads
        size_t numPosts = postingList.size();
        if(numPosts < 1024) {return 1;}
        // Need to change
        return ceil(sqrt(numPosts));
    }
    [[nodiscard]] static BlobHeader BytesRequired(const PostingList &postingList)
    {
        // Calculates the num of bytes required for PostingListBlob
        // Last size_t is for size of seekTable and num of elems in it
        size_t total = postingList.header_size() + sizeof(size_t);
        size_t seekTableTotal = sizeof(size_t) + sizeof(size_t);

        size_t syncInterval = CalcSyncPointsPerXPosts(postingList);
        size_t seekTableCount = 0;
        auto it = postingList.begin();
        auto end = postingList.end();

        // Offset of post
        uint64_t offset = 0;
        // Actual position
        uint64_t pos = 0;
        uint64_t index=  0;
        while(it != end)
        {   
            auto post = *it;
            uint64_t delta;
            decodeVarint(post.delta.get(), delta);
            pos += delta;
            if (syncInterval > 1 && index % syncInterval == 0) [[unlikely]]
            {
                // Adds the num of bytes of an aligned encoded offset and encoded pos
                size_t seekTableEntry = RoundUp(SizeOfDelta(offset) + SizeOfDelta(pos), sizeof(size_t));
                total += seekTableEntry;
                seekTableTotal += seekTableEntry;
                ++seekTableCount;
            
            }
            size_t postSize =  SerialPost::BytesRequired(post);
            total += postSize;
            offset += postSize;
            ++it;
            ++index;

        }
        return BlobHeader(RoundUp(total, sizeof(size_t)), RoundUp(seekTableTotal, sizeof(size_t)), seekTableCount);
    }

    static PostingListBlob* Write(PostingListBlob * plb, size_t bytes, const PostingList &postingList)
    {
        BlobHeader blobHeader = BytesRequired(postingList);
        if (blobHeader.BlobSize > bytes) [[unlikely]]
        {
            return nullptr;
        }

        char *buffer = reinterpret_cast<char *>(plb);
        char *bufferEnd = buffer + bytes;

        // writes header information to buffer
        std::memcpy(buffer, &plb->blobHeader, sizeof(plb->blobHeader));
        buffer += sizeof(plb->blobHeader);

        // Reserves buffer space at start for seekTable   
        size_t syncInterval = CalcSyncPointsPerXPosts(postingList);
        // size_t seekTableCount = floor(postingList.size() / syncInterval); 
        char * bufferPostStart = buffer;
        size_t seekTableCount = plb->blobHeader.SeekTableCount;
        if (seekTableCount > 0) [[likely]]
        {
            bufferPostStart = buffer + blobHeader.SeekTableSize;
        }
        
        auto it = postingList.begin();
        auto end = postingList.end();
        
        uint64_t offset = 0;
        uint64_t pos = 0;
        uint64_t index = 0;
        while(it != end)
        {
            auto post = *it;
            uint64_t delta;
            decodeVarint(post.delta.get(), delta);
            pos += delta;
            
            if (seekTableCount > 0 && index % syncInterval == 0) [[unlikely]]
            {
                size_t offsetSize = SizeOfDelta(offset);
                size_t posSize = SizeOfDelta(pos);
                uint8_t varOffset[offsetSize];
                uint8_t varPos[posSize];

                encodeVarint(offset, varOffset, offsetSize);
                encodeVarint(pos, varPos, posSize);
                
                std::memcpy(buffer, &SeekEntry(varOffset, varPos), sizeof(SeekEntry));
                buffer += sizeof(SeekEntry);
            }
            size_t postSize = SerialPost::BytesRequired(post);
            SerialPost::Write(bufferPostStart, bufferEnd, post);
            offset += postSize;
            ++it;
            ++index;
        }
        return plb;
    }

    [[nodiscard]] static PostingListBlob *Create(const PostingList &postingList) {
        BlobHeader blobHeader = BytesRequired(postingList);
        void *mem = operator new(blobHeader.BlobSize);
        std::memset(mem, 0, blobHeader.BlobSize);
        PostingListBlob * plb = new(mem) PostingListBlob();
        plb->blobHeader = blobHeader;
        plb = Write(plb, blobHeader.BlobSize, postingList);

        return plb;
    }

    static void Discard(PostingListBlob * plb)
    {
        operator delete(plb);
    }
};

class PostingListFile {


};