#pragma once

#include <string_view>

#include "isr.h"
#include "ranker/dynamic_rank.h"

constexpr size_t TOTAL_DOCS_TO_RETURN = 100;
struct UrlRank 
{
  std::string_view url;
  int rank;

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

// returns outer index as the first element of the pair and the inner index as the second element of the pair
pair<int, int> get_anchor_ISR(vector<vector<ISRWord*>> orderedQueryTerms) {
     // Loop over ISR words vector to get the anchor term
     int anchorOuterIndex = -1;
     int anchorInnerIndex = -1; 
     int minOccurences = INT_MAX; 
     for (int i = 0; i < orderedQueryTerms.size(); i++) {
         for (int j = 0; j < orderedQueryTerms[i].size(); j++) {
             if (orderedQueryTerms[i][j]->GetNumberOfOccurrences() < minOccurences) {
                 anchorOuterIndex = i;
                 anchorInnerIndex = j;
                 minOccurences = orderedQueryTerms[i][j]->GetNumberOfOccurrences();
             }
         }
     }
     assert(anchorOuterIndex != -1);
     assert(anchorInnerIndex != -1);
     return {anchorOuterIndex, anchorInnerIndex}; 
    
}

// actual constraint solver function
std::vector<pair<cstring_view, int>> constraint_solver(ISR* queryISR, vector<vector<ISRWord*>> orderedQueryTerms) {
    // create an ISR for document seeking
    vector<pair<cstring_view, int>> returnedResults;
    auto indices = get_anchor_ISR(orderedQueryTerms);
    int anchorOuterIndex = indices.first;
    int anchorInnerIndex = indices.second;
    int currElements = 0; 
    // seek to the first occurence
    Post* currPost = queryISR->NextDocument(); 
    while (currPost) {
        int docStartLoc = queryISR->getDocStartLoc(); 
        int docEndLoc = queryISR->getDocEndLoc(); 
        // TO DO use the relative loc to get relevant doc data
        int relativeLoc = queryISR->GetCurrentPost()->location;
        assert(false);

        // TO DO: Figure out how to get the document URL
        assert(false); 
        cstring_view documentURL; 

        // TO DO: Update this function call with actual values and figure out how to determine if its in the title or body
        assert(false); 
        int dynamic_score = get_dynamic_rank(orderedQueryTerms[anchorOuterIndex][anchorInnerIndex], orderedQueryTerms, docStartLoc, 
                            docEndLoc, "\0");
        
        // TO DO: get static rank and add it to the dynamic rank
        pair<cstring_view, int> urlPair = {documentURL, dynamic_score + static_score}; 

        TopN(returnedResults, urlPair, 10, currElements); 
        currPost = queryISR->NextDocument(); 
    }
    return returnedResults; 
}

    
    
