#include <iostream>
#include <string>

#include "../../HtmlParser/HtmlParser.h"
#include "../crawler/sockets.h"
#include "rank.h"

int main(int argc, char **argv) {
  std::string url{argv[1]}, content{};
  getHTML(url, content);
  HtmlParser parser(content.data(), content.size());

  std::cout << "=====================" << std::endl;
  std::cout << "Url: " << url << std::endl;
  std::cout << "Static rank: " << get_static_rank(url, parser) << std::endl;
  std::cout << "=====================" << std::endl;
}