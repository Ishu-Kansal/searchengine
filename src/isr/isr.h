#pragma once

#include <memory>
#include <../utils/cstring_view.h>
#include "Index.h"
#include "IndexFile.h"

typedef size_t Location; // Location 0 is the null location.
typedef size_t FileOffset;

constexpr Location NULL_LOCATION = 0;
struct SeekObj {
    Location offset;
    Location location;
};
class ISR {
public:
    virtual SeekObj* Seek(Location target) = 0;
    virtual SeekObj* NextDocument() = 0;
    virtual SeekObj* Next() = 0;
    virtual Location GetStartLocation() {
        return currDocStartLocation;
    }
    virtual Location GetEndLocation() {
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
        if (!indexData || word.empty())
            return nullptr;

        // Add '!' if needed to match title-word format
        std::string lookup = word;
        if (lookup.back() != TITLE_MARKER)
        {
            lookup.push_back(TITLE_MARKER);
        }

        // Get byte offset from dictionary
        auto *entry = dictionary->Find(lookup);
        if (!entry)
            return nullptr;

        size_t byteOffset = entry->value;
        uint8_t *ptr = indexData + byteOffset;

        // Get number of postings
        uint64_t numPosts = 0;
        ptr = decodeVarint(ptr, &numPosts);

        uint64_t pos = 0;
        for (uint64_t i = 0; i < numPosts; ++i)
        {
            uint64_t delta;
            ptr = decodeVarint(ptr, &delta);
            pos += delta;

            if (pos >= target)
            {
                // Found match
                static Post match;
                match.location = pos;

                // Update internal state
                currPost = reinterpret_cast<SeekObj *>(&match); // cast if you want SeekObj behavior
                currDocStartLocation = pos;
                currDocEndLocation = pos;

                return &match;
            }
        }

        return nullptr;
    }
    
    unsigned GetDocumentCount() {
        return; // Get from index file
    } 

    unsigned GetNumberOfOccurrences() {
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
        Location currEnd = GetEndLocation();
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
    bool end = true;
    ISRAnd(std::vector<std::unique_ptr<ISR>> childISRs) : terms(std::move(childISRs))
    {}
    
    SeekObj * Seek(Location target) {
        if (terms.empty() || target == NULL_LOCATION) return nullptr;
        farthestTerm = 0;
        for (size_t i = 0; i < terms.size(); ++i)
        {
            // 1. Seek all the ISRs to the first occurrence beginning at the target location.
            SeekObj * post = terms[i]->Seek(target);
            // 5. If any ISR reaches the end, there is no match.
            if (!post) return nullptr;
            if (post->location > terms[farthestTerm]->GetCurrentPost()->location)
            {
                farthestTerm = i;
            }
        }
        while (true)
        {
            // 2. Move the document end ISR to just past the furthest word, then calculate the document begin location.
            SeekObj * docEnd = docEndISR->Seek(terms[farthestTerm]->GetCurrentPost()->location + 1);
            if (!docEnd) return nullptr;
            nearestStartLocation = docEnd->location - docEndISR->GetDocumentLength();
            nearestEndLocation = docEnd->location;
            // 3. Seek all the other terms to past the document begin.
            for (size_t i = 0; i < terms.size(); ++i)
            {                                       
                SeekObj * post = terms[i]->Seek(nearestStartLocation+ 1);
                // 5. If any ISR reaches the end, there is no match.
                if (!post) return nullptr; 
                 // 4. If any term is past the document end, return to step 2.
                if (post->location > nearestEndLocation)
                {
                    farthestTerm = i;
                    end = false;
                }
            }
            if (end) break;
        }
        // Need to return something
    }
    SeekObj * Next() override 
    {
        // redo
    }
private:
    unsigned farthestTerm;
    Location nearestStartLocation, nearestEndLocation;
};

class ISRPhrase : public ISR {
public:
    std::vector<std::unique_ptr<ISR>> terms;
    std::unique_ptr<ISREndDoc> docEndISR;
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

// class ISRContainer : public ISR {
// public:
//     ISR** Contained;
//     ISR* Excluded;
//     ISREndDoc* EndDoc;
//     unsigned CountContained, CountExcluded;

//     Location Next();
//     Post* Seek(Location target) {
//         // 1. Seek all the included ISRs to the first occurrence beginning at the target location.
//         // 2. Move the document end ISR to just past the furthest contained ISR, then calculate the document begin location.
//         // 3. Seek all the other contained terms to past the document begin.
//         // 4. If any contained term is past the document end, return to step 2.
//         // 5. If any ISR reaches the end, there is no match.
//         // 6. Seek all the excluded ISRs to the first occurrence beginning at the document begin location.
//         // 7. If any excluded ISR falls within the document, reset the target to one past the end of the document and return to step 1.
//     }
//     Post* Next() {
//         Seek(Contained[nearestContained]->GetStartlocation() + 1);
//     }

// private:
//     unsigned nearestTerm, farthestTerm;
//     Location nearestStartLocation, nearestEndLocation;
// };