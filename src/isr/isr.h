#pragma once

#include <memory>
#include <../utils/cstring_view.h>
#include "Index.h"
#include "IndexFile.h"

typedef size_t Location; // Location 0 is the null location.
typedef size_t FileOffset;

constexpr Location NULL_LOCATION = 0;

class ISR {
public:
    virtual Post* Seek(Location target) = 0;
    virtual Post* NextDocument() = 0;
    virtual Post* Next() = 0;
    virtual Location GetStartLocation() const {
        return currDocStartLocation;
    }
    virtual Location GetEndLocation() const {
        return currDocEndLocation;
    }
    virtual Post* GetCurrentPost() {
        return (currPost.location != NULL_LOCATION) ? &currPost : nullptr;
    }
protected:
    Post currPost;
    Location currDocStartLocation = NULL_LOCATION;
    Location currDocEndLocation = NULL_LOCATION;
};

class ISRWord : public ISR {
public:
    // TO DO: Maybe fix the constructor, stub for query processor
    ISRWord(std::string & word_in) 
    {
        word = std::move(word_in);
    }
    unsigned GetDocumentCount() 
    {
        return; // Get from index file
    } 
    unsigned GetNumberOfOccurrences() 
    {
        return; // Get from index file
    }

private:
    string word;
};

class ISREndDoc : public ISRWord {
public:
    unsigned GetDocumentLength() const { return currDocumentLength; }
    unsigned GetTitleLength() const { return currTitleLength; }
    unsigned GetUrlLength() const { return currUrlLength; }
private:
    unsigned currDocumentLength;
    unsigned currTitleLength;
    unsigned currUrlLength;
};

class ISROr : public ISR {
public:
    std::vector<std::unique_ptr<ISR>> terms;
    int nearestTermIndex = -1; 
    ISROr(std::vector<std::unique_ptr<ISR>> childISRs) : terms(std::move(childISRs)) {}

    Location GetStartLocation() {
        return nearestStartLocation;
    }

    Post* Seek(Location target) override {
        // Seek all the ISRs to the first occurrence beginning at
        // the target location. Return null if there is no match.
        // The document is the document containing the nearest term.
    }

    Post* Next() override {
        // Do a next on the nearest term, then return
        // the new nearest match.

    }
    Post* NextDocument() override {
        // Seek all the ISRs to the first occurrence just past
        // the end of this document.
        Location currEnd = GetEndLocation();
         if (currEnd == NULL_LOCATION) {
             if (!Seek(1)) return nullptr;
             currEnd = GetEndLocation();
             if (currEnd == NULL_LOCATION) return nullptr;
        }
        return Seek(currEnd + 1);

    }

public:

private:
    unsigned nearestTerm;
    // nearStartLocation and nearestEndLocation are the start and end of the nearestTerm.
    Location nearestStartLocation;
};

class ISRAnd : public ISR {
public:
    std::vector<std::unique_ptr<ISR>> terms;
    std::unique_ptr<ISREndDoc> docEndISR;

    Post* FindMatchingDocument(Location target); 

    ISRAnd(std::vector<ISR*> childISRs, ISREndDoc* docEnd) : docEndISR(docEnd) {

        for (ISR *ptr : childISRs)
        {
            terms.emplace_back(ptr);
        }
    }
    
    Post * Seek(Location target) {
        if (terms.empty() || target == NULL_LOCATION) return nullptr;
        Post * lastPost = nullptr;
        farthestTerm = 0;
        unsigned farthestLocation = 0;
        for (size_t i = 0; i < terms.size(); ++i)
        {
            // 1. Seek all the ISRs to the first occurrence beginning at the target location. 
            Post * post = terms[i]->Seek(target);
            // 5. If any ISR reaches the end, there is no match.
            if (!post) return nullptr;
            if (post->location)
            {
                farthestTerm = i;
                farthestLocation = post->location;
            }
        }
        while (true)
        {
            // 2. Move the document end ISR to just past the furthest word, then calculate the document begin location.
            Post * docEnd = docEndISR->Seek(farthestLocation + 1);
            currDoc = docEnd;
            if (!docEnd) return nullptr;
            unsigned docLen = docEndISR->GetDocumentLength();
            unsigned docStart = docEnd->location - docLen;
            // 3. Seek all the other terms to past the document begin.
            for (size_t i = 0; i < terms.size(); ++i)
            {                                       
                Post * post = terms[i]->Seek(docStart + 1);
                // 5. If any ISR reaches the end, there is no match.
                if (!post) return nullptr; 
                 // 4. If any term is past the document end, return to step 2.
                if (post->location > docEnd->location)
                {
                    farthestTerm = i;
                    farthestLocation = post->location;
                }
            }
            if (farthestLocation < docEnd->location)
            {
                break;
            }
            return currDoc;
        }

    }
    Post * Next() override 
    {
        Location startLoc = GetStartLocation();
        return Seek(startLoc + 1);
    }
    Location GetStartLocation()
    {
        return nearestStartLocation;
    }
private:
    unsigned farthestTerm;
    Location nearestStartLocation;
};

class ISRPhrase : public ISR {
public:
    std::vector<std::unique_ptr<ISR>> terms;
    // TO DO: Maybe fix the constructor, stub for query processor
    ISRPhrase(std::vector<ISR*> ISRterms) {
        for (ISR *ptr : ISRterms) {
            terms.emplace_back(ptr);
        }
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