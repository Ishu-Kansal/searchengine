#pragma once

#include <../utils/cstring_view.h>
#include <../HtmlParser/HtmlParser.h>

template <class T>
using vector = std::vector<T>;

enum DocumentLengthRank : uint8_t
{
  VERYSHORT = 0,
  SHORT = 1,
  MEDIUM = 2,
  LONG = 3,
  VERYLONG = 4
};

namespace
{
  const static double urlLengthWeight = 1.0;

  const static double articleLengthWeight = 2.5;
  const static double articleWeights[] = {-1.0, 0.8, 1.0, 0.5, -1.0};
  /*const static double veryShortArticle = -1.0;
  const static double shortArticle = 0.8;
  const static double mediumArticle = 1.0;
  const static double longArticle = 0.5;
  const static double veryLongArticle = -1.0;*/
}

namespace
{
  DocumentLengthRank get_document_length(size_t n)
  {
    if (n < 64)
      return VERYSHORT;
    else if (n < 256)
      return SHORT;
    else if (n < 1024)
      return MEDIUM;
    else if (n < 2048)
      return LONG;
    else
      return VERYLONG;
  }
}

double get_document_length_weight(const decltype(HtmlParser::words) &words)
{
  const size_t n = words.size();
  return articleWeights[get_document_length(n)];
}

double get_static_rank(cstring_view url, const HtmlParser &parser)
{
  return urlLengthWeight * url.size();
}