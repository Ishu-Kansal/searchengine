#pragma once

#include <fcntl.h>
#include <netdb.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <poll.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <iostream>
#include <memory>

#include "../../utils/socket_wrapper.h"
#include "../../utils/ssl_cleaner.h"
// #include "robotParser.h"
class ParsedUrl {
 public:
  const char *CompleteUrl{};
  char *Service{}, *Host{}, *Port{}, *Path{};

  ParsedUrl(const char *url) {
    // Assumes url points to static text but
    // does not check.

    CompleteUrl = url;

    pathBuffer = new char[strlen(url) + 1]{};
    const char *f{};
    char *t{};
    for (t = pathBuffer, f = url; *t++ = *f++;);

    Service = pathBuffer;

    const char Colon = ':', Slash = '/';
    char *p{};
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

int connect_socket(addrinfo *address) {
  pollfd p{};

  int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock == -1) return -1;

  int res = fcntl(sock, F_SETFL, O_NONBLOCK);
  if (res == -1) return -1;

  connect(sock, address->ai_addr, address->ai_addrlen);

  p.fd = sock;
  p.events = POLLIN | POLLOUT;

  int ready = poll(&p, 1, 2500);

  if (ready == 0) {
    close(sock);
    return -1;
  }
  if (ready == -1) {
    perror("");
  }

  int oldfl = fcntl(sock, F_GETFL);

  fcntl(sock, F_SETFL, oldfl & ~O_NONBLOCK);
  return sock;
}

int runSocket(std::string req, std::string url_in, std::string &output) {
  ParsedUrl url(url_in.data());
  if (!url.Host || !url.Port || !url.CompleteUrl) return 1;
  struct addrinfo hints, *address;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  if (getaddrinfo(url.Host, (*url.Port ? url.Port : "443"), &hints, &address) !=
      0) {
    // // << "Failed to resolve host" << std::endl;
    return 2;
  }
  // Create a socket

  int socketFD = connect_socket(address);

  freeaddrinfo(address);

  if (socketFD < 0) {
    // << "Failed to create socket" << std::endl;
    return 3;
  }

  SocketWrapper _wrapper{socketFD};

  SSL_CTX *ctx = SSL_CTX_new(SSLv23_method());
  if (!ctx) {
    return 5;
  }

  SSL *ssl = SSL_new(ctx);
  if (ssl == nullptr) {
    SSL_CTX_free(ctx);
    return 6;
  }

  SSL_Cleaner cleaner_{ctx, ssl};

  struct timeval tv;
  tv.tv_sec = 5;  // 5 seconds
  tv.tv_usec = 0;
  int ret = setsockopt(socketFD, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv,
                       sizeof tv);

  if (ret != 0) {
    return 7;
  }

  SSL_set_fd(ssl, socketFD);

  if (SSL_connect(ssl) <= 0) {
    return 9;
  }

  if (SSL_write(ssl, req.c_str(), req.length()) <= 0) {
    return 10;
  }

  // Read and process the response
  // char buffer[32768]{};
  const static size_t BUFFER_SIZE = 16384;
  char *buffer = new char[BUFFER_SIZE];
  std::unique_ptr<char[]> buf(buffer);

  int bytes = 0;
  bool skip_header = false;
  std::string leftover = "";

  std::string header = "";
  int code = 0;

  while ((bytes = SSL_read(ssl, buffer, BUFFER_SIZE)) > 0) {
    if (!skip_header) {
      // Combine leftover from previous reads
      std::string response = leftover + std::string(buffer, bytes);
      size_t header_end = response.find("\r\n\r\n");

      header = response;
      // code = stoi(header.substr(9, 3));
      if (header.size() < 9) {
        return 11;
      }
      code = atoi(header.data() + 9);
      if (code == 0) {
        return 12;
      }
      if (code == 301) break;

      if (header_end != std::string::npos) {
        skip_header = true;
        header_end += 4;  // Move past the "\r\n\r\n"
        // write(1, response.c_str() + header_end, response.length() -
        // header_end);
        assert(header_end <= response.length());
        output.append(response.data() + header_end,
                      response.length() - header_end);
      } else {
        leftover = std::move(response);
        // Save the partial response for the next read
      }
    } else {
      // std::cout << "outputting\n";
      // write(1, buffer, bytes);
      output.append(buffer, bytes);  // avoid temp string construction
    }
  }

  if (code == 301) {
    int locPos = header.find("location");
    int endPos = header.find("\r\n", locPos);
    if (locPos + 10 >= header.size()) return 404;
    std::string newURL = header.substr(locPos + 10, endPos - locPos - 10);
    // std::cout << "redirecting to " << newURL << std::endl;
    output = newURL;
    return code;
  }

  if (code == 404) {
    return 404;
  }
  return 0;
}

int getHTML(std::string url_in, std::string &output) {
  ParsedUrl url(url_in.data());
  // static RobotParser bot;
  //  int robotStatus = bot.handleRobotFile(url, url_in);
  int robotStatus = 0;
  if (robotStatus == 0) {
    // Send the GET request
    std::string req =
        "GET /" + std::string(url.Path) +
        " HTTP/1.1\r\n"
        "Host: " +
        std::string(url.Host) +
        "\r\n"
        "User-Agent: SonOfAnton/1.0 404FoundEngine@umich.edu (Linux)\r\n"
        "Accept: */*\r\n"
        "Accept-Encoding: identity\r\n"
        "Connection: close\r\n\r\n";

    int status = runSocket(req, url_in, output);

    // std::cout << "output:\n" << output << std::endl;

    if (status == 301) {
      ParsedUrl newURL(output.data());
      std::string req2 = "GET /" + std::string(newURL.Path) +
                         " HTTP/1.1\r\n"
                         "Host: " +
                         std::string(newURL.Host) +
                         "\r\n"
                         "User-Agent: SonOfAnton/1.0 "
                         "404FoundEngine@umich.edu (Linux)\r\n"
                         "Accept: */*\r\n"
                         "Accept-Encoding: identity\r\n"
                         "Connection: close\r\n\r\n";
      std::string newishURL = output;
      output = "";
      status = runSocket(req2, newishURL, output);

      // std::cout << "redirected output:\n" << output << std::endl;
    }
    return status;
  }
  return 0;
}