#pragma once

#include <string_view>

#include "isr/isr.h"
#include "ranker/dynamic_rank.h"

constexpr size_t TOTAL_DOCS_TO_RETURN = 100;

struct AnchorTermIndex
{
  int outerIndex;
  int innerIndex;

  AnchorTermIndex(int o, int i) : outerIndex(o), innerIndex(i) {} 
};
struct UrlRank 
{
  int rank;
  std::string_view url;

  bool operator<(const UrlRank& other) const { return rank < other.rank; }
  bool operator>(const UrlRank& other) const { return rank > other.rank; }
  bool operator>=(const UrlRank& other) const { return rank >= other.rank; }

  UrlRank() : url(""), rank(0) {}
  UrlRank(std::string_view u, int r) : url(u), rank(r) {}

};

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
  } else 
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

AnchorTermIndex get_anchor_ISR(vector<vector<std::unique_ptr<ISRWord>>> &orderedQueryTerms) {
     // Loop over ISR words vector to get the anchor term
     int anchorOuterIndex = -1;
     int anchorInnerIndex = -1; 
     int minOccurences = INT_MAX; 
     for (int i = 0; i < orderedQueryTerms.size(); i++) {
         for (int j = 0; j < orderedQueryTerms[i].size(); j++) {
            int occurences = orderedQueryTerms[i][j]->GetNumberOfOccurrences();
            if (occurences < minOccurences) {
                anchorOuterIndex = i;
                anchorInnerIndex = j;
                minOccurences = occurences;
            }
         }
     }
     assert(anchorOuterIndex != -1);
     assert(anchorInnerIndex != -1);
     return {anchorOuterIndex, anchorInnerIndex}; 
}

// actual constraint solver function
std::vector<UrlRank> constraint_solver(std::unique_ptr<ISR> &queryISR, vector<vector<std::unique_ptr<ISRWord>>> &orderedQueryTerms, uint32_t numChunks) {
    // create an ISR for document seeking
    std::vector<UrlRank> topNdocs;
    topNdocs.reserve(TOTAL_DOCS_TO_RETURN);

    AnchorTermIndex indices = get_anchor_ISR(orderedQueryTerms);
    int anchorOuterIndex = indices.outerIndex;
    int anchorInnerIndex = indices.innerIndex;
    // seek to the first occurence
    IndexFileReader reader(numChunks);
    for (int i = 0; i < numChunks; ++i)
    {
      SeekObj * docObj = queryISR->NextDocument(); 
      while (docObj) {
  
          int docStartLoc = queryISR->getStartLocation(); 
          int docEndLoc = queryISR->getEndLocation(); 
  
          // use the index to get relevant doc data
          unique_ptr<Doc> doc = reader.FindUrl(docObj->index, i);
          
          int dynamic_score = get_dynamic_rank(orderedQueryTerms[anchorOuterIndex][anchorInnerIndex], orderedQueryTerms, docStartLoc, 
                              docEndLoc);
          

          UrlRank urlRank = {doc->url, dynamic_score + doc->staticRank}; 

          insertionSort(topNdocs,urlRank); 
  
          docObj = queryISR->NextDocument(); 
      }
    }
    return topNdocs; 
}