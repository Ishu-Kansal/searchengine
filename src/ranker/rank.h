#pragma once
#include <stdint.h>
#include <string>
#include <unordered_map>
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

// Number of images in website multiplier, I don't know how to make this branchless
float get_numImages_weight(int occurences) {
  // if occurences less than 5 check 0, return 1 if 0, otherwise return 1.2
  if (occurences <= 5) return (occurences == 0) ? 1.0f : 1.2f;
  // if 6-15 images return 1.25
  else if (occurences <= 15) return 1.25f;
  // if 16-25 return 1.2
  else if (occurences <= 25) return 1.2f; 
  // if more than 25 then slightly punish with a 0.75 multiplier
  return 0.75f; 
}

// optimized for branch prediction
float get_numLinks_weight(int occurences) {
  if (occurences <= 250) return (occurences == 0) ? 1.0f : 1.1f; 
  // if more than 250 outgoing links probably a spam site so punish with a 0.8(might be a bibliography so dont punish too much)
  else return 0.8f; 
}

// branchless
float get_domain_weight(std::string domain) {
  // make a static hashmap for branchless and give it a weight 
  static const std::unordered_map<std::string, float> domainWeights = {
    {".gov", 1.5f}, {".edu", 1.5f}, 
    {".org", 1.2f}, {".ai", 1.2f},  
    {".com", 1.0f}, {".net", 1.0f}, 
    {".info", 0.75f}, {".biz", 0.75f}, 
  };
  auto it = domainWeights.find(domain);
  // if iterator not found return 0.5 otherwise return its respective weight
  return (it != domainWeights.end()) ? it->second : 0.5f;
}

double get_document_length_weight(const decltype(HtmlParser::words) &words)
{
  const size_t n = words.size();
  return articleWeights[get_document_length(n)];
}

// branchless
float get_url_length_weight(int length) {
  static const float weightsarr[] = {0.8f, 1.0f, 1.1f, 1.2f};
  // reward shorter urls
  return weightsarr[
    (length >= 100) ? 0:
    (length >= 75) ? 1:
    (length >= 50) ? 2:
    3
  ];
}

// TBD
double get_static_rank(cstring_view url, const HtmlParser &parser)
{
  return urlLengthWeight * url.size();
}