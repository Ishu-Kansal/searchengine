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
#include <regex>
#include <string>
#include <utility>
#include <vector>

#include "../../BloomFilterStarterFiles/BloomFilter.h"
#include "../../HtmlParser/HtmlParser.h"
#include "../../utils/pthread_lock_guard.h"
#include "sockets.cpp"
// #include "../inverted_index/Index.h"

constexpr uint32_t MAX_PROCESSED = 500;
constexpr uint32_t TOP_K_ELEMENTS = 5000;
constexpr uint32_t NUM_RANDOM = 10000;

const static int NUM_THREADS = 1;  // start small

uint32_t STATIC_RANK = 0;  // temp global variable

std::queue<std::string> explore_queue{};
std::vector<std::pair<std::string, uint32_t>> links_vector;
Bloomfilter bf(100000, 0.0001);  // Temp size and false pos rate

pthread_mutex_t queue_lock{};
uint32_t num_processed{};
std::mt19937 mt{std::random_device{}()};

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

std::string get_next_url() {
    pthread_lock_guard guard{queue_lock};

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

    return url;
}

std::string getHostFromUrl(const std::string& url) {
    std::regex urlRe("^.*://([^/?:]+)/?.*$");
    return std::regex_replace(url, urlRe, "$1");
}

void* runner(void*) {
    while (num_processed < MAX_PROCESSED) {
        // Get the next url to be processed
        std::string url = get_next_url();

        // Print the url that is being processed
        std::cout << url << std::endl;

        // Get the html code for the url
        std::string html;
        int status = getHTML(url, html);

        // Continue if html code was not retrieved
        if (status != 0) {
            std::cout << "Status " << status << std::endl;
            std::cout << "Could not retrieve HTML\n" << std::endl;
            continue;
        }

        // Parse the html code
        HtmlParser parser(html.data(), html.size());
        num_processed++;

        // ------------------------------------------------------------------
        // TODO: The code to add the words from parser to the index goes here

        // ------------------------------------------------------------------

        // Process links found by the parser
        {
            pthread_lock_guard guard{queue_lock};
            for (auto& link : parser.links) {
                std::string next_url = std::move(link.URL);

                // Ignore links that begin with '#' or '?'
                if (next_url[0] == '#' || next_url[0] == '?') {
                    continue;
                }

                // If link starts with '/', add the domain to the beginning of
                // it
                if (next_url[0] == '/') {
                    next_url =
                        url.substr(0, 8) + getHostFromUrl(url) + next_url;
                }

                // If link has not been seen before, add it to the bf and links
                // vector
                if (!bf.contains(next_url)) {
                    bf.insert(next_url);
                    links_vector.push_back(
                        {std::move(next_url), STATIC_RANK++});
                }
            }
        }

        // --------------------------------------------------
        // For debugging (not needed for crawler to function)
        std::string filename =
            "../files/file" + std::to_string(num_processed) + ".txt";
        std::ofstream output_file(filename);

        if (!output_file) {
            std::cerr << "Error opening file!\n" << std::endl;
            continue;
        }

        output_file << url << "\n\n";
        output_file << parser.words.size() << " words\n";
        output_file << parser.links.size() << " links\n\n";
        output_file << html;

        output_file.close();
        // --------------------------------------------------

        std::cout << '\n';
    }

    return NULL;
}

int main(int argc, char** argv) {
    links_vector.push_back(
        //{"https://en.wikipedia.org/wiki/University_of_Michigan", 0}
        {"https://www.yahoo.com", 0});

    pthread_mutex_init(&queue_lock, NULL);

    pthread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(threads + i, NULL, runner, NULL);
    }
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&queue_lock);
}