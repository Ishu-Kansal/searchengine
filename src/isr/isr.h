#pragma once

#include <memory>
#include <../utils/cstring_view.h>
#include "IndexFileReader.h"

typedef size_t Location; // Location 0 is the null location.
typedef size_t FileOffset;

constexpr Location NULL_LOCATION = 0;
static IndexFileReader READER(1);

class ISR {
public:
    virtual SeekObj* Seek(Location target) = 0;
    virtual SeekObj* NextDocument() = 0;
    virtual SeekObj* Next() = 0;
    virtual Location getDocStartLoc() {
        return currDocStartLocation;
    }
    virtual Location getDocEndLoc() {
        return currDocEndLocation;
    }
    virtual SeekObj * GetCurrentPost()
    {
        return currPost;
    }

protected:
    SeekObj * currPost = nullptr;
    Location currDocStartLocation = NULL_LOCATION;
    Location currDocEndLocation = NULL_LOCATION;
};

class ISRWord : public ISR {
public:
    // TO DO: Maybe fix the constructor, stub for query processor
    ISRWord(std::string & word_in) {
        word = std::move(word_in);
    }
    
    SeekObj* Seek(Location target) {
        return READER.Find(word, target, 1).get();
    }
    
    unsigned GetDocumentCount() {
        return; // Get from index file
    } 

    unsigned GetNumberOfOccurrences() {
        return GetCurrentPost()->numOccurrences;
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
    std::unique_ptr<ISREndDoc> docEndISR;

    ISROr(std::vector<std::unique_ptr<ISR>> childISRs) : terms(std::move(childISRs)) {
        docEndISR = std::make_unique<ISREndDoc>();
    }

    SeekObj* Seek(Location target) override {
        if (terms.empty() || target == NULL_LOCATION) return nullptr;
        // Seek all the ISRs to the first occurrence beginning at
        // the target location. Return null if there is no match.
        // The document is the document containing the nearest term.
        for (size_t i = 0; i < terms.size(); ++i)
        {
            SeekObj * seekObj = terms[i]->Seek(target);
            if (!seekObj) return nullptr;
            if (seekObj->location < terms[nearestTerm]->GetCurrentPost()->location)
            {
                nearestTerm = i;
                SeekObj * docEnd = docEndISR->Seek(seekObj->location);
                if (!docEnd) return nullptr;
                nearestStartLocation = docEnd->location - docEndISR->GetDocumentLength();
                nearestEndLocation = docEnd->location;
            }
        }
        return docEndISR->Seek(nearestEndLocation);
    }

    SeekObj* Next() override {
        // Do a next on the nearest term, then return
        // the new nearest match.
        SeekObj * post = terms[nearestTerm]->GetCurrentPost();
        return Seek(post->location + 1);
    }
    SeekObj* NextDocument() override {
        // Seek all the ISRs to the first occurrence just past
        // the end of this document.
        Location currEnd = getDocEndLoc();
        return Seek(currEnd + 1);
    }

public:
private:
    unsigned nearestTerm;
    // nearStartLocation and nearestEndLocation are the start and end of the nearestTerm.
    Location nearestStartLocation, nearestEndLocation;
};

class ISRAnd : public ISR {
public:
    std::vector<std::unique_ptr<ISR>> terms;
    std::unique_ptr<ISREndDoc> docEndISR;
    bool anotherOne = false;
    ISRAnd(std::vector<std::unique_ptr<ISR>> childISRs) : terms(std::move(childISRs)) {
        docEndISR = std::make_unique<ISREndDoc>();
    }
    
    SeekObj * Seek(Location target) {
        if (terms.empty() || target == NULL_LOCATION) return nullptr;
        farthestTerm = 0;
        for (size_t i = 0; i < terms.size(); ++i)
        {
            // 1. Seek all the ISRs to the first occurrence beginning at the target location.
            SeekObj * seekObj = terms[i]->Seek(target);
            // 5. If any ISR reaches the end, there is no match.
            if (!seekObj) return nullptr;
            if (seekObj->location > terms[farthestTerm]->GetCurrentPost()->location)
            {
                farthestTerm = i;
            }
            if (seekObj->location < terms[nearestTerm]->GetCurrentPost()->location)
            {
                nearestTerm = i;
            }
        }
        while (true)
        {
            // 2. Move the document end ISR to just past the furthest word, then calculate the document begin location.
            anotherOne = false;
            SeekObj * docEnd = docEndISR->Seek(terms[farthestTerm]->GetCurrentPost()->location + 1);
            if (!docEnd) return nullptr;
            nearestStartLocation = docEnd->location - docEndISR->GetDocumentLength();
            nearestEndLocation = docEnd->location;
            // 3. Seek all the other terms to past the document begin.
            for (size_t i = 0; i < terms.size(); ++i)
            {                                       
                SeekObj * seekObj = terms[i]->Seek(nearestStartLocation+ 1);
                // 5. If any ISR reaches the end, there is no match.
                if (!seekObj) return nullptr; 
                 // 4. If any term is past the document end, return to step 2.
                if (seekObj->location > nearestEndLocation)
                {
                    farthestTerm = i;
                    anotherOne = true;
                }
                if (seekObj->location < terms[nearestTerm]->GetCurrentPost()->location)
                {
                    nearestTerm = i;
                }
            }
            if (!anotherOne) break;
        }
        return docEndISR->Seek(nearestStartLocation);
    }
    SeekObj * Next() override 
    {
        return Seek(terms[nearestTerm]->GetCurrentPost()->location + 1);
    }
    SeekObj * NextDocument() 
    {
        return Seek(nearestEndLocation + 1);
    }
private:
    unsigned nearestTerm, farthestTerm;
    Location nearestStartLocation, nearestEndLocation;
};

class ISRPhrase : public ISR {
public:
    std::vector<std::unique_ptr<ISR>> terms;
    std::unique_ptr<ISREndDoc> docEndISR;
    bool anotherOne = false; // Need to know if we need return to step 2

    ISRPhrase(std::vector<std::unique_ptr<ISR>> childISRs) : terms(std::move(childISRs)) {
        docEndISR = std::make_unique<ISREndDoc>();
    }
    SeekObj * Seek(Location target) {
        if (terms.empty()) return nullptr;
        // 4. If any ISR reaches the end, there is no match.

        // 1. Seek all ISRs to the first occurrence beginning at the target location.
        for (size_t i = 0; i < terms.size(); ++i)
        {
            SeekObj * seekObj = terms[i]->Seek(target);
            if (!seekObj) return nullptr; 
            if (seekObj->location > farthestTermLocation)
            {
                farthestTerm = i;
                farthestTermLocation = seekObj->location;
            }
        }
        // 2. Pick the furthest term and attempt to seek all the other terms to the first location beginning where they should appear relative to the furthest term.
        while (true)
        {
            anotherOne = false;
            for (size_t i = 0; i < terms.size(); ++i)
            {
                if (i != farthestTerm)
                {
                    // Finds what relative order it should be at
                    Location relativePos = farthestTermLocation + i - farthestTerm;
                    SeekObj * tempObj = terms[i]->Seek(relativePos);
                    if (!tempObj) return nullptr;
                    if (tempObj->location > farthestTermLocation) 
                    {
                        // Out of relative order, have to go back to step 2
                        farthestTerm = i;
                        farthestTermLocation = tempObj->location;
                        anotherOne = true;
                    }
                }
            }
            if (!anotherOne) break;
            // 3. If any term is past the desired location, return to step 2.
        }
        SeekObj * docEnd = docEndISR->Seek(farthestTermLocation);
        if (!docEnd) return nullptr;
        nearestStartLocation = docEnd->location - docEndISR->GetDocumentLength();
        nearestEndLocation = docEnd->location;
        return docEnd;
    }   
    SeekObj * Next() {
        // Finds overlapping phrase matches.
        return Seek(terms[0]->GetCurrentPost()->location + 1);
    }

private:
    unsigned farthestTerm;
    Location farthestTermLocation;
    Location nearestStartLocation, nearestEndLocation;
};

class ISRContainer : public ISR {
public:
    std::vector<std::unique_ptr<ISR>> terms;
    std::vector<std::unique_ptr<ISR>> excluded;
    std::unique_ptr<ISREndDoc> docEndISR;
    bool anotherOne = false;
    bool excludedNotFound = true;
    ISRContainer(std::vector<std::unique_ptr<ISR>> childISRs, std::vector<std::unique_ptr<ISR>>  excluded) : terms(std::move(childISRs)), excluded(std::move(excluded)) {
        docEndISR = std::make_unique<ISREndDoc>();
    }
    unsigned CountContained, CountExcluded;

    SeekObj * Seek(Location target) {
        //1. Seek all the included ISRs to the first occurrence beginning at the target location.
        if (terms.empty()) return nullptr;
        farthestTerm = 0;
        while(true)
        {
            excludedNotFound = true;
            for (size_t i = 0; i < terms.size(); ++i)
            {
                SeekObj * seekObj = terms[i]->Seek(target);
                if (!seekObj) return nullptr;
                if (seekObj->location > terms[farthestTerm]->GetCurrentPost()->location)
                {
                    farthestTerm = i;
                }
                if (seekObj->location < terms[nearestTerm]->GetCurrentPost()->location)
                {
                    nearestTerm = i;
                }
            }
            // 2. Move the document end ISR to just past the furthest contained ISR, then calculate the document begin location.
            SeekObj * docEnd = docEndISR->Seek(terms[farthestTerm]->GetCurrentPost()->location + 1);
            if (!docEnd) return nullptr;
            nearestStartLocation = docEnd->location - docEndISR->GetDocumentLength();
            nearestEndLocation = docEnd->location;
    
             // 3. Seek all the other contained terms to past the document begin.
             // 4. If any contained term is past the document end, return to step 2.
             // 5. If any ISR reaches the end, there is no match.
             // 6. Seek all the excluded ISRs to the first occurrence beginning at the document begin location.
             // 7. If any excluded ISR falls within the document, reset the target to one past the end of the document and return to step 1.
            while (true)
            {
                // 3. Seek all the other terms to past the document begin.
                anotherOne = false;
                for (size_t i = 0; i < terms.size(); ++i)
                {                                       
                    SeekObj * seekObj = terms[i]->Seek(nearestStartLocation+ 1);
                    // 5. If any ISR reaches the end, there is no match.
                    if (!seekObj) return nullptr; 
                     // 4. If any term is past the document end, return to step 2.
                    if (seekObj->location > nearestEndLocation)
                    {
                        farthestTerm = i;
                        anotherOne = true;
                    }
                    if (seekObj->location < terms[nearestTerm]->GetCurrentPost()->location)
                    {
                        nearestTerm = i;
                    }
                }
                if (!anotherOne) break;
            }
            for (size_t i = 0; i < excluded.size(); ++i)
            {
                SeekObj * excludedObj = excluded[i]->Seek(nearestStartLocation + 1);
                if (!excludedObj) continue;
                if (excludedObj->location < nearestEndLocation) 
                {
                    target = nearestEndLocation + 1;
                    excludedNotFound = false;
                    break;
                }
            }
            if (excludedNotFound) break;
        }
        return docEndISR->Seek(nearestStartLocation);
    }
    SeekObj * Next() override 
    {
        return Seek(terms[nearestTerm]->GetCurrentPost()->location + 1);
    }
    SeekObj * NextDocument() 
    {
        return Seek(nearestEndLocation + 1);
    }

 private:
     unsigned nearestTerm, farthestTerm;
     Location nearestStartLocation, nearestEndLocation;
};