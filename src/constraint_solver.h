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
  std::string url;

  bool operator<(const UrlRank& other) const { return rank < other.rank; }
  bool operator>(const UrlRank& other) const { return rank > other.rank; }
  bool operator>=(const UrlRank& other) const { return rank >= other.rank; }

  UrlRank() : url(""), rank(0) {}
  UrlRank(std::string u, int r) : url(std::move(u)), rank(r) {}
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
std::vector<UrlRank> constraint_solver(
  std::unique_ptr<ISR> &queryISR,
  vector<vector<std::unique_ptr<ISRWord>>> &orderedQueryTerms,
  uint32_t numChunks,
  IndexFileReader& reader) 
  {
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
                copiedInner.emplace_back(std::make_unique<ISRWord>(termPtr->GetWord() + "!"));
            }
        }
        titleTerms.push_back(std::move(copiedInner));
    }

    AnchorTermIndex indices = get_anchor_ISR(orderedQueryTerms);
    int anchorOuterIndex = indices.outerIndex;
    int anchorInnerIndex = indices.innerIndex;

    SeekObj* currMatch = queryISR->Seek(0);
    for (int i = 0; i < numChunks; ++i)
    {
      std::unique_ptr<ISREndDoc> docISR = make_unique<ISREndDoc>(reader); 
      SeekObj * docObj = docISR->Seek(currMatch->location);
      while (docObj) 
      {
          ++matchedDocs;
          int currLoc = docObj->location;
          int currDelta = docObj->delta;
          int index = docObj->index;

          int docEndLoc = currLoc;
          int docStartLoc = currLoc - currDelta;
  
          // use the index to get relevant doc data
          unique_ptr<Doc> doc = reader.FindUrl(index, i);
          
          int dynamic_score = get_dynamic_rank(
            orderedQueryTerms[anchorOuterIndex][anchorInnerIndex],
            orderedQueryTerms, 
            docStartLoc, 
            docEndLoc,
            reader,
            i);

          // Dynamic score for title words
          int title_score = get_dynamic_rank(
            titleTerms[anchorOuterIndex][anchorInnerIndex],
            titleTerms,
            docStartLoc,
            docEndLoc,
            reader,
            i);

          // Add weights to the score later
          UrlRank urlRank = {doc->url, dynamic_score + title_score + doc->staticRank}; 
  
          insertionSort(topNdocs, urlRank); 
          currMatch = queryISR->NextDocument(currLoc); 
          if (!currMatch) 
          {
            break;
          }
          docObj = docISR->Seek(currMatch->location);
    }
   
    }
    cout << "MATCHED DOCUMENTS:" << matchedDocs << '\n';
    return topNdocs; 
}