#include <cassert>
#include <cstdlib>

#include "utf_encoding.h"

int main() {
  uint8_t buf[10];
  uint64_t k, m;
  for (int i = 0; i < 100000; ++i) {
    k = rand();
    encodeVarint(k, buf);
    decodeVarint(buf, m);
    assert(k == m);
  }
}