#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
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

#include "../../HtmlParser/HtmlParser.h"
#include "../../utils/pthread_lock_guard.h"

constexpr uint32_t MAX_PROCESSED = 5;
constexpr uint32_t TOP_K_ELEMENTS = 5000;
constexpr uint32_t NUM_RANDOM = 10000;

uint32_t STATIC_RANK = 0;  // temp global variable

std::queue<std::string> explore_queue{};
std::vector<std::pair<std::string, uint32_t>> links_vector{};
sem_t *queue_sem{};

pthread_mutex_t queue_lock{};
pthread_mutex_t output_lock{};
uint32_t num_processed{};
std::mt19937 mt{std::random_device{}()};

class ParsedUrl {
 public:
  const char *CompleteUrl;
  char *Service, *Host, *Port, *Path;

  void printURL() {
    std::cout << "url: " << CompleteUrl << std::endl;
    std::cout << "service: " << Service << std::endl;
    std::cout << "host: " << Host << std::endl;
    std::cout << "port: " << Port << std::endl;
    std::cout << "path: " << Path << std::endl;
  }

  ParsedUrl(const char *url) {
    // Assumes url points to static text but
    // does not check.

    CompleteUrl = url;

    pathBuffer = new char[strlen(url) + 1];
    const char *f;
    char *t;
    for (t = pathBuffer, f = url; *t++ = *f++;);

    Service = pathBuffer;

    const char Colon = ':', Slash = '/';
    char *p;
    for (p = pathBuffer; *p && *p != Colon; p++);

    if (*p) {
      // Mark the end of the Service.
      *p++ = 0;

      if (*p == Slash) p++;
      if (*p == Slash) p++;

      Host = p;

      for (; *p && *p != Slash && *p != Colon; p++);

      if (*p == Colon) {
        // Port specified.  Skip over the colon and
        // the port number.
        *p++ = 0;
        Port = +p;
        for (; *p && *p != Slash; p++);
      } else
        Port = p;

      if (*p)
        // Mark the end of the Host and Port.
        *p++ = 0;

      // Whatever remains is the Path.
      Path = p;
    } else
      Host = Path = p;
  }

  ~ParsedUrl() { delete[] pathBuffer; }

 private:
  char *pathBuffer;
};

void printAddrInfo(addrinfo &info) {
  std::cout << "ai addr: " << info.ai_addr << std::endl;
  std::cout << "ai addrlen: " << info.ai_addrlen << std::endl;
}

int write_to_socket(const char *input_url, int fd) {
  // Parse the URL
  ParsedUrl url(input_url);

  // Get the host address.
  struct addrinfo *address, hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  //  std::cout << url.Host << std::endl;
  getaddrinfo(url.Host, (*url.Port ? url.Port : "80"), &hints, &address);

  //  url.printURL();

  // Create a TCP/IP socket.
  int socketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (socketFD == -1) {
    perror("Failed to open socket");
    exit(EXIT_FAILURE);
  }

  // Connect the socket to the host address.
  // printAddrInfo(*address);
  int connectResult = connect(socketFD, address->ai_addr, address->ai_addrlen);

  struct timeval timeout;
  timeout.tv_sec = 30;
  timeout.tv_usec = 0;
  setsockopt(socketFD, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
  //  std::cout << "working" << std::endl;

  // Send a GET message.
  std::string req =
      "GET /" + std::string(url.Path) +
      " HTTP/1.1\r\nHost: " + std::string(url.Host) +
      "\r\nUser-Agent: "
      "LinuxGetUrl/2.0 404FoundEngine@umich.edu (Linux)\r\nAccept: "
      "*/*\r\nAccept-Encoding: identity\r\nConnection: close\r\n\r\n";
  std::cout << req << std::endl;

  ssize_t sent = send(socketFD, req.c_str(), req.length(), 0);
  if (sent == -1) return -1;

  // Read from the socket until there's no more data, copying it to
  // stdout.
  char buffer[65536];  // 64 KB or 16 pages
  int bytes;

  bool skip_header = false;

  while ((bytes = recv(socketFD, buffer, sizeof(buffer), 0)) != 0) {
    std::cout << "HERE\n" << bytes << std::endl;
    if (bytes == -1) {
      perror("");
      return -1;
    }
    if (!skip_header) {
      write(fd, buffer, bytes);
      // Find the first <
      // right now, doing first newline bc we need that for assignment
      // for crawler modify and change to find first instance of "<"
      char *afterBracket = strstr(buffer, "\r\n\r\n");
      if (afterBracket != nullptr) {
        afterBracket += 4;
        write(STDOUT_FILENO, afterBracket, bytes - (afterBracket - buffer));
      }
      skip_header = true;
    } else {
      write(STDOUT_FILENO, buffer, bytes);
    }
  }

  // Close the socket and free the address info structure.
  close(socketFD);
  freeaddrinfo(address);
}

int get_and_parse_url(const char *url, int fd) {
  write_to_socket(url, fd);
  return 0;
}

int get_file_size(int fd) {
  struct stat buf;
  fstat(fd, &buf);
  return buf.st_size;
}

std::string get_next_url() {
  pthread_mutex_lock(&queue_lock);

  std::string url;

  if (!explore_queue.empty()) {
    url = std::move(explore_queue.front());
    explore_queue.pop();
  }

  pthread_mutex_unlock(&queue_lock);

  return url;
}

void runner() {
  std::string url = get_next_url();

  const std::string fileName = "./files/temp.txt";

  int outputFd = open(fileName.data(), O_CREAT | O_TRUNC | O_RDWR, 0777);
  if (outputFd == -1) {
    perror("");
    std::clog << "URL already processed for url: " << url << '\n';
    return;
  }

  int status = get_and_parse_url(url.data(), outputFd);
  if (status != 0) {
    close(outputFd);
    return;
  }
  const int len = get_file_size(outputFd);
  if (len == 0) exit(EXIT_FAILURE);

  const char *fileData =
      (char *)mmap(nullptr, len, O_RDWR, PROT_READ | PROT_WRITE, outputFd, 0);
  if (fileData == MAP_FAILED) {
    perror("Failed to process url: ");
    close(outputFd);
    return;
  }
  try {
    HtmlParser parser(fileData, len);
    ++num_processed;
    {
      pthread_lock_guard guard{queue_lock};
      for (const auto &word : parser.titleWords) std::cout << word << ' ';
    }
  } catch (...) {
  }
  munmap((void *)fileData, len);
  close(outputFd);
  return;
}

int main(int argc, char **argv) {
  static const char *sem_name = "/crawler_semaphore";
  mode_t m;
  mkdir("files", 0777);
  sem_unlink(sem_name);

  pthread_mutex_init(&queue_lock, NULL);
  queue_sem = sem_open(sem_name, O_CREAT, 0777, 0);
  if (queue_sem == SEM_FAILED) {
    perror("Failed to open semaphore:");
    exit(EXIT_FAILURE);
  }

  explore_queue.push("https://www.wikipedia.org/");
  sem_post(queue_sem);

  runner();

  sem_close(queue_sem);
  pthread_mutex_destroy(&queue_lock);
}
