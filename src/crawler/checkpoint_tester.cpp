#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <queue>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
void test1();
void testdata();

std::queue<std::string> explore_queue{};
std::vector<std::pair<std::string, uint32_t>> links_vector;

uint32_t stou32(const std::string& str) {
    if (str.empty()) {
        throw std::invalid_argument("String is empty");
    }

    char* end;
    unsigned long long result = std::strtoull(str.c_str(), &end, 10);

    if (*end != '\0') {
        throw std::invalid_argument("Invalid characters in String");
    }

    if (result > UINT32_MAX) {
        throw std::out_of_range("Value out of range for uint32_t");
    }

    return static_cast<uint32_t>(result);
}

void printqueue() {
    std::cout << "\n----- printing queue ----- size: " << explore_queue.size()
              << '\n';
    int size = explore_queue.size();
    for (size_t i = 0; i < size; i++) {
        std::cout << explore_queue.front() << '\n';
        explore_queue.pop();
    }
}

void writeQueueToFile(int handle) {
    int size = explore_queue.size();
    for (size_t i = 0; i < size; i++) {
        std::string entry = explore_queue.front() + "\n";
        write(handle, entry.data(), entry.size());
        explore_queue.pop();
    }
}

void printvector() {
    std::cout << "\n----- printing vector -----\n";
    for (size_t i = 0; i < links_vector.size(); i++) {
        std::cout << links_vector[i].first << ' ' << links_vector[i].second
                  << '\n';
    }
}

void writeVectorToFile(int handle) {
    for (size_t i = 0; i < links_vector.size(); i++) {
        char buf[12];
        sprintf(buf, "%u", links_vector[i].second);
        std::string pair_to_str = links_vector[i].first + " " + buf + "\n";
        write(handle, pair_to_str.c_str(), pair_to_str.size());
    }
}

// generates filename in format: checkpoint-MM-DD::HH::mm::ss
std::string generateoutfilename() {
    time_t currtime;
    struct tm* timeinfo;
    time(&currtime);
    timeinfo = localtime(&currtime);

    char buf[12];

    std::string outfilename = "checkpoint-";
    // year stuff
    //  sprintf(buf, "%d", timeinfo->tm_year + 1900);
    //  outfilename += buf;
    //  outfilename += '-';
    if (timeinfo->tm_mon + 1 < 10) {
        outfilename += '0';
    }
    sprintf(buf, "%d", timeinfo->tm_mon + 1);
    outfilename += buf;
    outfilename += '-';
    sprintf(buf, "%d", timeinfo->tm_mday);
    if (timeinfo->tm_mday + 1 < 10) {
        outfilename += '0';
    }
    outfilename += buf;
    outfilename += "::";
    sprintf(buf, "%d", timeinfo->tm_hour);
    if (timeinfo->tm_hour + 1 < 10) {
        outfilename += '0';
    }
    outfilename += buf;
    outfilename += ':';
    sprintf(buf, "%d", timeinfo->tm_min);
    if (timeinfo->tm_min + 1 < 10) {
        outfilename += '0';
    }
    outfilename += buf;
    outfilename += ':';
    if (timeinfo->tm_sec + 1 < 10) {
        outfilename += '0';
    }
    sprintf(buf, "%d", timeinfo->tm_sec);
    outfilename += buf;

    return outfilename;
}

void generatefile() {
    // use this in prod
    // std::string filename = generateoutfilename();
    // temp filename
    std::string filename = "temp";
    std::string path = "./checkpoints/" + filename + ".txt";
    int outfile = open(path.data(), O_CREAT | O_WRONLY | O_TRUNC, 0777);
    write(outfile, "@@@queue\n", 9);
    writeQueueToFile(outfile);
    write(outfile, "@@@vector\n", 10);
    writeVectorToFile(outfile);
    close(outfile);
}

void startup(const std::string& filename) {
    std::string path = "./checkpoints/" + filename;
    int cpfilehandle = open(path.data(), O_RDONLY);
    if (cpfilehandle == -1) {
        switch (errno) {
            case EACCES:
                std::cerr << "error opening checkpoint file: " << filename
                          << " [permission denied]\n";
                return;
            case EFAULT:
                std::cerr << "error opening checkpoint file: " << filename
                          << " [path not found]\n";
                return;
            default:
                std::cerr << "error opening checkpoint file: " << filename
                          << " [error]\n";
                return;
        }
    }

    struct stat buf;
    fstat(cpfilehandle, &buf);

    const int len = buf.st_size;
    const char* cpfile =
        (const char*)mmap(0, len, PROT_READ, MAP_PRIVATE, cpfilehandle, 0);

    if (cpfile == MAP_FAILED) {
        std::cerr << "Failed to mmap checkpont file " << cpfile << " \n";
        close(cpfilehandle);
        return;
    }

    bool readingQueue = false;
    bool readingVector = false;
    const char* line = cpfile;

    const char* cptr = line;
    while (line < cpfile + len) {
        cptr = line;
        while (*cptr != '\n') {
            // std::cout << *cptr;
            cptr++;
        }
        cptr += 1;
        if (cptr - line < 2) {
            line++;
            continue;
        }
        std::string entry = std::string(line, cptr - 1);
        if (entry == "@@@queue") {
            readingQueue = true;
            readingVector = false;
        } else if (entry == "@@@vector") {
            readingVector = true;
            readingQueue = false;
        } else {
            if (readingQueue) {
                explore_queue.push(entry);
            } else if (readingVector) {
                int space_split = entry.find(' ');
                std::string url = entry.substr(0, space_split);
                uint32_t rank;
                try {
                    rank = stou32(entry.substr(space_split + 1));
                    links_vector.push_back(
                        std::pair<std::string, uint32_t>(url, rank));

                } catch (...) {
                    links_vector.push_back(
                        std::pair<std::string, uint32_t>(url, 1));
                }
            }
        }
        line = cptr;
    }

    munmap((void*)cpfile, len);
    close(cpfilehandle);
}

int main(int argc, char** argv) {
    if (argc > 2)
        std::cerr
            << "Usage: ./<program_name> (optional)<checkpoint filename>\n";
    if (argc == 2) {
        startup(std::string(argv[1]));
    }

    // test1();
    // printvector();
    testdata();

    // generatefile();

    // printqueue();
    // printvector();
}

void test1() {
    int failon = 5;
    int i = 0;
    while (i < 10) {
        if (i < failon) {
            std::cout << "working " << i << '\n';
            explore_queue.pop();
        }
        i++;
    }
    generatefile();
}

void testdata() {
    int failon = 3;
    int i = 0;
    try {
        while (i < 10) {
            if (i < failon) {
                std::cout << "working " << i << '\n';
                i++;
                explore_queue.pop();
            } else {
                throw std::logic_error("went over limit");
            }
        }
    } catch (...) {
        generatefile();
    }
}
