#include <cassert>
#include <cstdint>
#include <iostream>
#include "Index.h"
int main() {
    uint8_t* buffer = new uint8_t[16];
    uint64_t original;
    uint64_t decoded;
    size_t len_encoded, len_decoded;

    uint64_t testValues[] = { 
        0, 1, 127, 128, 255, 300, 16384, 0xffffffff, 0xffffffffffffffffULL 
    };
    
    for (auto value : testValues) {
        original = value;
        size_t num_bytes = SizeOfDelta(value);
        encodeVarint(original, buffer, num_bytes);
        decodeVarint(buffer, decoded);

        std::cout << "Value: " << original 
                << ", Num Bytes: " << num_bytes 
                << ", Decoded: " << decoded << "\n";

        assert(original == decoded);
    }

    delete[] buffer;
}
