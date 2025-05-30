#include <netdb.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/tls1.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
// static constexpr std::string_view HTTP_HEADER = 
//     "\r\n"
//     "User-Agent: SonOfAnton/1.0 404FoundEngine@umich.edu (Linux)\r\n"
//     "Accept: */*\r\n"
//     "Accept-Encoding: identity\r\n"
//     "Connection: close\r\n\r\n";

static constexpr std::string_view HTTP_HEADER = 
"\r\n"
"User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/18.0 Safari/605.1.15\r\n"
"Accept: */*\r\n"
"Accept-Encoding: identity\r\n"
"Referer: https://www.google.com/\r\n"
"Connection: close\r\n\r\n";

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
    struct addrinfo hints, *address;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(url.Host, (*url.Port ? url.Port : "443"), &hints,
                    &address) != 0) {
        // std::cout << "Failed to resolve host" << std::endl;
        return 1;
    }

    // Create a socket
    int socketFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socketFD < 0) {
        // std::cout << "Failed to create socket" << std::endl;
        freeaddrinfo(address);
        return 1;
    }

    // Connect the socket
    if (connect(socketFD, address->ai_addr, address->ai_addrlen) < 0) {
        // std::cout << "Failed to connect to host" << std::endl;
        close(socketFD);
        freeaddrinfo(address);
        return 1;
    }
    freeaddrinfo(address);  // Done with the address info

    // Initialize SSL
    SSL_library_init();
    SSL_CTX *ctx = SSL_CTX_new(SSLv23_method());
    // SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) {
        // std::cout << "Failed to create SSL context" << std::endl;
        close(socketFD);
        return 1;
    }

    SSL *ssl = SSL_new(ctx);
    if (ssl == nullptr) {
        // std::cout << "could not create ssl object" << std::endl;
    }
    SSL_set_tlsext_host_name(ssl, url.Host);

    SSL_set_fd(ssl, socketFD);

    if (SSL_connect(ssl) <= 0) {
        // std::cout << "Failed to establish SSL connection" << std::endl;
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        close(socketFD);
        return 1;
    }

    if (SSL_write(ssl, req.c_str(), req.length()) <= 0) {
        // std::cout << "Failed to send request" << std::endl;
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

    std::string header;
    int code;

    while ((bytes = SSL_read(ssl, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes] = '\0';  // Null-terminate for string operations

        if (!skip_header) {
            // Combine leftover from previous reads
            std::string response = leftover + std::string(buffer);
            size_t header_end = response.find("\r\n\r\n");

            header = response;
            // // std::cout << "print header " << header << std::endl;
            if (header.substr(0, 4) != "HTTP") {
                // std::cout << "header invalid\n";
                return 1;
            }
            code = stoi(header.substr(9, 3));
            if (code == 301) break;

            if (header_end != std::string::npos) {
                skip_header = true;
                header_end += 4;  // Move past the "\r\n\r\n"
                // write(1, response.c_str() + header_end, response.length() -
                // header_end);
                output += std::string(response.c_str() + header_end,
                                      response.length() - header_end);
            } else {
                leftover =
                    response;  // Save the partial response for the next read
            }
        } else {
            // // std::cout << "outputting\n";
            // write(1, buffer, bytes);
            output += std::string(buffer, buffer + bytes);
        }
    }

    if (code == 301) {
        int locPos = header.find("location");
        int endPos = header.find("\r\n", locPos);
        std::string newURL = header.substr(locPos + 10, endPos - locPos - 10);
        // // std::cout << "redirecting to " << newURL << std::endl;
        output = newURL;
        return code;
    }

    if (bytes < 0) {
        // std::cout << "Error reading from SSL socket" << std::endl;
        return 1;
    }

    // Clean up
    SSL_shutdown(ssl);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    close(socketFD);

    return 0;
}

int getHTML(std::string url_in, std::string &output) {
    ParsedUrl url(url_in.data());

    // Send the GET request
    std::string req =
        "GET /" + std::string(url.Path) +
        " HTTP/1.1\r\n"
        "Host: " +
        std::string(url.Host);
        req.append(HTTP_HEADER);

    int status = runSocket(req, url_in, output);
    // // std::cout << status << std::endl;

    // // std::cout << "output:\n" << output << std::endl;

    if (status == 301) {
        ParsedUrl newURL(output.data());
        std::string req2 =
            "GET /" + std::string(newURL.Path) +
            " HTTP/1.1\r\n"
            "Host: " +
            std::string(newURL.Host);
            req2.append(HTTP_HEADER);
        std::string newishURL = output;
        output = "";
        status = runSocket(req2, newishURL, output);

        // // std::cout << "redirected output:\n" << output << std::endl;
    }

    return status;
}