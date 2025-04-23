#pragma once


#include <stdint.h>


#include <algorithm>
#include <vector>
#include <climits>
#include <string>
#include <unordered_map>
#include "../isr/isr.h"
#include "../inverted_index/IndexFileReader.h"


struct AnchorTermIndex
{
 int outerIndex;
 int innerIndex;


 AnchorTermIndex(int o, int i) : outerIndex(o), innerIndex(i) {}
};
// TO DO: WEIGHTS ARE ALL 0 RIGHT NOW, WILL NEED TO SET THEM TO REAL VALUES SOON AND DEBUG THEM TO FIND WHICH WEIGHTS ARE THE BEST FOR
//       OUR RESULTS FOR THE ENGINE


// Weights for computing the score of specific ranking components, enums for quick readjusting and no magic numbers
enum DynamicWeights: int {
   SHORTSPANSWEIGHT = 32,
   TOPSPANSWEIGHT = 16,
   NEARTOPWEIGHT = 1,
};


// Weights for the section the dynamic ranking heuristic is being ranked on, e.g if its being ran on the URL of a document or the body
// enums for quick readjusting and no magic numbers once again
enum SectionWeights: int {
   TITLEWEIGHT = 4,
   BODYWEIGHT = 1
};


// enum class for requirements of what is considered a short span or a top span
enum Requirements: int {
   TOPSPANSIZE = 256
};


using locationVector = std::vector<Location>;


constexpr Location RANGE_TOLERANCE = 32;
constexpr int MAX_SPANS = 8;

// given all of the occurences assigns the weights and returns the actual dynamic rank
// ***RME CLAUSE***
// Requires: The number of short spans, ordered spans, phrase matches, top spans, and the type of section this is being scored on
// Modifies: Nothing
// Effect: Scores a given rank of the document using weights with the given numbers above.
int get_rank_score(int nearTopAnchorCount, int shortSpans, int topSpans, bool isBody) {
   int sectionWeight = -1;
   // assign the weights based on what we are running the ranker on
   // urls will get higher weights than titles, titles will get higher weights than body, and body gets a normal weight
   if (isBody) {
       sectionWeight = BODYWEIGHT;
   }
   else {
       sectionWeight = TITLEWEIGHT;
   }
   assert(sectionWeight != -1);
   shortSpans = std::min(MAX_SPANS, shortSpans);
   topSpans = std::min(MAX_SPANS, topSpans);
   // declare individual ints below for debugging purposes
   int shortSpansResult = shortSpans * SHORTSPANSWEIGHT;
   int nearTopAnchorResult = nearTopAnchorCount;
   int topSpansResult = topSpans * TOPSPANSWEIGHT;
   // multiply by the type weight and the added up weights of the spans
   return sectionWeight * (shortSpansResult + nearTopAnchorResult + topSpansResult);
}


// computes all of the variables that is needed for the rank score function using a variety of ISR'S pertaining to a SPECIFIC document
// ***RME CLAUSE***
// Requires: The rarest word's ISR as the anchor term, and vector of ISR's all other terms w/the anchor term being in the vector as well
//           the int is the index of the phrase term in the search query to check for ordering.
//           phraseTerms should be ordered like they are in the query, need a vector of vectors incase we have multiplle phraes in the query tree
//           Needs a start location so we can seek to that location
// Modifies: Nothing.
// Effect: Returns the dynamic rank score for a single document.
int get_dynamic_rank(const std::vector<AnchorTermIndex> &rarestAnchorTermVectors, vector<vector<std::unique_ptr<ISRWord>>> &phraseTerms, uint64_t startLocation, uint64_t endLocation, const IndexFileReader & reader, uint32_t currChunk, bool isBody, int shortestSpanPossible)
{
   int shortSpans = 0;
   int topSpans = 0;
   int totalSpans = 0;
   int nearTopAnchor = 0;
   for (int anchorTermIdx = 0; anchorTermIdx < rarestAnchorTermVectors.size(); ++anchorTermIdx)
   {
       auto & anchorTerm = phraseTerms[rarestAnchorTermVectors[anchorTermIdx].outerIndex][rarestAnchorTermVectors[anchorTermIdx].innerIndex];


       const std::string & anchorWord = anchorTerm->GetWord();
       locationVector anchorLocations = reader.LoadChunkOfPostingList(
           anchorWord,
           currChunk,  
           startLocation,
           endLocation,
           anchorTerm->GetSeekTableIndex()
       );
     
       if (!anchorLocations.empty())
       {
            if (shortestSpanPossible == 1)
            {  
                for (const auto & loc : anchorLocations)
                {
                    if (loc < startLocation + Requirements::TOPSPANSIZE)
                    {
                        ++nearTopAnchor;
                    }
                }
            return nearTopAnchor;
            }
           locationVector spans(anchorLocations.size(), 0);
  
           for (int i = 0; i < phraseTerms.size(); ++i)
           {
               for (int j = 0; j < phraseTerms[i].size(); ++j)
               {
                   const std::string & word = phraseTerms[i][j]->GetWord();
                   if (word != anchorWord)
                   {
                       locationVector currPhraseMinDiffs = reader.FindClosestPostingDistancesToAnchor
                       (
                       word,
                       currChunk,
                       anchorLocations,
                       endLocation,
                       phraseTerms[i][j]->GetSeekTableIndex()
                       );
                       for (int k = 0; k < spans.size(); ++k)
                       {
                           spans[k] += currPhraseMinDiffs[k];
                       }
                   }
                  
               }
           }


           for (int i = 0; i < spans.size(); ++i)
           {  
               Location & currentTotalSpan = spans[i];

               if (currentTotalSpan < (shortestSpanPossible << 1) && currentTotalSpan != 0)
               {
                   shortSpans++;
                   if (anchorLocations[i] < startLocation + Requirements::TOPSPANSIZE)
                   {
                       topSpans++;
                   }
               }
               if (anchorLocations[i] < startLocation + Requirements::TOPSPANSIZE)
               {
                   ++nearTopAnchor;
               }
              
           }
           break;
       }
   }
   /*std::cout << "shortSpans: " << shortSpans << ", orderedSpans: " << orderedSpans << "phraseMatches: " << phraseMatches << ", topSpans: " << topSpans << "isBody: " << isBody << '\n';*/
   return get_rank_score(nearTopAnchor, shortSpans , topSpans, isBody);
}