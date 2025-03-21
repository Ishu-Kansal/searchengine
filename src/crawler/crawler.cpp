#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/md5.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <queue>
#include <random>
#include <string>
#include <utility>

#include "../../BloomFilterStarterFiles/BloomFilter.h"
#include "../../HtmlParser/HtmlParser.h"
#include "../../utils/pthread_lock_guard.h"
// #include "../inverted_index/Index.h"

constexpr uint32_t MAX_PROCESSED = 5;
constexpr uint32_t TOP_K_ELEMENTS = 5000;
constexpr uint32_t NUM_RANDOM = 10000;

uint32_t STATIC_RANK = 0;  // temp global variable

std::queue<std::string> explore_queue{};
std::vector<std::pair<std::string, uint32_t>> links_vector;
Bloomfilter bf(100000, .0001);  // Temp size and false pos rate
sem_t *queue_sem{};

pthread_mutex_t queue_lock{};
pthread_mutex_t output_lock{};
uint32_t num_processed{};
std::mt19937 mt{std::random_device{}()};

int get_and_parse_url(const char *url, int fd) {
    static const char *const proc = "../../LinuxGetUrl/LinuxGetSSL";
    std::cout << "running get and parse URL" << " fd: " << fd << " url " << url
              << std::endl;
    pid_t pid = fork();
    if (pid == 0) {
        dup2(fd, STDOUT_FILENO);  // redirect STDOUT to output file
        int t = execl(proc, proc, url, NULL);
        if (t == -1) {
            std::clog << "Failed to execute for url: " << url << '\n';
            return -1;
        }
    } else {
        int status;
        waitpid(pid, &status, 0);
        if (status != 0) {
            std::clog << "Failed reading for url: " << url << '\n';
        }
        return status;
    }

    // dup2(fd, STDOUT_FILENO);  // redirect STDOUT to output file
    // std::cout << "Ran DUP" << std::endl;
    // int t = execlp(proc, "LinuxGetSSL", url, NULL);
    // std::cout << "Ran exec" << std::endl;
    // if (t == -1) {
    //     std::clog << "Failed to execute for url: " << url << '\n';
    //     return -1;
    // }

    return 0;
}

int get_file_size(int fd) {
    struct stat buf;
    fstat(fd, &buf);
    return buf.st_size;
}

int partition(int left, int right, int pivot_index) {
    int pivot_rank = links_vector[pivot_index].second;

    // Move the pivot to the end
    std::swap(links_vector[pivot_index], links_vector[right]);

    // Move all less ranked elements to the left
    int store_index = left;
    for (int i = left; i <= right; i++) {
        if (links_vector[i].second < pivot_rank) {
            std::swap(links_vector[store_index], links_vector[i]);
            store_index += 1;
        }
    }

    // Move the pivot to its final place
    std::swap(links_vector[right], links_vector[store_index]);

    return store_index;
}

void quickselect(int left, int right, int k) {
    std::uniform_int_distribution<> gen(left, right);
    int pivot_index = gen(mt);

    // Find the pivot position in a sorted list
    pivot_index = partition(left, right, pivot_index);

    // If the pivot is in its final sorted position
    if (k == pivot_index) {
        return;
    } else if (k < pivot_index) {
        // go left
        quickselect(left, pivot_index - 1, k);
    } else {
        // go right
        quickselect(pivot_index + 1, right, k);
    }
}

void fill_queue() {
    uint32_t links_vector_size = links_vector.size();

    if (explore_queue.empty() && links_vector_size > 10000) {
        // Establish range for uniform random num gen
        // Range is from 0 to the last element in the vector
        std::uniform_int_distribution<> gen{0, links_vector_size - 1};

        // Generates N random elements and moves them to the end
        for (size_t t = links_vector_size - 1;
             t > links_vector_size - NUM_RANDOM; t--) {
            std::swap(links_vector[gen(mt)], links_vector[t]);
        }

        // Sorts the last N elements of the vector
        quickselect(links_vector_size - NUM_RANDOM, links_vector_size - 1,
                    TOP_K_ELEMENTS);

        // Takes last K from vector and adds its to queue
        for (size_t i = links_vector_size - 1;
             i > links_vector_size - TOP_K_ELEMENTS; --i) {
            explore_queue.push(std::move(links_vector[i].first));
            links_vector.pop_back();
        }
    }
}
#include <pthread.h>

#include <queue>
#include <string>
#include <vector>

std::string get_next_url() {
    pthread_mutex_lock(&queue_lock);

    size_t links_vector_size = links_vector.size();
    std::string url;

    if (!explore_queue.empty()) {
        url = std::move(explore_queue.front());
        explore_queue.pop();
    } else if (links_vector_size > 10000) {
        fill_queue();
        url = std::move(explore_queue.front());
        explore_queue.pop();
    } else {
        url = std::move(links_vector[links_vector_size - 1].first);
        links_vector.pop_back();
    }

    pthread_mutex_unlock(&queue_lock);

    return url;
}

int count = 0;

void *runner(void *) {
    while (num_processed < MAX_PROCESSED) {
        // sem_wait(queue_sem);

        std::string url = get_next_url();

        std::cout << "Running URL " << url << std::endl;

        const std::string fileName =
            "../files/file" + std::to_string(count) + ".txt";
        count++;

        // this isn't working rn bc if u open a file that already exists, u can
        // do that
        int outputFd =
            open(fileName.data(), O_CREAT | O_TRUNC | O_WRONLY | O_EXCL, 0777);
        if (outputFd == -1) {
            std::clog << "URL already processed for url: " << url << '\n';
            continue;
        }

        std::cout << "Here" << std::endl;

        int status = get_and_parse_url(url.data(), outputFd);

        if (status != 0) {
            std::cout << "here " << status << '\n';
            close(outputFd);
            continue;
        }

        std::cout << "Here2" << std::endl;

        const int len = get_file_size(outputFd);

        std::cout << "Here3" << std::endl;

        const char *fileData =
            (char *)mmap(nullptr, len, O_RDONLY, PROT_READ, outputFd, 0);
        if (fileData == MAP_FAILED) {
            std::clog << "Failed to process url: " << url << '\n';
            close(outputFd);
            continue;
        }
        std::cout << "Here4" << std::endl;
        try {
            HtmlParser parser(fileData, len);

            std::cout << "Here5" << std::endl;
            ++num_processed;
            {
                pthread_lock_guard guard{queue_lock};
                for (auto &link : parser.links) {
                    std::string nextURL = std::move(link.URL);
                    if (nextURL[0] == '/') {
                        if (url.back() == '/') {
                            nextURL = url.substr(0, url.size() - 1) + nextURL;
                        } else {
                            nextURL = url + nextURL;
                        }
                    } else if (nextURL[0] == '?') {
                        nextURL = url + nextURL;
                    }
                    if (!bf.contains(nextURL)) {
                        bf.insert(nextURL);
                        links_vector.push_back(
                            {std::move(nextURL), STATIC_RANK++});
                        //    sem_post(queue_sem);
                    }
                }
            }
            // add to index (single threaded so far)
            std::cout << "Found " << parser.words.size() << " words"
                      << std::endl;
            // uint64_t pos = 0;
            // IndexChunk chunk;
            // chunk.add_url(std::move(url));
            // for (auto &word : parser.words)
            // {
            //   // add_word should get dictionary entry for word (create if
            //   necessary)
            //   // and append new posting pos to word's postings list
            //   chunk.add_word(word, pos);
            //   pos++;
            // }
        } catch (...) {
        }
        munmap((void *)fileData, len);
        close(outputFd);
    }
    return NULL;
}

int main(int argc, char **argv) {
    std::ofstream logging_file{"log.txt"};
    std::clog.rdbuf(logging_file.rdbuf());

    links_vector.push_back(
        {"https://en.wikipedia.org/wiki/University_of_Michigan", 0});

    pthread_mutex_init(&queue_lock, NULL);
    queue_sem = sem_open("/crawler_semaphore", O_CREAT);
    if (queue_sem == SEM_FAILED) {
        std::clog << "Failed to open semaphore: " << strerror(errno)
                  << std::endl;
        exit(EXIT_FAILURE);
    }

    const static int num_threads = 1;  // start small
    pthread_t threads[num_threads];
    for (int i = 0; i < num_threads; ++i) {
        pthread_create(threads + i, NULL, runner, NULL);
    }
    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], NULL);
    }

    sem_close(queue_sem);
    pthread_mutex_destroy(&queue_lock);
}
