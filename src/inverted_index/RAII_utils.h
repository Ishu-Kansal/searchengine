#pragma once

#include <sys/mman.h>
#include <unistd.h>

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