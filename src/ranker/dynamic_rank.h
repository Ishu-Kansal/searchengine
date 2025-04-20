#pragma once

#include <stdint.h>

#include <algorithm>
#include <vector>
#include <climits>
#include <string>
#include <unordered_map>
#include "../isr/isr.h"
#include "../inverted_index/IndexFileReader.h"

// TO DO: WEIGHTS ARE ALL 0 RIGHT NOW, WILL NEED TO SET THEM TO REAL VALUES SOON AND DEBUG THEM TO FIND WHICH WEIGHTS ARE THE BEST FOR 
//       OUR RESULTS FOR THE ENGINE

// Weights for computing the score of specific ranking components, enums for quick readjusting and no magic numbers
enum DynamicWeights: int {
    SHORTSPANSWEIGHT = 7, 
    ORDEREDSPANSWEIGHT = 10,
    PHRASEMATCHESWEIGHT = 20,
    TOPSPANSWEIGHT = 7
}; 

// Weights for the section the dynamic ranking heuristic is being ranked on, e.g if its being ran on the URL of a document or the body
// enums for quick readjusting and no magic numbers once again 
enum SectionWeights: int {
    TITLEWEIGHT = 4,
    BODYWEIGHT = 1
};

// enum class for requirements of what is considered a short span or a top span
enum Requirements: int {
    SHORTSPANSIZE = 10,
    TOPSPANSIZE = 100
};

using locationVector = std::vector<Location>;

constexpr Location RANGE_TOLERANCE = 25;
// given all of the occurences assigns the weights and returns the actual dynamic rank
// ***RME CLAUSE***
// Requires: The number of short spans, ordered spans, phrase matches, top spans, and the type of section this is being scored on
// Modifies: Nothing
// Effect: Scores a given rank of the document using weights with the given numbers above.
int get_rank_score(int shortSpans, int orderedSpans, int phraseMatches, int topSpans, bool isBody) {
    // TODO: make a hashtable/array for the type weights
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
int get_dynamic_rank(std::unique_ptr<ISRWord> &anchorTerm, vector<vector<std::unique_ptr<ISRWord>>> &phraseTerms, uint64_t startLocation, uint64_t endLocation, const IndexFileReader & reader, uint32_t currChunk, bool isBody) {

    // Gets all locations for anchor in this document
    locationVector anchorLocations = reader.LoadChunkOfPostingList(
        anchorTerm->GetWord(), 
        currChunk,   
        startLocation,
        endLocation,
        anchorTerm->GetSeekTableIndex()
    );

    Location newStart = startLocation;
    Location newEnd = endLocation;

    if (!anchorLocations.empty())
    {
        Location anchorStart = anchorLocations.front();
        Location anchorEnd = anchorLocations.back();
    
        newStart = max(anchorStart + RANGE_TOLERANCE, Location(newStart));
        newEnd = min(anchorEnd + RANGE_TOLERANCE, Location(newEnd));
    }
    // 3D vector, 1D is each phrase, 2D is the words in that phrase i, and 3D are all the locations for word j
    std::vector<std::vector<locationVector>> loadedPhrasePostings(phraseTerms.size());

    bool possibleOverall = true;

    // Go through each phrase
    for (int i = 0; i < phraseTerms.size(); ++i) {
        // Resize it to the number of words in that phrase
        loadedPhrasePostings[i].resize(phraseTerms[i].size());
        for (int j = 0; j < phraseTerms[i].size(); ++j) 
        {
            if (phraseTerms[i][j]->GetWord() == anchorTerm->GetWord()) 
            {
                loadedPhrasePostings[i][j] = anchorLocations;
            } 
            else 
            {
                // Load chunk for term (i, j)
                loadedPhrasePostings[i][j] = reader.LoadChunkOfPostingList(
                    phraseTerms[i][j]->GetWord(),
                    currChunk,                   
                    newStart,
                    newEnd,
                    phraseTerms[i][j]->GetSeekTableIndex()
                );
            }
        }
    }

    int shortSpans = 0;
    int orderedSpans = 0;
    int phraseMatches = 0;
    int topSpans = 0; 

    // Indices to keep track of the current position in each posting list
    std::vector<std::vector<size_t>> currentIndices(phraseTerms.size());
    for (int i = 0; i < phraseTerms.size(); ++i) {
        currentIndices[i].resize(phraseTerms[i].size(), 0);
    }

    for (const Location currentAnchorLoc : anchorLocations) 
    {

        int64_t currentTotalSpan = 0;
        bool allTermsNearTop = true; 

        std::vector<std::vector<Location>> synchronizedLocations(phraseTerms.size());

        for (int i = 0; i < phraseTerms.size(); ++i) 
        {

            synchronizedLocations[i].resize(phraseTerms[i].size(), static_cast<Location>(-1));

            for (int j = 0; j < phraseTerms[i].size(); ++j) 
            {

                if (loadedPhrasePostings[i][j].empty()) 
                {
                     synchronizedLocations[i][j] = static_cast<Location>(-1);
                     continue; 
                }

                size_t& currentIdx = currentIndices[i][j];

                // advance we find a location >= currentAnchorLoc
                while (currentIdx < loadedPhrasePostings[i][j].size() &&
                       loadedPhrasePostings[i][j][currentIdx] < currentAnchorLoc) 
                {
                    currentIdx++;
                }

                if (currentIdx >= loadedPhrasePostings[i][j].size())
                {
                    currentIdx = loadedPhrasePostings[i][j].size() - 1;
                }

                if (loadedPhrasePostings[i][j][currentIdx] >= endLocation) {
                    synchronizedLocations[i][j] = static_cast<Location>(-1);
                    continue; // Continue to next term j
                }
                if (currentIdx != 0) 
                {
                    // Will always be less than currentAnchorLoc
                    Location offsetBeforeAnchor = currentAnchorLoc - loadedPhrasePostings[i][j][currentIdx - 1];

                    // Will always be greater or equal to currentAnchorLoc
                    Location offsetAfterAnchor = loadedPhrasePostings[i][j][currentIdx] - currentAnchorLoc;

                    if (offsetBeforeAnchor < offsetAfterAnchor)
                    {
                        currentIdx -= 1;
                    }
                }
                Location termLoc = loadedPhrasePostings[i][j][currentIdx];
                synchronizedLocations[i][j] = termLoc;
                
                bool isAnchorTerm = (phraseTerms[i][j]->GetWord() == anchorTerm->GetWord());

                // Add to total span (if not the anchor term itself)
                if (!isAnchorTerm) 
                {
                     currentTotalSpan += std::abs(static_cast<int64_t>(currentAnchorLoc) - static_cast<int64_t>(termLoc));
                }

                // Check if this term's location is near the top
                if (termLoc >= startLocation + Requirements::TOPSPANSIZE) 
                {
                    allTermsNearTop = false;
                }

            } // End inner loop (j) for terms within a phrase
        } // End outer loop (i) for phrases

        // Check for short span (using the total span calculated)
        if (currentTotalSpan < Requirements::SHORTSPANSIZE) 
        {
            shortSpans++;
        }

        // Check if this anchor position initiated a "top span"
        // This requires *all* found terms to be near the top.
        if (allTermsNearTop) 
        {
            topSpans++;
        }

        // Check order and phrase matches using the synchronizedLocations
        for (int i = 0; i < phraseTerms.size(); ++i) 
        {
            if (phraseTerms[i].size() > 1) 
            {
                bool phraseInOrder = true;
                bool exactPhraseMatch = true;

                for (int j = 0; j < phraseTerms[i].size() - 1; ++j) 
                {
                    Location loc1 = synchronizedLocations[i][j];
                    Location loc2 = synchronizedLocations[i][j + 1];

                    // Check if both locations were successfully found
                    if (loc1 == static_cast<Location>(-1) || loc2 == static_cast<Location>(-1)) 
                    {
                        phraseInOrder = false;
                        exactPhraseMatch = false;
                        break;
                    }

                    // Check order
                    if (loc1 >= loc2) 
                    {
                        phraseInOrder = false;
                        exactPhraseMatch = false;
                        break;
                    }

                    // Check exact phrase match distance
                    if (static_cast<int64_t>(loc2) - static_cast<int64_t>(loc1) != 1) 
                    {
                        exactPhraseMatch = false;
                    }
                } // End loop checking pairs (j, j+1)

                if (phraseInOrder) 
                {
                    orderedSpans++;
                    if (exactPhraseMatch) 
                    {
                        phraseMatches++;
                    }
                }
            } // End if phrase size > 1
        } // End loop over phrases (i) for order/phrase check

    } // End loop over anchorLocations

    return get_rank_score(shortSpans, orderedSpans, phraseMatches, topSpans, isBody);
}