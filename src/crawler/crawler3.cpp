#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/md5.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <chrono>
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
#include "../../utils/cstring_view.h"
#include "../../utils/pthread_lock_guard.h"
#include "../../utils/socket_wrapper.h"
#include "../inverted_index/Index.h"
#include "../inverted_index/IndexFile.h"
#include "../ranker/rank.h"
#include "constants.h"
#include "sockets.h"

struct ChunkBlock {
    IndexChunk chunk;
    pthread_mutex_t lock;
    sem_t* sem;
};
// Get error when i try to use cstring_view
static constexpr std::string_view APOS_ENTITY = "&#039;";
static constexpr std::string_view APOS_ENTITY2 = "&apos;";
static constexpr std::string_view HTML_ENTITY = "&#";

static constexpr std::string_view UNWANTED_NBSP = "&nbsp";
static constexpr std::string_view UNWANTED_LRM = "&lrm";
static constexpr std::string_view UNWANTED_RLM = "&rlm";
static constexpr size_t MAX_WORD_LENGTH = 50;

constexpr uint32_t MAX_PROCESSED = 100'000;
constexpr uint32_t NUM_CHUNKS = 1;

IndexChunk chunk{};

const static int NUM_THREADS = 256;  // start small

uint32_t STATIC_RANK = 0;  // temp global variable

std::queue<std::string> explore_queue{};
std::vector<std::pair<std::string, uint32_t>> links_vector;

pthread_mutex_t queue_lock{};
pthread_mutex_t chunk_lock{};
pthread_mutex_t cout_lock{};
sem_t* queue_sem;
uint32_t num_processed{};
std::mt19937 mt{std::random_device{}()};

int get_socket(int timeout = 5) {
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(SERVER_PORT);             // Port number
    address.sin_addr.s_addr = inet_addr("127.0.0.1");  // Server IP

    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == -1) return -1;
    int res = connect(sock, (struct sockaddr*)&address, sizeof(address));
    if (res == -1) return -1;
    struct timeval tv;
    tv.tv_sec = timeout;  // 5 seconds
    tv.tv_usec = 0;
    res =
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    if (res == -1) return -1;
    return sock;
}

std::string get_string() {
    const int sock = get_socket(5);
    if (sock == -1) return "";
    SocketWrapper _{sock};
    int res = send(sock, &GET_COMMAND, sizeof(GET_COMMAND), 0);
    if (res == -1) return "";
    size_t header = 0;
    if (recv(sock, &header, sizeof(header), 0) <= 0 || header == 0) return "";
    std::string result(header, 0);
    if (recv(sock, result.data(), result.size(), MSG_WAITALL) <= 0) return "";
    return result;
}

void add_url(cstring_view url, uint64_t rank) {
    int sock = get_socket(1);
    if (sock == -1) return;
    SocketWrapper _{sock};
    header_t header = sizeof(rank) + url.size();
    send(sock, &header, sizeof(header), 0);
    send(sock, &rank, sizeof(rank), 0);
    send(sock, url.data(), url.size(), 0);
}

bool isEnglish(const std::string& s) {
    for (const auto& ch : s) {
        if (static_cast<unsigned char>(ch) > 127) {
            return false;
        }
    }
    return true;
}
void cleanString(std::string& s) {
    // Gets rid of both &#039; and &apos;
    size_t pos = 0;
    while ((pos = s.find(APOS_ENTITY, pos)) != std::string::npos ||
           (pos = s.find(APOS_ENTITY2, pos)) != std::string::npos) {
        s.erase(pos, (pos == s.find(APOS_ENTITY, pos)) ? APOS_ENTITY.length()
                                                       : APOS_ENTITY2.length());
    }
    if (s.size() > MAX_WORD_LENGTH) {
        s.clear();
        return;
    }
    // Gets rid of strings containing HTML entities
    if (s.find(HTML_ENTITY) != std::string::npos ||
        s.find(UNWANTED_NBSP) != std::string::npos ||
        s.find(UNWANTED_LRM) != std::string::npos ||
        s.find(UNWANTED_RLM) != std::string::npos) {
        s.clear();
        return;
    }
    // Gets rid of non english words
    /*
      if (!isEnglish(s)) {
      s.clear();
      return;
    }
    */

    // Gets rid of '
    s.erase(std::remove(s.begin(), s.end(), '\''), s.end());

    // Get rid of start and end non punctuation
    auto start = std::find_if(s.begin(), s.end(), ::isalnum);

    if (start == s.end()) {
        s.clear();
        return;
    }

    auto end = std::find_if(s.rbegin(), s.rend(), ::isalnum).base();

    s.erase(end, s.end());
    s.erase(s.begin(), start);
}

std::vector<std::string> splitHyphenWords(const std::string& word) {
    std::vector<std::string> parts;
    if (word.find('-') != std::string::npos) {
        size_t start = 0, end = 0;
        while ((end = word.find('-', start)) != std::string::npos) {
            std::string token = word.substr(start, end - start);
            if (!token.empty()) parts.push_back(token);
            start = end + 1;
        }
        std::string token = word.substr(start);
        if (!token.empty()) parts.push_back(token);
    } else {
        parts.push_back(word);
    }
    return parts;
}

int partition(int left, int right, int pivot_index) {
    int pivot_rank = links_vector[pivot_index].second;

    // Move the pivot to the end
    std::swap(links_vector[pivot_index], links_vector[right]);

    // Move all less ranked elements to the left
    int store_index = left;
    for (int i = left; i < right; i++) {
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
    if (left >= right) {
        return;
    }

    std::uniform_int_distribution<> gen(left, right - 1);
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
    // std::cout << "Enter fill_queue()" << std::endl;
    uint32_t links_vector_size = links_vector.size();

    if (explore_queue.empty() && links_vector_size > MAX_VECTOR_SIZE) {
        // std::cout << "Here1" << std::endl;
        // Establish range for uniform random num gen
        // Range is from 0 to the last element in the vector
        std::uniform_int_distribution<> gen{0, int(links_vector_size - 1)};
        // std::cout << "Here2" << std::endl;

        // Generates N random elements and moves them to the end
        for (size_t t = links_vector_size - 1;
             t > links_vector_size - NUM_RANDOM; t--) {
            std::swap(links_vector[gen(mt)], links_vector[t]);
        }
        // std::cout << "Here3" << std::endl;

        // Sorts the last N elements of the vector
        quickselect(links_vector_size - NUM_RANDOM, links_vector_size - 1,
                    NUM_RANDOM - TOP_K_ELEMENTS);

        // std::cout << "Here4" << std::endl;

        // Takes last K from vector and adds its to queue
        for (size_t i = 0; i < TOP_K_ELEMENTS; ++i) {
            explore_queue.push(std::move(links_vector.back().first));
            links_vector.pop_back();
        }
    }
    // std::cout << "Exit fill_queue()" << std::endl;
}

std::string get_next_url() {
    sem_wait(queue_sem);
    pthread_lock_guard guard{queue_lock};

    size_t links_vector_size = links_vector.size();
    std::string url;

    if (!explore_queue.empty()) {
        // std::cout << "Pull url from explore queue" << std::endl;
        url = std::move(explore_queue.front());
        explore_queue.pop();
    } else if (links_vector_size > MAX_VECTOR_SIZE) {
        // std::cout << "Explore queue empty, fill queue with links from vector"
        // << std::endl;
        fill_queue();
        url = std::move(explore_queue.front());
        explore_queue.pop();
    } else {
        // std::cout << "Explore queue empty, use last link from vector"
        // << std::endl;
        url = std::move(links_vector.back().first);
        links_vector.pop_back();
    }

    return url;
}

std::string getHostFromUrl(const std::string& url) {
    // std::regex urlRe("^.*://([^/?:]+)/?.*$");
    // return std::regex_replace(url, urlRe, "$1");

    std::string::size_type pos = url.find("://");
    if (pos == std::string::npos) return "";
    pos += 3;
    std::string::size_type endPos = url.find('/', pos);
    return url.substr(pos, endPos - pos);
}

struct ThreadArgs {
    std::string url;
    std::string html;
    int status;
};

void* getHTML_wrapper(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;

    args->status = getHTML(args->url, args->html);

    return nullptr;
}

bool check_url(cstring_view url) {
    // If link does not begin with 'http', ignore it
    if (url.size() < 4 || !url.starts_with(cstring_view{"http", 4UZ})) {
        return false;
    }

    if (url.find(cstring_view{"porn"}) != cstring_view::npos) return false;

    if (url.size() > 250) {
        return false;
    }

    return true;
}

struct Args {
    HtmlParser parser;
    std::string url;
    int static_rank;
    unsigned int thread_id;
};

void* add_to_index(void* addr) {
    Args* arg = reinterpret_cast<Args*>(addr);

    pthread_lock_guard _{chunk_lock};
    if (arg->parser.isEnglish) {
        const uint16_t urlLength = arg->url.size();
        chunk.add_url(arg->url, arg->static_rank);

        for (auto& word : arg->parser.titleWords) {
            cleanString(word);
            if (word.empty()) continue;
            auto parts = splitHyphenWords(word);
            for (auto& part : parts) {
                std::transform(part.begin(), part.end(), part.begin(),
                               [](unsigned char c) { return std::tolower(c); });
                // std::cout << part << '\n';
                chunk.add_word(part, false);
            }
        }

        for (auto& word : arg->parser.words) {
            cleanString(word);
            if (word.empty()) continue;
            auto parts = splitHyphenWords(word);
            for (auto& part : parts) {
                std::transform(part.begin(), part.end(), part.begin(),
                               [](unsigned char c) { return std::tolower(c); });
                // std::cout << part << '\n';
                chunk.add_word(part, false);
            }
        }

        chunk.add_enddoc();
    }
    delete arg;
    return NULL;
}

void* runner(void*) {
    // const static thread_local pid_t thread_id = syscall(SYS_gettid);
    const static thread_local auto thread_id = rand() % NUM_CHUNKS;
    while (num_processed < MAX_PROCESSED) {
        // Get the next url to be processed
        std::string url = get_string();
        if (url.empty()) {
            sleep(10);
            continue;
        }

        // Print the url that is being processed
        // std::cout << "-----------------" << '\n';
        // std::cout << url << '\n';
        // std::cout << "-----------------" << '\n';

        // std::cout << "URL length: " << url.size() << std::endl;
        // std::cout << "Size of link vector: " << links_vector.size()
        // << std::endl;

        pthread_t thread;

        ThreadArgs args = {url, "", -1};

        // Start getHTML in a new thread
        pthread_create(&thread, nullptr, getHTML_wrapper, &args);
        pthread_join(thread, nullptr);

        // Continue if html code was not retrieved
        if (args.status != 0) {
            // std::cout << "Status " << args.status << std::endl;
            // std::cout << "Could not retrieve HTML\n" << std::endl;
            continue;
        }

        // Parse the html code
        std::string& html = args.html;
        HtmlParser parser(html.data(), html.size());
        int static_rank = get_static_rank(cstring_view{url}, parser);
        // Process links found by the parser
        {
            pthread_lock_guard guard{queue_lock};
            num_processed++;
            if (num_processed % 1000 == 0)
                std::cout << num_processed << std::endl;

            if (links_vector.size() < MAX_QUEUE_SIZE) {
                for (auto& link : parser.links) {
                    std::string next_url = std::move(link.URL);

                    // Ignore links that begin with '#' or '?'
                    if (next_url[0] == '#' || next_url[0] == '?') {
                        continue;
                    }

                    // If link starts with '/', add the domain to the beginning
                    // of it
                    if (next_url[0] == '/') {
                        next_url =
                            url.substr(0, 8) + getHostFromUrl(url) + next_url;
                    }

                    if (!check_url(next_url)) {
                        continue;
                    }

                    // If link has not been seen before, add it to the bf and
                    // links vector
                    add_url(next_url, static_rank);
                    /*if (!bf.contains(next_url)) {
                      bf.insert(next_url);
                      links_vector.emplace_back(next_url,
                                                static_rank);  //
                    STATIC_RANK++}); sem_post(queue_sem);
                    }*/
                }
            }
            // --------------------------------------------------
            // For debugging (not needed for crawler to function)
            /*std::string filename =
                "./files/file" + std::to_string(num_processed) + ".txt";
            std::ofstream output_file(filename);

            if (!output_file) {
              // std::cerr << "Error opening file!\n" << std::endl;
              // std::cerr << url << std::endl;
              continue;
            }

            output_file << url << "\n\n";
            /*output_file << "Number of links in queue: "
                        << explore_queue.size() + links_vector.size() << "\n\n";
            output_file << parser.words.size() << " words\n";
            output_file << parser.links.size() << " links\n\n";
            output_file << html;
            output_file << "\n\n";
            for (auto& word : parser.words) output_file << word << ' ';

            output_file.close();
             */
            // --------------------------------------------------
            pthread_t t;
            pthread_create(&t, NULL, add_to_index,
                           new Args{std::move(parser), std::move(url),
                                    static_rank, thread_id});
            pthread_detach(t);
            // std::cout << '\n';
        }
    }
    return NULL;
}

int main(int argc, char** argv) {
    signal(SIGPIPE, SIG_IGN);

    std::vector<std::string> sem_names{};
    for (int i = 0; i < NUM_CHUNKS; ++i) {
        sem_names.push_back("/sem_" + std::to_string(i));
        sem_unlink(sem_names[i].data());
    }

    /*for (const auto& url : seed_urls) {
      explore_queue.push(url);
      bf.insert(url);
    }*/
    sem_unlink("/crawler_sem");
    queue_sem = sem_open("/crawler_sem", O_CREAT, 0666, explore_queue.size());
    if (queue_sem == SEM_FAILED) exit(EXIT_FAILURE);

    pthread_mutex_init(&queue_lock, NULL);
    pthread_mutex_init(&chunk_lock, NULL);
    pthread_mutex_init(&cout_lock, NULL);

    pthread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(threads + i, NULL, runner, NULL);
    }
    std::cout << "STARTED THREADS...\n";
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    std::cout << "FINISHED THREADS/...\n";

    IndexFile chunkFile(0, chunk);

    pthread_mutex_destroy(&queue_lock);
    pthread_mutex_destroy(&chunk_lock);
    pthread_mutex_destroy(&cout_lock);
    // for (int i = 0; i < NUM_CHUNKS; ++i) IndexChunk::Write(chunks[i].chunk,
    // i);

    // std::cout << "Time taken: " << duration.count() << " ms" << std::endl;
    return 0;
}
