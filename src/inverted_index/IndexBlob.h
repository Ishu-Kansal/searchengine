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
        size_t total = sizeof(title) + sizeof(bold) + sizeof(post.array_size);
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

        std::memcpy(buffer, &post.title, sizeof(post.title));
        buffer += sizeof(post.title);

        std::memcpy(buffer, &post.bold, sizeof(post.bold));
        buffer += sizeof(post.bold);

        // Will change to delta
        std::memcpy(buffer, &post.delta, sizeof(post.array_size));
        buffer += sizeof(post.array_size);

        return buffer;
    }
};

class SerialPostingList {
    static void CreateSeekTable(PostingList &postingList)
    {   
        size_t num_posts = postingList.size();
        size_t ceil(sqrt(num_posts));

    }
    static size_t BytesRequired(PostingList &postingList)
    {
        size_t total = postingList.header_size();
        auto it = postingList.begin();
        auto end = postingList.end();
        while(it != end)
        {   
            total += SerialPost::BytesRequired(*it);
            ++it;
        }
        return RoundUp(total, sizeof(size_t));
    }

};