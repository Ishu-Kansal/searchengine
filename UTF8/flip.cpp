#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <vector>

#include "Utf8.h"

struct return_t
{
  Utf8 *memStart;
  long numBytes;
};

int GetFileSize(int f)
{
  struct stat fileInfo;
  fstat(f, &fileInfo);
  return fileInfo.st_size;
}

std::vector<Unicode> GetUnicodeMem(const Utf8 *fileStart, const Utf8 *fileEnd)
{
  std::vector<Unicode> chars;
  chars.reserve(fileEnd - fileStart);
  chars.push_back(ByteOrderMark);
  while (fileStart < fileEnd)
  {
    chars.push_back(GetUtf8(fileStart, fileEnd));
    fileStart = NextUtf8(fileStart, fileEnd);
  }
  return chars;
}

return_t GetUtf8Mem(const Unicode *fileStart, const Unicode *fileEnd)
{
  size_t worstCase = (fileEnd - fileStart + 1) * 3;
  Utf8 *mem = new Utf8[worstCase]();
  Utf8 *start = mem;
  start = WriteUtf8(start, ByteOrderMark);
  while (fileStart < fileEnd)
  {
    start = WriteUtf8(start, *fileStart);
    ++fileStart;
  }
  return {.memStart = mem, .numBytes = start - mem};
}

int main(int argc, char **argv)
{
  if (argc == 1)
  {
    std::cerr << "Usage: flip <filename>\n";
    std::cerr << "Convert Unicode to Utf8 and Utf8 or ASCII to Unicode and "
                 "write to stdout.\n";
    return 0;
  }
  const int f = open(argv[1], O_RDONLY);
  if (f == -1)
  {
    std::cerr << "Error opening file: " << argv[1] << " with error "
              << errno << std::endl;
    close(f);
    exit(EXIT_FAILURE);
  }
  size_t size = GetFileSize(f);
  Utf8 *UtfFile = (Utf8 *)(mmap(nullptr, size, PROT_READ, MAP_PRIVATE, f, 0));
  Utf8 *UtfEnd = UtfFile + size;
  Unicode *UnicodeFile = reinterpret_cast<Unicode *>(UtfFile);
  Unicode *UnicodeEnd = reinterpret_cast<Unicode *>(UtfEnd);
  if (GetUtf8(UtfFile, UtfEnd) == ByteOrderMark)
  {
    const std::vector<Unicode> unicode = GetUnicodeMem(UtfFile + 3, UtfEnd);
    write(STDOUT_FILENO, unicode.data(),
          unicode.size() * sizeof(unicode[0]));
  }
  else if (UnicodeFile[0] == ByteOrderMark)
  {
    const return_t data = GetUtf8Mem(UnicodeFile + 1, UnicodeEnd);
    write(STDOUT_FILENO, data.memStart, data.numBytes);
    delete[] data.memStart;
  }
  else
  {
    const std::vector<Unicode> unicode = GetUnicodeMem(UtfFile, UtfEnd);
    write(STDOUT_FILENO, unicode.data(),
          unicode.size() * sizeof(unicode[0]));
  }
  close(f);
}