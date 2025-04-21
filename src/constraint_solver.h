#pragma once

#include <string_view>
#include <cctype>
#include <algorithm>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <sstream>
#include "isr/isr.h"
#include "ranker/dynamic_rank.h"

constexpr size_t TOTAL_DOCS_TO_RETURN = 100;
constexpr size_t HOST_MATCH_SCORE = 100;
constexpr size_t PATH_MATCH_SCORE = 50;
constexpr size_t SEGEMENT_MATCH_SCORE = 25;

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

constexpr auto RECIPROCAL_TABLE = createReciprocalTable();
constexpr auto SHORTEST_SPAN_TABLE = createShortestSpanTable();

float HostMatchScore(int queryLen, int hostLen) 
{
  if (hostLen == 0) return 0.0f;
  return queryLen * RECIPROCAL_TABLE[hostLen];
}

int getShortestSpan(int queryLen)
{
  return SHORTEST_SPAN_TABLE[queryLen];
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

int calculateURLscore(std::string url)
{

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
    
    for (int chunkNum = 0; chunkNum < numChunks; ++chunkNum)
    {
      SeekObj* currMatch = queryISR->Seek(0, chunkNum);
      std::unique_ptr<ISREndDoc> docISR = make_unique<ISREndDoc>(reader); 
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
          
          int dynamic_score = get_dynamic_rank(
            rarestTermInOrder,
            orderedQueryTerms, 
            docStartLoc, 
            docEndLoc,
            reader,
            chunkNum,
            true);
          // std::cout << "Dynamic rank: " << dynamic_score << '\n';
          // Dynamic score for title words
          /*
          int title_score = get_dynamic_rank(
            titleTerms[anchorOuterIndex][anchorInnerIndex],
            titleTerms,
            docStartLoc,
            docEndLoc,
            reader,
            chunkNum,
            false);
            */
          // if (title_score > 0) {
          //   std::cout << "Title score: " << title_score << " URL: " << doc->url << std::endl;
          // }

          int url_score = calculateURLscore(doc->url);
          // Add weights to the score later
          UrlRank urlRank(doc->url, dynamic_score + url_score); /*title_score + url_score + doc->staticRank*/
          // cout << urlRank.rank << '\n';
          insertionSort(topNdocs, urlRank); 
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