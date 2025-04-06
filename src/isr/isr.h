#pragma once

#include <../utils/cstring_view.h>
#include "Index.h"

typedef size_t Location; // Location 0 is the null location.
typedef size_t FileOffset;

class ISR {
public:
    virtual Post* Next();
    virtual Post* NextDocument();
    virtual Post* Seek(Location target);
    virtual Location GetStartLocation();
    virtual Location GetEndLocation();
};

class ISRWord : public ISR {
public:
    // TO DO: Maybe fix the constructor, stub for query processor
    ISRWord(std::string word_in) {
        word = word_in;
    }
    unsigned GetDocumentCount();
    unsigned GetNumberOfOccurrences();
    virtual Post* GetCurrentPost();

private:
    string word;
};

class ISREndDoc : public ISRWord {
public:
    unsigned GetDocumentLength();
    unsigned GetTitleLength();
    unsigned GetUrlLength();
};

class ISROr : public ISR {
public:
    
    std::vector<ISR> terms;
    // TO DO: Maybe fix the constructor, stub for query processor
    ISROr(std::vector<ISR> ISRterms) {
        terms = ISRterms;
    }
    Location GetStartLocation() {
        return nearestStartLocation;
    }
    Location GetEndLocation() {
        return nearestEndLocation;
    }
    Post* Seek(Location target) {
        // Seek all the ISRs to the first occurrence beginning at
        // the target location. Return null if there is no match.
        // The document is the document containing the nearest term.
        nearestStartLocation = std::numeric_limits<Location>::max();
        nearestEndLocation = std::numeric_limits<Location>::max();
        nearestTerm = -1;

        Post* nearestPost = nullptr;

        for (unsigned i = 0; i < terms.size(); i++) {
            Post* currPost = terms[i].Seek(target);
            if (currPost != nullptr) {
                Location currStart = terms[i].GetStartLocation();
                if (currStart < nearestStartLocation) {
                    nearestStartLocation = currStart; // Update the nearest start location
                    nearestEndLocation = terms[i].GetEndLocation();
                    nearestTerm = i; // Update the nearest term index
                    nearestPost = currPost;
                }
            }
        }
        return nearestPost;
    }
    Post* Next() {
        // Do a next on the nearest term, then return
        // the new nearest match.

        if (nearestTerm == -1) {
            return nullptr;
        }

        terms[nearestTerm].Next(); // Move the nearest term to the next post

        nearestStartLocation = std::numeric_limits<Location>::max();
        nearestEndLocation = std::numeric_limits<Location>::max();
        nearestTerm = -1;

        Post* nearestPost = nullptr;

        for (unsigned i = 0; i < terms.size(); i++) {
            Post* currPost = terms[i].getCurrentPost(); // Get the current post for each term
            if (currPost != nullptr) {
                Location currStart = terms[i].GetStartLocation();
                if (currStart < nearestStartLocation) {
                    nearestStartLocation = currStart; // Update the nearest start location
                    nearestEndLocation = terms[i].GetEndLocation();
                    nearestTerm = i; // Update the nearest term index
                    nearestPost = currPost;
                }
            }
        }

        return nearestPost;

    }
    Post* NextDocument() {
        // Seek all the ISRs to the first occurrence just past
        // the end of this document.
        return Seek(DocumentEnd->GetEndLocation() + 1);
    }

private:
    unsigned nearestTerm;
    // nearStartLocation and nearestEndLocation are the start and end of the nearestTerm.
    Location nearestStartLocation, nearestEndLocation;
};

class ISRAnd : public ISR {
public:
    std::vector<ISR> terms;
    ISREndDoc endDocISR;

    // TO DO: Maybe fix the constructor, stub for query processor
    ISRAnd(std::vector<ISR> ISRterms) {
        terms = ISRterms;
    } 
    Post* Seek(Location target) {
        // 5. If any ISR reaches the end, there is no match.
        for (size_t i = 0; i < terms.size(); ++i)
        {
            // 1. Seek all the ISRs to the first occurrence beginning at
            // the target location.
            Post * post = terms[i].Seek(target);
            if (i == 0)
            {
                nearestTerm = post->location;
                farthestTerm = post->location;
            }
            else
            {
                if (post->location > farthestTerm) farthestTerm = post->location;
                if (post->location < farthestTerm) farthestTerm = post->location;    
            }
        }
        bool restart = false;
        while(true)
        {
            // 2. Move the document end ISR to just past the furthest word, then calculate the document begin location.
            restart = false;
            Post * endDoc = endDocISR.Seek(farthestTerm);
            size_t docLen = endDocISR.GetDocumentLength();
            size_t docStart =  endDoc->location - docLen;
            // 3. Seek all the other terms to past the document begin.
            for (size_t i = 0; i < terms.size(); ++i)
            {
                Post * post = terms[i].Seek(docStart);
                if (post->location > endDoc->location)
                {
                    restart = true;
                    break;
                }
            }
            // 4. If any term is past the document end, return to step 2.
            if (restart) continue;
            break;
        }
        // TODO: 5. If any ISR reaches the end, there is no match.

    }
    Post* Next() {
        return Seek(nearestStartLocation + 1);
    }

private:
    unsigned nearestTerm, farthestTerm;
    Location nearestStartLocation, nearestEndLocation;
};

class ISRPhrase : public ISR {
public:
    std::vector<ISR> terms;
    // TO DO: Maybe fix the constructor, stub for query processor
    ISRPhrase(std::vector<ISR> ISRterms) {
        terms = ISRterms;
    }
    Post* Seek(Location target) {
        // 1. Seek all ISRs to the first occurrence beginning at the target location.
        // 2. Pick the furthest term and attempt to seek all the other terms to the first location beginning
        //    where they should appear relative to the furthest term.
        // 3. If any term is past the desired location, return to step 2.
        // 4. If any ISR reaches the end, there is no match.
        
    }
    Post* Next() {
        // Finds overlapping phrase matches.
        return Seek(nearestStartLocation + 1);
    }

private:
    Location nearestStartLocation, nearestEndLocation;
};

class ISRContainer : public ISR {
public:
    ISR** Contained;
    ISR* Excluded;
    ISREndDoc* EndDoc;
    unsigned CountContained, CountExcluded;

    Location Next();
    Post* Seek(Location target) {
        // 1. Seek all the included ISRs to the first occurrence beginning at the target location.
        // 2. Move the document end ISR to just past the furthest contained ISR, then calculate the document begin location.
        // 3. Seek all the other contained terms to past the document begin.
        // 4. If any contained term is past the document end, return to step 2.
        // 5. If any ISR reaches the end, there is no match.
        // 6. Seek all the excluded ISRs to the first occurrence beginning at the document begin location.
        // 7. If any excluded ISR falls within the document, reset the target to one past the end of the document and return to step 1.
    }
    Post* Next() {
        Seek(Contained[nearestContained]->GetStartlocation() + 1);
    }

private:
    unsigned nearestTerm, farthestTerm;
    Location nearestStartLocation, nearestEndLocation;
};