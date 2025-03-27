#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_set>

#include "../../BloomFilterStarterFiles/BloomFilter.h"

int main() {
  auto dir = std::filesystem::path("./files");
  Bloomfilter bf(100000, 0.0001);  // Temp size and false pos rate
  std::unordered_set<std::string> s{};
  for (auto &file : std::filesystem::directory_iterator(dir)) {
    const auto &p = file.path();
    std::fstream f{p};
    std::string st;
    std::getline(f, st);
    auto [it, b] = s.insert(st);
    auto b2 = bf.contains(st);
    bf.insert(st);
    if (!b) {
      std::cout << st << ' ' << file.path() << std::endl;
      assert(b2);
    }
  }
}