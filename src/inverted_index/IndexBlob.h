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

using PostOffset = uint64_t;
using PostPosition = uint64_t;
using PostIndex = uint64_t;

static const size_t Unknown = 0;

[[nodiscard]] size_t RoundUp(size_t length, size_t boundary)
{
   // Round up to the next multiple of the boundary, which
   // must be a power of 2.
static const size_t oneless = boundary - 1,
                    mask = ~(oneless);
return (length + oneless) & mask;
}
size_t highBitsIndex(uint64_t seek, size_t seekTableCount) {
    // Maybe use this to index into seek table? don't know how to 
    size_t k = __builtin_ctzll(seekTableCount);
    return seek >> (64 - k);
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
    PostOffset postOffset;
    PostPosition location;
    SeekEntry(PostOffset postOffset, PostPosition location) : postOffset(postOffset), location(location) {}
};


class PostingListBlob {
    public:
        struct BlobHeader 
        {
            // Need to add the Posting List headers
            size_t BlobSize;
            size_t SeekTableSize;
            size_t SeekTableCount;

            BlobHeader() = default;
            BlobHeader(size_t blobSize, size_t seekTableSize, size_t seekTableCount) : BlobSize(blobSize), SeekTableSize(seekTableSize), SeekTableCount(seekTableCount) {}
        };
        BlobHeader blobHeader;

    [[nodiscard]] const SerialPost *Find(size_t seek) const
    {
        // Seek table is the first thing after the headers
        const char *seekTable = reinterpret_cast<const char *>(this) + sizeof(this->blobHeader);
        
        size_t numElems = this->blobHeader.SeekTableCount;
        // If there is a seek table
        if (numElems != 0) [[likely]]
        {
            
            

            // Gets second seek table entry
            const SeekEntry * entry = reinterpret_cast<const SeekEntry *>(seekTable) + sizeof(SeekEntry); 

            // Galloping search
            size_t i = 0;
            size_t j = 1;
            while(j < numElems && entry->location < seek)
            {
                i = j;
                j *= 2;
                entry = reinterpret_cast<const SeekEntry *>(seekTable) + j * sizeof(SeekEntry); 
            }

            // Binary search
            size_t left = i;
            size_t right = min(j, numElems);
            const SeekEntry * candidate = nullptr;

            while (left <= right)
            {
                size_t mid = left + (right - left) / 2;
                entry = reinterpret_cast<const SeekEntry *>(seekTable) + mid * sizeof(SeekEntry); 
                if (entry->location <= seek)
                {
                    candidate = entry;
                    left = mid + 1;
                }
                else
                {
                    if (mid == 0) break;
                    right = mid - 1;
                }
            }
            
            size_t postOffset = candidate->postOffset; // How far from start of posting list
            // Posting list starts after header and seek table
            const char *postsStart = reinterpret_cast<const char *>(this) + sizeof(this->blobHeader) + this->blobHeader.SeekTableSize;

            return reinterpret_cast<const SerialPost *>(postsStart + postOffset);
        }
        else 
        {
            // No seektable
            const char * start = reinterpret_cast<const char *>(this) + sizeof(this->blobHeader);
            const char * end = reinterpret_cast<const char *>(this) + this->blobHeader.BlobSize;
            uint64_t pos = 0;

            // Linear search
            while (start < end)
            {
                const SerialPost * post = reinterpret_cast<const SerialPost *>(start);
                uint64_t delta = 0;
                decodeVarint(post->delta, delta);
                pos += delta;
                if (pos > seek)
                {
                    return post;
                }
                start += post->totalLen;
            }
        }
        return nullptr;

    }
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
        size_t total = postingList.header_size();

        size_t syncInterval = CalcSyncPointsPerXPosts(postingList);
        size_t seekTableCount = floor(postingList.size() / syncInterval); 

        auto it = postingList.begin();
        auto end = postingList.end();

        // Offset of post
        PostOffset offset = 0;
        // Actual position
        PostPosition pos = 0;
        while(it != end)
        {   
            auto post = *it;
            uint64_t delta;
            decodeVarint(post.delta.get(), delta);
            pos += delta;
            size_t postSize =  SerialPost::BytesRequired(post);
            total += postSize;
            ++it;

        }
        return BlobHeader(RoundUp(total, sizeof(size_t)), RoundUp(sizeof(SeekEntry) * seekTableCount, sizeof(size_t)), seekTableCount);
    }

    static PostingListBlob* Write(PostingListBlob * plb, size_t bytes, const PostingList &postingList)
    {
        BlobHeader blobHeader = BytesRequired(postingList);
        if (blobHeader.BlobSize > bytes) [[unlikely]]
        {
            return nullptr;
        }
        // cast plb to buffer
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

        // Moves post start to after seek table if there is one
        if (seekTableCount > 0) [[likely]]
        {
            bufferPostStart = buffer + blobHeader.SeekTableSize;
        }
        
        auto it = postingList.begin();
        auto end = postingList.end();
        
        PostOffset offset = 0; // offset from posting list
        PostPosition pos = 0; // Actual location
        PostIndex index = 0; // what post we are on

        while(it != end)
        {
            auto post = *it;
            uint64_t delta;
            decodeVarint(post.delta.get(), delta);
            pos += delta;
            
            if (seekTableCount > 0 && index % syncInterval == 0) [[unlikely]]
            {
                std::memcpy(buffer, &SeekEntry(offset, pos), sizeof(SeekEntry));
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
        void *mem = std::calloc(1, blobHeader.BlobSize);
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
    private:
    PostingListBlob* blob;
    int fileDescrip;

    size_t FileSize(int f)
    {
        struct stat fileInfo;
        fstat(f, &fileInfo);
        return fileInfo.st_size;
    }
    public:
    const PostingListBlob* Blob() const
    {
        return blob;
    }
    PostingListFile(const char* filename) : blob(nullptr)
    {
        fileDescrip = open(filename, O_RDONLY);
        size_t fileSize = FileSize(fileDescrip);
        void* map = mmap(nullptr, fileSize, PROT_READ, MAP_PRIVATE, fileDescrip, 0);
        blob = reinterpret_cast<PostingListBlob*>(map);
    }
    PostingListFile(const char* filename, const PostingList &postingList) : blob(nullptr)
    {
        blob = PostingListBlob::Create(postingList);
        size_t requiredSize = blob->blobHeader.BlobSize;
        
        fileDescrip = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0666);

        ftruncate(fileDescrip, requiredSize);
        
        void* map = mmap(nullptr, requiredSize, PROT_WRITE, MAP_PRIVATE, fileDescrip, 0);
        std::memcpy(map, blob, requiredSize);
    }
    ~PostingListFile()
    {
        if (blob)
        {
            munmap(blob, blob->blobHeader.BlobSize);
        }
        if (fileDescrip >= 0)
        {
            close(fileDescrip);
        }
    }
};