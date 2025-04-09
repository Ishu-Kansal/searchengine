#include <fstream>
#include "../../utils/utf_encoding.h"
#include "Index.h"
#include "IndexFile.h"

int main() {
    IndexChunk indexChunk;
    std::string url = "http://example.com";
    size_t rank = 5;
    indexChunk.add_url(url, rank);
    
    std::string word1 = "test";
    indexChunk.add_word(word1, false);
    
    std::string word2 = "creeper";
    indexChunk.add_word(word2, true);
    
    indexChunk.add_enddoc();
    
    assert(!indexChunk.get_posting_lists().empty());
    
    uint32_t chunkNum = 1;

    IndexFile indexFile(chunkNum, indexChunk);
    const char * hashFilePath = "HashFile_00001";
    HashFile hashFile(hashFilePath);
    const HashBlob *hashblob = hashFile.Blob();
    const char *filename = "IndexChunk_00001";
    int fileDescrip = open(filename, O_RDWR);
    const size_t bufferSize = 4096;
    std::vector<char> buffer;       
    std::vector<char> varintBytes;    
    bool varintDecoded = false;
    uint64_t decodedVal = 0;
    char tempBuffer[bufferSize];
    ssize_t bytesRead;

    auto offset = hashblob->Find("test");
    off_t resultPostition = lseek(fileDescrip, offset->Value, SEEK_SET);
    if ( resultPostition == -1)
    {
        cerr << "Could not seek to " << offset->Value << '\n';
    }
    while ((bytesRead = read(fileDescrip, tempBuffer, bufferSize)) > 0) {

   }
   if (bytesRead < 0) {
       std::cerr << "Error reading file: " << strerror(errno) << "\n";
       close(fileDescrip);
       return 1;
   }
   close(fileDescrip);

   std::cout << "Decoded varint value: " << decodedVal << "\n";
   std::cout.write(buffer.data(), buffer.size());
   std::cout << "\n";
}
