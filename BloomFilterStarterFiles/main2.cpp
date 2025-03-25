#include "BloomFilter.h"
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <cstdio>

int main()
{
    const char* filename = "bf.dat";

    int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if(fd < 0)
    {
        perror("open");
        return 1;
    }

    Bloomfilter bf1(1000, 0.01);
    bf1.insert("hello");
    bf1.insert("world");
    std::cout << "bf1.contains(\"hello\"): " << bf1.contains("hello") << std::endl;
    std::cout << "bf1.contains(\"world\"): " << bf1.contains("world") << std::endl;
    std::cout << "bf1.contains(\"test\"): "  << bf1.contains("test")  << std::endl;
    std::cout << "bf1.contains(\"tes\"): "  << bf1.contains("test")  << std::endl;
    std::cout << "bf1.contains(\"te\"): "  << bf1.contains("test")  << std::endl;
    std::cout << "bf1.contains(\"t\"): "  << bf1.contains("test")  << std::endl;
    bf1.writeBFtoFile(fd);

    lseek(fd, 0, SEEK_SET);

    Bloomfilter bf2(fd);
    close(fd);

    std::cout << "bf2.contains(\"hello\"): " << bf2.contains("hello") << std::endl;
    std::cout << "bf2.contains(\"world\"): " << bf2.contains("world") << std::endl;
    std::cout << "bf2.contains(\"test\"): "  << bf2.contains("test")  << std::endl;
    std::cout << "bf1.contains(\"tes\"): "  << bf1.contains("test")  << std::endl;
    std::cout << "bf1.contains(\"te\"): "  << bf1.contains("test")  << std::endl;
    std::cout << "bf1.contains(\"t\"): "  << bf1.contains("test")  << std::endl;

    return 0;
}