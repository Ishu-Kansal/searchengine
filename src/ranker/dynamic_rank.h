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
// Requires: The number of short minSpansVector, ordered minSpansVector, phrase matches, top minSpansVector, and the type of section this is being scored on
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
   // multiply by the type weight and the added up weights of the minSpansVector
   return sectionWeight * (shortSpansResult + nearTopAnchorResult + topSpansResult);
}


// computes all of the variables that is needed for the rank score function using a variety of ISR'S pertaining to a SPECIFIC document
// ***RME CLAUSE***
// Requires: The rarest word's ISR as the anchor term, and vector of ISR's all other terms w/the anchor term being in the vector as well
// the int is the index of the phrase term in the search query to check for ordering.
//   phraseTerms should be ordered like they are in the query, need a vector of vectors incase we have multiplle phrares in the query tree
// Needs a start location so we can seek to that location
// Modifies: Nothing.
// Effect: Returns the dynamic rank score for a single document.
int get_dynamic_rank(const std::vector<AnchorTermIndex> &rarestAnchorTermVectors, vector<vector<std::unique_ptr<ISRWord>>> &phraseTerms, uint64_t startLocation, uint64_t endLocation, const IndexFileReader & reader, uint32_t currChunk, bool isBody, int shortestSpanPossible)
{
   int shortSpans = 0;
   int topSpans = 0;
   int totalSpans = 0;
   int nearTopAnchor = 0;
    // Loop through all query terms to find anchor incase, the number one rarest term is not present in this document
   for (int anchorTermIdx = 0; anchorTermIdx < rarestAnchorTermVectors.size(); ++anchorTermIdx)
   {
       auto & anchorTerm = phraseTerms[rarestAnchorTermVectors[anchorTermIdx].outerIndex][rarestAnchorTermVectors[anchorTermIdx].innerIndex];

       const std::string & anchorWord = anchorTerm->GetWord();
       // Attempts to load all postings in this document for the current anchor
       locationVector anchorLocations = reader.LoadChunkOfPostingList(
           anchorWord,
           currChunk,  
           startLocation,
           endLocation,
           anchorTerm->GetSeekTableIndex()
       );
       
       if (!anchorLocations.empty())
       {
            // If the query is one term long
            // count the number of occurence near the top of the document and treat it as its rank
            if (shortestSpanPossible == 1)
            {  
                Location threshold = startLocation + Requirements::TOPSPANSIZE;
                if (anchorLocations.size() > 32)
                {
                    auto it_first_outside_range = std::lower_bound(
                        anchorLocations.begin(),
                        anchorLocations.end(),
                        threshold);

                    nearTopAnchor = std::distance(anchorLocations.begin(), it_first_outside_range);

                    return nearTopAnchor;
                }
                else
                {
                    for (const auto & loc : anchorLocations) 
                    {
                        if (loc < threshold) nearTopAnchor++;
                        else return nearTopAnchor;
                    }
                }
            }
            // Vector to hold min span calculations for anchor word
            locationVector minSpansVector(anchorLocations.size(), 0);
            
            // Loops through every term
            for (int i = 0; i < phraseTerms.size(); ++i)
            {
               for (int j = 0; j < phraseTerms[i].size(); ++j)
               {
                    const std::string & word = phraseTerms[i][j]->GetWord();
                    if (word != anchorWord)
                    {
                        // Calculates min distance between current term and anchor term in this document
                        locationVector currPhraseMinDiffs = reader.FindClosestPostingDistancesToAnchor
                        (
                        word,
                        currChunk,
                        anchorLocations,
                        endLocation,
                        phraseTerms[i][j]->GetSeekTableIndex()
                        );
                        // Adds the min distances to current span
                        for (int k = 0; k < minSpansVector.size(); ++k)
                        {
                            minSpansVector[k] += currPhraseMinDiffs[k];
                        }
                    }
                  
               }
           }
           // Once we are done calculating the min spans
           // Check if the min spans match our requirements
           for (int i = 0; i < minSpansVector.size(); ++i)
           {  
                Location & currentTotalSpan = minSpansVector[i];

                // Checks if its a short span
                if (currentTotalSpan < (shortestSpanPossible << 1) && currentTotalSpan != 0)
                {
                    shortSpans++;
                    // Checks if its a top span
                    if (anchorLocations[i] < startLocation + Requirements::TOPSPANSIZE)
                    {
                        topSpans++;
                    }
                }
                // Checks if the anchor word is near the top of the page
                if (anchorLocations[i] < startLocation + Requirements::TOPSPANSIZE)
                {
                    ++nearTopAnchor;
                }
           }
           break;
       }
   }
   return get_rank_score(nearTopAnchor, shortSpans , topSpans, isBody);
}