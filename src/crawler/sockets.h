#pragma once

#include <netdb.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <iostream>
#include <memory>

#include "../../utils/cunique_ptr.h"
//#include "robotParser.h"
class ParsedUrl {
 public:
  const char *CompleteUrl;
  char *Service, *Host, *Port, *Path;

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
    std::cerr << "Failed to resolve host" << std::endl;
    return 1;
  }

  // Create a socket
  int socketFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (socketFD < 0) {
    std::cerr << "Failed to create socket" << std::endl;
    freeaddrinfo(address);
    return 1;
  }

  // Connect the socket
  if (connect(socketFD, address->ai_addr, address->ai_addrlen) < 0) {
    std::cerr << "Failed to connect to host" << std::endl;
    close(socketFD);
    freeaddrinfo(address);
    return 1;
  }
  freeaddrinfo(address);  // Done with the address info
  // Initialize SSL
  SSL_library_init();
  SSL_CTX *ctx = SSL_CTX_new(SSLv23_method());
  if (!ctx) {
    std::cerr << "Failed to create SSL context" << std::endl;
    close(socketFD);
    return 1;
  }

  SSL *ssl = SSL_new(ctx);
  if (ssl == nullptr) {
    std::cerr << "could not create ssl object" << std::endl;
    SSL_CTX_free(ctx);
    close(socketFD);
    return 1;
  }

  struct timeval tv;
  tv.tv_sec = 5;  // 5 seconds
  tv.tv_usec = 0;
  int ret = setsockopt(socketFD, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv,
                       sizeof tv);

  if (ret != 0) {
    std::cerr << "Failed to set socket timeout" << std::endl;
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    close(socketFD);
    return 1;
  }

  int val = 1;
  ret = setsockopt(socketFD, SOL_SOCKET, SO_NOSIGPIPE, &val, sizeof(val));

  if (ret != 0) {
    std::cerr << "Failed to set socket timeout" << std::endl;
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    close(socketFD);
    return 1;
  }

  SSL_set_fd(ssl, socketFD);
  SSL_set_tlsext_host_name(ssl, url.Host); // needed for umich and espn to work

  if (SSL_connect(ssl) <= 0) {
    std::cerr << "Failed to establish SSL connection" << std::endl;
    // ERR_print_errors_fp(stderr);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    close(socketFD);
    return 1;
  }

  if (SSL_write(ssl, req.c_str(), req.length()) <= 0) {
    std::cerr << "Failed to send request" << std::endl;
    // SSL_shutdown(ssl);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    close(socketFD);
    return 1;
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
        SSL_shutdown(ssl);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        close(socketFD);
        return -1;
      }
      code = atoi(header.data() + 9);
      if (code == 0) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        close(socketFD);
        return -1;
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
    // Convert to lowercase to find "location:" in a case-insensitive way
    std::string header_lower = header;
    std::transform(header_lower.begin(), header_lower.end(), header_lower.begin(), ::tolower);

    size_t locPos = header_lower.find("location:");
    if (locPos != std::string::npos) {
        // Use the original header to extract the actual Location value (case preserved)
        size_t endPos = header.find("\r\n", locPos);
        std::string newURL = header.substr(locPos + 9, endPos - locPos - 9);

        // Trim whitespace
        newURL.erase(0, newURL.find_first_not_of(" \t\r\n"));
        newURL.erase(newURL.find_last_not_of(" \t\r\n") + 1);

        output = newURL;
        return 301;
    } else {
        std::cerr << "Redirect code 301, but no Location header found." << std::endl;
        return -1;
    }
  }

  if (bytes < 0) {
    std::cerr << "Error reading from SSL socket" << std::endl;
  }

  // Clean up
  SSL_shutdown(ssl);
  SSL_free(ssl);
  SSL_CTX_free(ctx);
  close(socketFD);
  if (code == 404)
  {
    return 404;
  }
  return 0;
}

int getHTML(std::string url_in, std::string &output) {
  ParsedUrl url(url_in.data());

    // Send the GET request
    std::string path = (url.Path && *url.Path) ? std::string(url.Path) : "/";
    std::string req =
      "GET " + path + " HTTP/1.1\r\n"
      "Host: " + std::string(url.Host) + "\r\n"
      "User-Agent: SonOfAnton/1.0 404FoundEngine@umich.edu (Linux)\r\n"
      "Accept: */*\r\n"
      "Accept-Encoding: identity\r\n"
      "Connection: close\r\n\r\n";

    int status = runSocket(req, url_in, output);

    if (status == 301) {
      ParsedUrl newURL(output.data());
      std::string path2 = (newURL.Path && *newURL.Path) ? std::string(newURL.Path) : "/";
      std::string req2 =
        "GET " + path2 + " HTTP/1.1\r\n"
        "Host: " + std::string(newURL.Host) + "\r\n"
        "User-Agent: SonOfAnton/1.0 404FoundEngine@umich.edu (Linux)\r\n"
        "Accept: */*\r\n"
        "Accept-Encoding: identity\r\n"
        "Connection: close\r\n\r\n";
        
      std::string newishURL = output;
      output = "";
      status = runSocket(req2, newishURL, output);
    }
    return status;

}