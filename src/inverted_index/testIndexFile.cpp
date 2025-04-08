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
    
    assert(!indexChunk.get_urls().empty());
    assert(!indexChunk.get_posting_lists().empty());
    
    uint32_t chunkNum = 1;
    IndexFile indexFile(chunkNum, indexChunk);
    const char * hashFilePath = "HashFile_00001";
    HashFile hashFile(hashFilePath);
    const HashBlob *hashblob = hashFile.Blob();
    /*
        auto offset = hashblob->Find("test");
    
    
    std::cout << "All tests passed.\n";
    return 0;
    */
   const char *filename = "IndexChunk_00001";
   int fileDescrip = open(filename, O_RDWR);
   const size_t bufferSize = 4096;
   std::vector<char> buffer;
   char tempBuffer[bufferSize];
   ssize_t bytesRead;
   while ((bytesRead = read(fileDescrip, tempBuffer, bufferSize)) > 0) {
       buffer.insert(buffer.end(), tempBuffer, tempBuffer + bytesRead);
   }
   if (bytesRead < 0) {
       std::cerr << "Error reading file: " << strerror(errno) << "\n";
       close(fileDescrip);
       return 1;
   }
   
   std::cout << "Data read (" << buffer.size() << " bytes):\n";
   std::cout.write(buffer.data(), buffer.size());
   std::cout << "\n";

   close(fileDescrip);

}