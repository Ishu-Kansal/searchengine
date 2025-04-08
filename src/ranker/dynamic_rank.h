#include <stdint.h>
#include <vector>
#include <climits>
#include <string>
#include <unordered_map>
#include "../isr/isr.h"

// TO DO: WEIGHTS ARE ALL 0 RIGHT NOW, WILL NEED TO SET THEM TO REAL VALUES SOON AND DEBUG THEM TO FIND WHICH WEIGHTS ARE THE BEST FOR 
//       OUR RESULTS FOR THE ENGINE

// Weights for computing the score of specific ranking components, enums for quick readjusting and no magic numbers
enum DynamicWeights: int {
    SHORTSPANSWEIGHT = 0, 
    ORDEREDSPANSWEIGHT = 0,
    PHRASEMATCHESWEIGHT = 0,
    TOPSPANSWEIGHT = 0
}; 

// Weights for the section the dynamic ranking heuristic is being ranked on, e.g if its being ran on the URL of a document or the body
// enums for quick readjusting and no magic numbers once again 
enum SectionWeights: int {
    TITLEWEIGHT = 0,
    BODYWEIGHT = 0
};

// enum class for requirements of what is considered a short span or a top span
enum Requirements: int {
    SHORTSPANSIZE = 0,
    TOPSPANSIZE = 0
};

// given all of the occurences assigns the weights and returns the actual dynamic rank
// ***RME CLAUSE***
// Requires: The number of short spans, ordered spans, phrase matches, top spans, and the type of section this is being scored on
// Modifies: Nothing
// Effect: Scores a given rank of the document using weights with the given numbers above.
int get_rank_score(int shortSpans, int orderedSpans, int phraseMatches, int topSpans, cstring_view type) {
    // TO DO: make a hashtable/array for the type weights
    int sectionWeight = -1;
    // assign the weights based on what we are running the ranker on
    // urls will get higher weights than titles, titles will get higher weights than body, and body gets a normal weight
    if (type == "body") {
        sectionWeight =  BODYWEIGHT;
    }
    else if (type == "title") {
        sectionWeight = TITLEWEIGHT;
    }
    assert(sectionWeight != -1); 
    // declare individual ints below for debugging purposes
    int shortSpansResult = shortSpans * SHORTSPANSWEIGHT;
    int orderedSpansResult = orderedSpans * ORDEREDSPANSWEIGHT;
    int phraseMatchesResult = phraseMatches * PHRASEMATCHESWEIGHT;
    int topSpansResult = topSpans * TOPSPANSWEIGHT;
    // multiply by the type weight and the added up weights of the spans
    return sectionWeight * (shortSpansResult + orderedSpansResult + phraseMatchesResult + topSpansResult); 
}

// computes all of the variables that is needed for the rank score function using a variety of ISR'S pertaining to a SPECIFIC document
// ***RME CLAUSE***
// Requires: The rarest word's ISR as the anchor term, and vector of ISR's all other terms w/the anchor term being in the vector as well
//           the int is the index of the phrase term in the search query to check for ordering. 
//           phraseTerms should be ordered like they are in the query, need a vector of vectors incase we have multiplle phraes in the query tree
//           Needs a start location so we can seek to that location
// Modifies: Nothing.
// Effect: Returns the dynamic rank score for a single document.
int get_dynamic_rank(ISRWord* anchorTerm, vector<vector<ISRWord*>> phraseTerms, ISREndDoc* endDoc, uint64_t startLocation, cstring_view type) {
    // declare the variables that we will be passing onto rank_score()
    int shortSpans = 0;
    int orderedSpans = 0;
    int phraseMatches = 0; 
    int topSpans = 0; 
    // go to the end of the document
    endDoc->Seek(startLocation + 1);
    // get the document end location for post location checking
    Post* endPost = endDoc->GetCurrentPost(); 
    uint64_t endLoc = endPost->location;
    // seek all of the ISRs to the start location
    for (int i = 0; i < phraseTerms.size(); i++) {
        for (int j = 0; j < phraseTerms[i].size(); j++) {
            phraseTerms[i][j]->Seek(startLocation);
        }
    }
    // need to calculate the remaining amount of spans(shortSpans, orderedSpans, and topSpans)
    while (anchorTerm->GetCurrentPost()->location < endLoc) {
        // get the current post of the anchored ISR(rarest term)
        auto anchorPost = anchorTerm->GetCurrentPost(); 
        // declare the current spans distance
        int currSpan = 0;
        // declare it to be near the top for each iteration
        bool nearTop = true; 
        // go over each ISR. This might be slow on perf but I don't know how else to do it
        for (int i = 0; i < phraseTerms.size(); i++) {
            for (int j = 0; j < phraseTerms[i].size(); j++) {
                auto currPost = phraseTerms[i][j]->GetCurrentPost();
                // if its not the anchor term and is still in the document do the math
                if (phraseTerms[i][j] != anchorTerm && currPost->location < endLoc) {
                    // span should be a non-negative value
                    currSpan += std::abs((int64_t)anchorPost->location - 
                    (int64_t)currPost->location);
                    // if the current location of the ISR is past what's considered a "near the top" span set it to false
                    if (currPost->location > startLocation + TOPSPANSIZE) {
                        nearTop = false; 
                    }
                    // seek the current ISR to the next occurence if in document
                    if (currPost->location < endLoc) {
                        phraseTerms[i][j]->Next(); 
                    }
                }
                // dont do anything if its the anchor term in the vector
                else {
                    continue; 
                }
            }
        }
        // if the current spans size is deemed to be a short span then add it to the number of short spans
        if (currSpan < SHORTSPANSIZE) {
            shortSpans++; 
        }
        // if all of the ISR locations were near the top then consider it a top span and increment the number of them
        if (nearTop) {
            topSpans++; 
        }
        // check if ordered
        for (int i = 0; i < phraseTerms.size(); i++) {
            if (phraseTerms[i].size() > 1) {
                // declare the boolean to be true and set it to false
                bool ordered = true; 
                bool phrased = true;
                for (int j = 0; j < phraseTerms[i].size() - 1; j++) {
                    // if not ordered(next ISR is before the current one) then break out of the loop and set ordered to false
                    if (phraseTerms[i][j]->GetCurrentPost()->location > phraseTerms[i][j + 1]->GetCurrentPost()->location
                        || phraseTerms[i][j]->GetCurrentPost()->location > endLoc) {
                        ordered = false; 
                        break; 
                    }
                    else if (std::abs((int64_t)phraseTerms[i][j]->GetCurrentPost()->location 
                            - (int64_t)phraseTerms[i][j + 1]->GetCurrentPost()->location) > 1) {
                                phrased = false; 
                    }
                }
                // if it is ordered increment the number of ordered spans
                if (ordered) {
                    orderedSpans++;
                    if (phrased) {
                        phraseMatches++; 
                    } 
                }
            }
        }
        // seek the anchor term's ISR to the next occurence and restart this loop
        anchorTerm->Next(); 
    }
    // return the finalized dynamic ranking
    return get_rank_score(shortSpans, orderedSpans, phraseMatches, topSpans, type); 
}

