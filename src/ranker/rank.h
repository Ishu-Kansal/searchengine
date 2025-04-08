#pragma once
#include <stdint.h>

#include <climits>
#include <string>
#include <unordered_map>

#include "../../HtmlParser/HtmlParser.h"
#include "../../utils/cstring_view.h"

template <class T>
using vector = std::vector<T>;

// hardcode everything in enums so that we can switch numbers out easily for
// testing the static ranker's effectiveness
enum DocumentLengthRank : uint8_t {
  VERYSHORT = 0,
  SHORT = 1,
  MEDIUM = 2,
  LONG = 3,
  VERYLONG = 4
};

enum DocumentLengths : int {
  VERYSHORTLENGTH = 64,
  SHORTLENGTH = 256,
  MEDIUMLENGTH = 1024,
  LONGLENGTH = 2048
};

enum ImagesWeights : int {
  NO_IMG_WEIGHT = 0,
  FEW_IMG_WEIGHT = 2,
  MODERATE_IMG_WEIGHT = 3,
  MANY_IMG_WEIGHT = 2,
  EXCESSIVE_IMG_WEIGHT = -1
};

enum NumImages : int {
  NOIMAGES = 0,
  FEWIMAGES = 5,
  MODERATEIMAGES = 15,
  MANYIMAGES = 25
};

enum NumLinks : int { NOLINKS = 0, MODERATELINKS = 250 };

enum NumLinksWeights : int {
  NOLINKSWEIGHT = 0,
  MODERATELINKSWEIGHT = 2,
  EXCESSIVELINKSWEIGHT = -1
};

enum DomainWeights : int {
  NONRECOGNIZED = -1,
  LESSRELEVANT = 1,
  RELEVANT = 2,
  ORG = 3,
  IMPORTANT = 4
};

enum UrlLengths : int {
  MODERATE_URL_LENGTH = 50,
  LONG_URL_LENGTH = 75,
  OVERLY_LONG_URL_LENGTH = 100
};

enum UrlLengthWeights : int {
  SHORT_URL_WEIGHT = 3,
  MODERATE_URL_WEIGHT = 2,
  LONG_URL_WEIGHT = 1,
  OVERLY_LONG_URL_WEIGHT = 0
};

enum TitleLengths : int {
  MODERATE_TITLE_LENGTH = 50,
  LONG_TITLE_LENGTH = 75,
  OVERLY_LONG_TITLE_LENGTH = 100
};

enum TitleLengthWeights : int {
  SHORT_TITLE_WEIGHT = 3,
  MODERATE_TITLE_WEIGHT = 2,
  LONG_TITLE_WEIGHT = 1,
  OVERLY_LONG_TITLE_WEIGHT = 0
};

namespace {
static constexpr double urlLengthWeight = 1.0;
static constexpr double titleLengthWeight = 1.0;
static constexpr double articleLengthWeight = 2.5;
static constexpr double articleWeights[] = {-1.0, 0.8, 1.0, 0.5, -1.0};
static constexpr double mobileFriendly =
    2.0;  // check if it has <meta name="viewport"
static constexpr double titleExists = 2.0;  // does it have title tag
static constexpr double usingHTTPS = 1.0;   // is it HTTPS

/*const static double veryShortArticle = -1.0;
const static double shortArticle = 0.8;
const static double mediumArticle = 1.0;
const static double longArticle = 0.5;
const static double veryLongArticle = -1.0;*/
}  // namespace

namespace {
DocumentLengthRank get_document_length(size_t n) {
  if (n < VERYSHORTLENGTH)
    return VERYSHORT;
  else if (n < SHORTLENGTH)
    return SHORT;
  else if (n < MEDIUMLENGTH)
    return MEDIUM;
  else if (n < LONGLENGTH)
    return LONG;
  else
    return VERYLONG;
}
}  // namespace

// Number of images in website multiplier, I don't know how to make this
// branchless
int get_numImages_weight(int occurences) {
  // if occurences less than 5 check 0, return 1 if 0, otherwise return 1.2
  if (occurences <= FEWIMAGES)
    return (occurences == NOIMAGES) ? NO_IMG_WEIGHT : FEW_IMG_WEIGHT;
  // if 6-15 images return 1.25
  else if (occurences <= MODERATEIMAGES)
    return MODERATE_IMG_WEIGHT;
  // if 16-25 return 1.2
  else if (occurences <= MANYIMAGES)
    return MANY_IMG_WEIGHT;
  // if more than 25 then slightly punish with a 0.75 multiplier
  return EXCESSIVE_IMG_WEIGHT;
}

// optimized for branch prediction
int get_numLinks_weight(int occurences) {
  if (occurences <= MODERATELINKS)
    return (occurences == NOLINKS) ? NOLINKSWEIGHT : MODERATELINKSWEIGHT;
  // if more than 250 outgoing links probably a spam site so punish with a
  // 0.8(might be a bibliography so dont punish too much)
  else
    return EXCESSIVELINKSWEIGHT;
}

struct DomainWeight {
  std::string domain;
  int weight;
};


// branchless
int get_domain_weight(cstring_view domain) {
  if (domain.size() < 2) return NONRECOGNIZED;
  switch (domain[1]) {
    case 'g':
      return domain == ".gov" ? IMPORTANT : NONRECOGNIZED;
    case 'c':
      return domain == ".com" ? RELEVANT : NONRECOGNIZED;
    case 'b':
      return domain == ".biz" ? LESSRELEVANT : NONRECOGNIZED;
    case 'e':
      return domain == ".edu" ? IMPORTANT : NONRECOGNIZED;
    case 'n':
      return domain == ".net" ? RELEVANT : NONRECOGNIZED;
    case 'o':
      return domain == ".org" ? ORG : NONRECOGNIZED;
    case 'i':
      return domain == ".info" ? LESSRELEVANT : NONRECOGNIZED;
    default:
      return NONRECOGNIZED;
  }
}

double get_document_length_weight(const decltype(HtmlParser::words) &words) {
  const size_t n = words.size();
  return articleWeights[get_document_length(n)];
}

// branchless
int get_url_length_weight(int length) {
  static const float weightsarr[] = {OVERLY_LONG_URL_WEIGHT, LONG_URL_WEIGHT,
                                     MODERATE_URL_WEIGHT, SHORT_URL_WEIGHT};
  // reward shorter urls
  return weightsarr[(length >= OVERLY_LONG_URL_LENGTH) ? 0
                    : (length >= LONG_URL_LENGTH)      ? 1
                    : (length >= MODERATE_URL_LENGTH)  ? 2
                                                       : 3];
}

cstring_view get_top_level_domain(cstring_view url) {
  size_t last_dot = url.rfind('.');
  if (last_dot != std::string::npos) {
    cstring_view cur = url.substr(last_dot);
    return cur.substr(0, cur.find('/'));
  }
  return cstring_view{};  // Return empty string if no dot is found
}

/*
float get_url_domain_weight(cstring_view url) {
   // Find the last '.' using cstring_view's find in a loop.
   size_t last_dot = cstring_view::npos;
   size_t pos = 0;
   // Use the find() method repeatedly to update last_dot
   while (pos < url.size()) {
      size_t found = url.find(".", pos);
      if (found == cstring_view::npos)
         break;
      last_dot = found;
      pos = found + 1;
   }
   // If we found a dot and there are characters after it:
   if (last_dot != cstring_view::npos && last_dot + 1 < url.size()) {
      cstring_view tld = url.substr(last_dot + 1, url.size() - last_dot - 1);
      // Create a key with a dot in front to match our hashmap keys, e.g.,
".com" std::string key = "." + std::string(tld.data(), tld.size()); return
get_domain_weight(key);
   }
   // Return default weight if no TLD is found.
   return 0.5f;
}
*/

// computes the static rank of an individual page given the URl and the parser
// object
double get_static_rank(cstring_view url, const HtmlParser &parser) {
  return get_numImages_weight(parser.img_count) +
         get_numLinks_weight(parser.links.size()) +
         get_domain_weight(get_top_level_domain(url)) +
         get_document_length_weight(parser.words) +
         get_url_length_weight(url.size());
}
