#include <netdb.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>

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

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " url" << std::endl;
    return 1;
  }

  // Parse the URL
  ParsedUrl url(argv[1]);

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
  int socketFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  // Connect the socket to the host address.
  // printAddrInfo(*address);
  int connectResult = connect(socketFD, address->ai_addr, address->ai_addrlen);
  //  std::cout << "working" << std::endl;

  // Send a GET message.
  std::string req =
      "GET /" + std::string(url.Path) +
      " HTTP/1.1\r\nHost: " + std::string(url.Host) +
      "\r\nUser-Agent: "
      "LinuxGetUrl/2.0 404FoundEngine@umich.edu (Linux)\r\nAccept: "
      "*/*\r\nAccept-Encoding: identity\r\nConnection: close\r\n\r\n";

  send(socketFD, req.c_str(), req.length(), 0);

  // Read from the socket until there's no more data, copying it to
  // stdout.
  char buffer[65536];  // 64 KB or 16 pages
  int bytes;

  bool skip_header = false;

  while ((bytes = recv(socketFD, buffer, sizeof(buffer), 0)) > 0) {
    if (!skip_header) {
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
