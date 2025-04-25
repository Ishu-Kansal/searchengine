#pragma once

#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

struct SSL_Cleaner {
  SSL_Cleaner(SSL_CTX *ctx, SSL *ssl) {
    this->ctx = ctx;
    this->ssl = ssl;
  }

  ~SSL_Cleaner() {
    if (ssl) SSL_shutdown(ssl);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
  }

 private:
  SSL_CTX *ctx{};
  SSL *ssl{};
};