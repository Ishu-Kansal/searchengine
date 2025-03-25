#include <netdb.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/tls1.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>

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

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " url" << std::endl;
        return 1;
    }

    // Parse the URL (assuming ParsedUrl is a valid class handling this)
    ParsedUrl url(argv[1]);

    // Resolve the host address
    struct addrinfo hints, *address;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(url.Host, (*url.Port ? url.Port : "443"), &hints,
                    &address) != 0) {
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
    }
    SSL_set_tlsext_host_name(ssl, url.Host);

    SSL_set_fd(ssl, socketFD);

    if (SSL_connect(ssl) <= 0) {
        std::cerr << "Failed to establish SSL connection" << std::endl;
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        close(socketFD);
        return 1;
    }

    // Send the GET request
    std::string req =
        "GET /" + std::string(url.Path) +
        " HTTP/1.1\r\n"
        "Host: " +
        std::string(url.Host) +
        "\r\n"
        "User-Agent: LinuxGetUrl/2.0 404FoundEngine@umich.edu (Linux)\r\n"
        "Accept: */*\r\n"
        "Accept-Encoding: identity\r\n"
        "Connection: close\r\n\r\n";

    if (SSL_write(ssl, req.c_str(), req.length()) <= 0) {
        std::cerr << "Failed to send request" << std::endl;
        SSL_shutdown(ssl);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        close(socketFD);
        return 1;
    }

    // Read and process the response
    char buffer[10240];
    int bytes;
    bool skip_header = false;
    std::string leftover;

    while ((bytes = SSL_read(ssl, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes] = '\0';  // Null-terminate for string operations

        if (!skip_header) {
            // Combine leftover from previous reads
            std::string response = leftover + std::string(buffer);
            size_t header_end = response.find("\r\n\r\n");
            if (header_end != std::string::npos) {
                skip_header = true;
                header_end += 4;  // Move past the "\r\n\r\n"
                write(1, response.c_str() + header_end,
                      response.length() - header_end);
            } else {
                leftover =
                    response;  // Save the partial response for the next read
            }
        } else {
            write(1, buffer, bytes);
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

    return 0;
}