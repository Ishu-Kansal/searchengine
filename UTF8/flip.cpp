#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <vector>

#include "Utf8.h"

enum Mode
{
  ASCII,
  UTF8,
  UNICODE,
};

int getFileSize(int f)
{
  struct stat fileInfo;
  fstat(f, &fileInfo);
  return fileInfo.st_size;
}

int main(int argc, char **argv)
{
  const int f = open(argv[1], O_RDONLY);
  if (f == -1)
  {
    std::cerr << "Error opening file: " << argv[1] << " with error " << errno
              << std::endl;
    return;
  }
  size_t size = getFileSize(f);
  char *file = (char *)(mmap(nullptr, size, PROT_READ, MAP_PRIVATE, f, 0));
  char *end = file + size;
  if (GetUtf8(reinterpret_cast<Utf8 *>(file)) == ByteOrderMark)
  {
  }
  std::vector<Utf8> chars{};
  chars.reserve(size);
  for (size_t i = 0; i < size; ++i)
  {
    const char byte = file[i];
    const Utf8 symbol = static_cast<Utf8>(byte);
  }

  write(STDOUT_FILENO, chars.data(), chars.size());
}