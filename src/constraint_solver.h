#pragma once

#include <string_view>
#include <cctype>
#include <algorithm>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <array>

#include "isr/isr.h"
#include "ranker/dynamic_rank.h"


constexpr size_t TOTAL_DOCS_TO_RETURN = 32;
/** @brief Weights for url matching */
constexpr size_t HOST_MATCH_SCORE = 256;
constexpr size_t PATH_MATCH_SCORE = 128;
constexpr size_t HOST_MATCHED_ALL_QUERY_TERMS = 128;
constexpr size_t PATH_MATCHED_ALL_QUERY_TERMS = 64;
constexpr size_t SHORT_URL_BOOST = 64;
/** @brief max len for url to be considered short */
constexpr size_t MAX_SHORT_URL_LEN = 16;

static bool bodyText = true;
/** @brief function to compute precompute reciprocals so we don't have to do division during runtime */
constexpr std::array<float, 256> createReciprocalTable() 
{
  std::array<float, 256> table = {};
  for (int i = 1; i < 256; ++i) 
  {
    table[i] = 1.0f / static_cast<float>(i);
  }
  table[0] = 0.0f; 
  return table;
}
/** @brief function to compute the shortest span possible for given query len
 * limits query size to 255
 */
constexpr std::array<int, 256> createShortestSpanTable()
{
  std::array<int, 256> table = {};
  for (int i = 1; i < 256; ++i)
  {
    table[i] = (i * (i + 1)) / 2;
  }
  table[0] = 0;
  return table;
}

/** @brief precompute tables at compile time
 * Doubt it saves much time though
 */
constexpr auto RECIPROCAL_TABLE = createReciprocalTable();
constexpr auto SHORTEST_SPAN_TABLE = createShortestSpanTable();


float matchScore(int queryLen, int partialUrlLen) 
{
  if (partialUrlLen == 0) return 0.0f;
  return queryLen * RECIPROCAL_TABLE[partialUrlLen];
}

int getShortestSpan(int queryLen)
{
  return SHORTEST_SPAN_TABLE[queryLen];
}

std::string to_lower_copy(const std::string& input) 
{
  std::string result;
  result.reserve(input.length()); 
  std::transform(
      input.begin(),
      input.end(),
      std::back_inserter(result),
      [](unsigned char c) { return std::tolower(c); }
  );
  return result;
}

struct ParsedUrlRanking 
{
  std::string host;               // e.g., "www.example.com", "example.co.uk:8080"
  std::string path;               // e.g., "/", "/path/to/doc.html"
  std::string first_path_segment; // e.g., "path" (from "/path/to/doc"), "blog" (from "/blog/")
  bool isValid() const 
  {
      return !host.empty();
  }
};

ParsedUrlRanking parseUrl(const std::string& url_string) {
  ParsedUrlRanking result;
  result.path = "/";

  size_t scheme_end = url_string.find("://");
  if (scheme_end == std::string::npos) 
  {
      return result;
  }
  size_t authority_start = scheme_end + 3;

  size_t authority_end = url_string.find_first_of("/?#", authority_start);

  if (authority_end == std::string::npos) 
  {
      result.host = url_string.substr(authority_start);
  } 
  else 
  {
      result.host = url_string.substr(authority_start, authority_end - authority_start);
      
      size_t path_end = url_string.find_first_of("?#", authority_end);
      if (path_end == std::string::npos) 
      {
          result.path = url_string.substr(authority_end);
      } 
      else 
      {
          result.path = url_string.substr(authority_end, path_end - authority_end);
      }
      if (result.path.empty() && url_string[authority_end] == '/') 
      {
           result.path = "/";
      }
  }

  std::string lower_url_temp = to_lower_copy(url_string);
  if (lower_url_temp.find("wikipedia") != std::string::npos) {
      result.host = to_lower_copy(result.host);
      result.path = to_lower_copy(result.path);
  }

  if (result.path.length() > 1 && result.path[0] == '/') 
  { // Path needs to be longer than just "/"
      size_t first_segment_end = result.path.find('/', 1); // Find the *next* slash after the first char
      if (first_segment_end == std::string::npos) 
      {
          // No more slashes, the segment is the rest of the path
          result.first_path_segment = result.path.substr(1);
      } 
      else if (first_segment_end > 1) 
      {
           // Segment ends before the next slash
          result.first_path_segment = result.path.substr(1, first_segment_end - 1);
      }
      // If first_segment_end is 1 (e.g., path "//"), segment remains empty.
  }

  // Basic cleanup: remove trailing slash from segment if it exists and is the only char
  if (!result.first_path_segment.empty() && result.first_path_segment.back() == '/') 
  {
       result.first_path_segment.pop_back();
  }
  // Replaces hyphens and underscores with spaces
  std::replace(result.path.begin(), result.path.end(), '-', ' ');
  std::replace(result.path.begin(), result.path.end(), '_', ' ');
  std::replace(result.host.begin(), result.host.end(), '-', ' ');
  std::replace(result.host.begin(), result.host.end(), '_', ' ');

  return result;
}
struct UrlRank 
{
  int rank;
  std::string url;

  bool operator<(const UrlRank& other) const { return rank < other.rank; }
  bool operator>(const UrlRank& other) const { return rank > other.rank; }
  bool operator>=(const UrlRank& other) const { return rank >= other.rank; }

  UrlRank() : rank(0), url("") {}
  UrlRank(std::string u, int r) : rank(r), url(std::move(u)) {}
  // UrlRank(std::string_view u, int r) : rank(r), url(u) {}
  // UrlRank(std::string_view u, uint8_t r) : rank(r), url(u) {}

};

struct WordSortInfo 
{
  int outerIndex;
  int innerIndex;
  int occurrences;

  bool operator<(const WordSortInfo& other) const 
  {
      if (occurrences != other.occurrences) 
      {
          return occurrences < other.occurrences;
      }
      if (outerIndex != other.outerIndex) 
      {
          return outerIndex < other.outerIndex;
      }
      return innerIndex < other.innerIndex;
  }
};

std::vector<AnchorTermIndex> getRarestIndices(
  const std::vector<std::vector<std::unique_ptr<ISRWord>>>& orderedQueryTerms)
{
  std::vector<WordSortInfo> allWordInfo;
  allWordInfo.reserve(orderedQueryTerms.size() * 10);
  for (int i = 0; i < orderedQueryTerms.size(); ++i) 
  {
      for (int j = 0; j < orderedQueryTerms[i].size(); ++j) 
      {
          if (orderedQueryTerms[i][j]) 
          {
               int occurrences = orderedQueryTerms[i][j]->GetNumberOfOccurrences();
               allWordInfo.push_back({i, j, occurrences});
          } 
      }
  }

  std::sort(allWordInfo.begin(), allWordInfo.end());

  std::vector<AnchorTermIndex> sortedIndices;
  sortedIndices.reserve(allWordInfo.size()); 
  for (const auto& info : allWordInfo) {
      sortedIndices.push_back({info.outerIndex, info.innerIndex});
  }

  return sortedIndices;
}

void insertionSort(vector<UrlRank> & topRankedDocs, UrlRank & rankedDoc) 
{
  const size_t maxSize = TOTAL_DOCS_TO_RETURN;
  size_t curSize = topRankedDocs.size();
  if (curSize == TOTAL_DOCS_TO_RETURN && topRankedDocs.back() >= rankedDoc)
  {
    return;
  }
  UrlRank docToInsert = rankedDoc;
  size_t pos;
  if (curSize < maxSize) 
  { 
    topRankedDocs.emplace_back();
    pos = curSize;
  } 
  else 
  {
    pos = curSize - 1;
  }
  while (pos > 0 && topRankedDocs[pos - 1] < docToInsert) 
  {
    topRankedDocs[pos] = std::move(topRankedDocs[pos - 1]);
    pos--;
  }
  topRankedDocs[pos] = std::move(docToInsert);
}

bool isSpecificWordInPath(const std::string& path, const std::string& word) 
{
  if (word.empty() || path.length() < word.length()) return false;

      size_t foundPos = path.find(word);

      if (foundPos == std::string::npos) {
          return false;
      }

      bool beforeOk = (foundPos == 0) || (path[foundPos - 1] == '/');

      size_t afterPos = foundPos + word.length();
      bool afterOk = (afterPos == path.length()) ||
                      (afterPos < path.length() && (path[afterPos] == '/'));

      if (beforeOk && afterOk) {
          return true;
      }

  return false;
}
float calculateURLscore(const ParsedUrlRanking & parsedUrl,
                       const std::vector<std::vector<std::unique_ptr<ISRWord>>> &orderedQueryTerms)
{
    float hostMatchScore = 0.0f;
    float pathMatchScore = 0.0f;
    size_t currScore = 0;
    for (int i = 0; i < orderedQueryTerms.size(); ++i)
    {
      size_t currQueryVectorSize = orderedQueryTerms[i].size();
      size_t currHostMatches = 0;
      size_t currPathMatches = 0;
      for (int j = 0; j < currQueryVectorSize; ++j)
      {
        const auto &wordISR = orderedQueryTerms[i][j];
        if (!wordISR) continue;
        const std::string_view word = wordISR->GetWord();
        if (word.empty()) continue;
        if (!parsedUrl.host.empty() && parsedUrl.host.find(word) != std::string_view::npos)
        {
          float curr = matchScore(word.size(), parsedUrl.host.size());
          ++currHostMatches;
          if (curr > hostMatchScore)
          {
            hostMatchScore = curr;
          }
        }

        if (!parsedUrl.path.empty() && parsedUrl.path.find(word) != std::string_view::npos)
        {
          float curr = matchScore(word.size(), parsedUrl.path.size());
          ++currPathMatches;
          if (curr > pathMatchScore)
          {
            pathMatchScore = curr;
          }
        }
      } // End inner loop (words)
      if (currHostMatches == currQueryVectorSize && currQueryVectorSize > 1)
      {
        currScore += HOST_MATCHED_ALL_QUERY_TERMS;
      }
      if (currPathMatches == currQueryVectorSize && currQueryVectorSize > 1)
      {
        currScore += PATH_MATCHED_ALL_QUERY_TERMS;
      }
    } // End outer loop (term vectors)
    if (parsedUrl.host.size() <= MAX_SHORT_URL_LEN)
    {
      if (hostMatchScore > 0.5f)
      {
        return HOST_MATCH_SCORE + currScore;
      }
      if (hostMatchScore > 0.3f)
      {
        return (HOST_MATCH_SCORE >> 1) + currScore;
      }
      if (hostMatchScore > 0.15f)
      {
        return (HOST_MATCH_SCORE >> 2) + currScore;
      }
    }
    else
    {
      if (hostMatchScore > 0.7f)
      {
        return HOST_MATCH_SCORE + currScore;
      }
      if (hostMatchScore > 0.5f)
      {
        return (HOST_MATCH_SCORE >> 1) + currScore;
      }
      if (hostMatchScore > 0.3f)
      {
        return (HOST_MATCH_SCORE >> 2) + currScore;
      }
    }
    if (parsedUrl.path.size() <= MAX_SHORT_URL_LEN)
    {
      if (pathMatchScore > 0.5f)
      {
        return HOST_MATCH_SCORE + currScore;
      }
      else if (pathMatchScore > 0.3f)
      {
        return (HOST_MATCH_SCORE >> 1) + currScore;
      }
      else if (pathMatchScore > 0.15f)
      {
        return (HOST_MATCH_SCORE >> 2) + currScore;
      }
    }
    else
    {
      if (pathMatchScore > 0.7f)
      {
        return PATH_MATCH_SCORE + currScore;
      }
      else if (pathMatchScore > 0.5f)
      {
        return (PATH_MATCH_SCORE >> 1) + currScore;
      }
      else if (pathMatchScore > 0.3f)
      {
        return (PATH_MATCH_SCORE >> 2) + currScore;
      }
    }
    return currScore;
}
int calculateSingleWordQueryScore(const ParsedUrlRanking & parsedUrl,
  const std::unique_ptr<ISRWord> & queryWord)
  {
    auto foundWiki = parsedUrl.host.find("wikipedia");
    auto foundDict = parsedUrl.host.find("dictionary");
    const auto & word = queryWord->GetWord();
    if (foundWiki == std::string::npos && foundDict == std::string::npos) 
    {
      return 0;
    }
    if (isSpecificWordInPath(parsedUrl.path, word))
    {
      return 1000;
    }
    return 0;
  }

// actual constraint solver function
std::vector<UrlRank> constraint_solver(
  std::unique_ptr<ISR> &queryISR,
  vector<vector<std::unique_ptr<ISRWord>>> &orderedQueryTerms,
  uint32_t numChunks,
  IndexFileReader& reader,
  int& matches,
  int queryLength
) 
  {
    auto rarestTermInOrder = getRarestIndices(orderedQueryTerms);
    // create an ISR for document seeking
    std::vector<UrlRank> topNdocs;
    topNdocs.reserve(TOTAL_DOCS_TO_RETURN);
    int matchedDocs = 0;
    
    // Copy orderedQueryTerms for the title terms
    std::vector<std::vector<std::unique_ptr<ISRWord>>> titleTerms;
    for (const auto& innerVec : orderedQueryTerms) {
        std::vector<std::unique_ptr<ISRWord>> copiedInner;
        for (const auto& termPtr : innerVec) {
            if (termPtr) {
                copiedInner.emplace_back(std::make_unique<ISRWord>(termPtr->GetWord() + "!", reader));
            }
        }
        titleTerms.push_back(std::move(copiedInner));
    }
    std::unique_ptr<ISREndDoc> docISR = make_unique<ISREndDoc>(reader); 
    for (int chunkNum = 0; chunkNum < numChunks; ++chunkNum)
    {
      SeekObj* currMatch = queryISR->Seek(0, chunkNum);
      if (!currMatch)
      {
        continue;
      }
      SeekObj * docObj = docISR->Seek(currMatch->location, chunkNum);
      while (docObj) 
      {
          ++matchedDocs;
          int currLoc = docObj->location;
          int currDelta = docObj->delta;
          int index = docObj->index;

          int docEndLoc = currLoc;
          int docStartLoc = currLoc - currDelta;

          // use the index to get relevant doc data
          unique_ptr<Doc> doc = reader.FindUrl(index, chunkNum);
          if (!doc) break;

          int shortestSpanPossible = getShortestSpan(queryLength);
          if (shortestSpanPossible > 1 || doc->url.size() < 64)
          {
              ParsedUrlRanking parsedUrl = parseUrl(doc->url);
              int url_score = calculateURLscore(parsedUrl, orderedQueryTerms);
              if (parsedUrl.path.size() <= MAX_SHORT_URL_LEN) url_score += SHORT_URL_BOOST;
              if (shortestSpanPossible == 1)
              {
                url_score += calculateSingleWordQueryScore(parsedUrl, orderedQueryTerms[0][0]);
              }
              int dynamic_score = get_dynamic_rank(
                rarestTermInOrder,
                orderedQueryTerms, 
                docStartLoc, 
                docEndLoc,
                reader,
                chunkNum,
                bodyText,
                shortestSpanPossible);
              // std::cout << "Dynamic rank: " << dynamic_score << '\n';
              // Dynamic score for title words
  /*          
              int title_score = get_dynamic_rank(
                rarestTermInOrder,
                titleTerms,
                docStartLoc,
                docEndLoc,
                reader,
                chunkNum,
                !bodyText,
                shortestSpanPossible);
*/              
              // if (parsedUrl.host.size() < MAX_SHORT_URL_LEN) url_score += SHORT_URL_BOOST;
              // Add weights to the score later
              UrlRank urlRank(doc->url, dynamic_score + url_score); /*title_score + url_score + doc->staticRank*/
              // cout << urlRank.rank << '\n';
              insertionSort(topNdocs, urlRank); 
          }
        
          currMatch = queryISR->NextDocument(currLoc, chunkNum); 
          if (!currMatch) 
          {
            break;
          }
          docObj = docISR->Seek(currMatch->location, chunkNum);
      }
    }
    cout << "MATCHED DOCUMENTS: " << matchedDocs << '\n';
    matches = matchedDocs;
    return topNdocs; 
}
