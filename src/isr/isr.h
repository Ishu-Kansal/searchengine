#pragma once

#include <memory>
#include "../inverted_index/IndexFileReader.h"

typedef size_t Location; // Location 0 is the null location.
typedef size_t FileOffset;

constexpr Location NULL_LOCATION = 0;
static std::string endDoc = "!#$%!#13513sfas";
class ISR {
public:

    virtual SeekObj* Seek(Location target) = 0;
    virtual SeekObj* NextDocument() = 0;
   
    virtual Location getStartLocation() {}
    virtual Location getEndLocation() {}
    
    virtual SeekObj * GetCurrentPost() const
    {
        return currPost.get();
    }

    virtual SeekObj* Next() {}
    ISR() = default;
    virtual ~ISR() = default;

protected:
    unique_ptr<SeekObj> currPost = nullptr;
    Location nearestStartLocation = NULL_LOCATION;
    Location nearestEndLocation = NULL_LOCATION;
};

class ISRWord : public ISR {
public:
    ISRWord() = delete;
    ISRWord(std::string word_in, const IndexFileReader& reader)
        : reader_(reader), word(std::move(word_in)) {}
    
    SeekObj* Seek(Location target) {
        currPost = reader_.Find(word, target, 0);
        return currPost.get();
    }
    unsigned GetNumberOfOccurrences() {
        auto post = GetCurrentPost();
        if (!post) return 0;
        return post->numOccurrences;
    }
    SeekObj* NextDocument() {
        return Seek(getEndLocation() + 1);
    }
    SeekObj* Next() {
        auto post = GetCurrentPost();
        if (!post) return Seek(0);
        return Seek(post->location + 1);
    }
    /*
        unsigned GetDocumentCount() {
        return;
        } 
    */
protected:
    const IndexFileReader& reader_;
    std::string word;
};

class ISREndDoc : public ISRWord {
public:
    // unsigned GetDocumentLength() const { return currDocumentLength; }
    // unsigned GetTitleLength() const { return currTitleLength; }
    // unsigned GetUrlLength() const { return currUrlLength; }
    unsigned GetDocumentLength() const 
    {
        auto currDoc = GetCurrentPost();
        if (!currDoc) return 0;
        return currDoc->delta; 
    }
    ISREndDoc(const IndexFileReader& reader)
    : ISRWord(endDoc, reader) {}
private:
    // unsigned currDocumentLength;
    // unsigned currTitleLength;
    // unsigned currUrlLength;
};  

class ISROr : public ISR {
public:
    std::vector<std::unique_ptr<ISR>> terms;
    std::unique_ptr<ISREndDoc> docEndISR;

    ISROr(std::vector<std::unique_ptr<ISR>> childISRs, const IndexFileReader& reader)
    : terms(std::move(childISRs)), nearestTerm(0), nearestStartLocation(0), nearestEndLocation(0) {
    docEndISR = std::make_unique<ISREndDoc>(reader);
    }

    Location getStartLocation() override
    {
        return nearestStartLocation;
    }
    Location getEndLocation() override
    {
        return nearestEndLocation;
    }

    SeekObj* Seek(Location target) override {
        if (terms.empty()) return nullptr;
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
        return terms[nearestTerm]->GetCurrentPost();
    }

    SeekObj* Next() override {
        // Do a next on the nearest term, then return
        // the new nearest match.
        SeekObj * seekObj = terms[nearestTerm]->GetCurrentPost();
        if (!seekObj) return Seek(0);
        return Seek(seekObj->location + 1);
    }
    SeekObj* NextDocument() override {
        // Seek all the ISRs to the first occurrence just past
        // the end of this document.
        Location currEnd = getEndLocation();
        return Seek(currEnd + 1);
    }

private:
    unsigned nearestTerm;
    // nearStartLocation and nearestEndLocation are the start and end of the nearestTerm.
    Location nearestStartLocation, nearestEndLocation;
};

class ISRAnd : public ISR {
public:
    std::vector<std::unique_ptr<ISR>> terms;
    std::unique_ptr<ISREndDoc> docEndISR;
    ISRAnd(std::vector<std::unique_ptr<ISR>> childISRs, const IndexFileReader& reader)
        : terms(std::move(childISRs)), nearestTerm(0), farthestTerm(0), nearestStartLocation(0), nearestEndLocation(0) {
        docEndISR = std::make_unique<ISREndDoc>(reader);
    }
    Location getStartLocation() override
    {
        return nearestStartLocation;
    }
    Location getEndLocation() override
    {
        return nearestEndLocation;
    }
    SeekObj * Seek(Location target) {
        if (terms.empty()) return nullptr;
        bool anotherOne = false;
        for (size_t i = 0; i < terms.size(); ++i)
        {
            // 1. Seek all the ISRs to the first occurrence beginning at the target location.
            SeekObj * seekObj = terms[i]->Seek(target);
            // 5. If any ISR reaches the end, there is no match.
            if (!seekObj) return nullptr;
            SeekObj * farthestPost = nullptr;
            SeekObj * nearestPost = nullptr;
            if (i != 0)
            {
                farthestPost = terms[farthestTerm]->GetCurrentPost();
                nearestPost = terms[nearestTerm]->GetCurrentPost();
            }
            
            if (!farthestPost || seekObj->location > farthestPost->location)
            {
                farthestTerm = i;
            }
            if (!nearestPost || seekObj->location < nearestPost->location)
            {
                nearestTerm = i;
            }
        }
        while (true)
        {
            // 2. Move the document end ISR to just past the furthest word, then calculate the document begin location.
            anotherOne = false;
            SeekObj * farthestPost = terms[farthestTerm]->GetCurrentPost();
            SeekObj * docEnd = docEndISR->Seek(farthestPost->location);
            if (!docEnd) return nullptr;
            unsigned len = docEndISR->GetDocumentLength();
            nearestStartLocation = docEnd->location - docEndISR->GetDocumentLength();
            nearestEndLocation = docEnd->location;
            // 3. Seek all the other terms to past the document begin.
            for (size_t i = 0; i < terms.size(); ++i)
            {                                       
                SeekObj * seekObj = terms[i]->Seek(nearestStartLocation);
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

        return terms[nearestTerm]->GetCurrentPost();
    
    }
    SeekObj * Next() override 
    {
        SeekObj * seekObj = terms[nearestTerm]->GetCurrentPost();
        if (!seekObj) return Seek(0);
        return Seek(seekObj->location + 1);
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

    ISRPhrase(std::vector<std::unique_ptr<ISR>> childISRs, const IndexFileReader& reader)
        : terms(std::move(childISRs)), farthestTerm(0), farthestTermLocation(0),nearestStartLocation(0), nearestEndLocation(0) {
        docEndISR = std::make_unique<ISREndDoc>(reader);
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

        return terms[farthestTerm]->GetCurrentPost();
    }   
    SeekObj * Next() {
        // Finds overlapping phrase matches.
        auto firstPost =  terms[0]->GetCurrentPost();
        if (!firstPost) return Seek(0);
        return Seek(firstPost->location + 1);
    }
    SeekObj * NextDocument() 
    {
        return Seek(nearestEndLocation + 1);
    }

    Location getStartLocation() override
    {
        return nearestStartLocation;
    }
    Location getEndLocation() override
    {
        return nearestEndLocation;
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
    ISRContainer(std::vector<std::unique_ptr<ISR>> childISRs, std::vector<std::unique_ptr<ISR>> excludedISRs, const IndexFileReader& reader)
        : terms(std::move(childISRs)), excluded(std::move(excludedISRs)), nearestTerm(0), farthestTerm(0), nearestStartLocation(0), nearestEndLocation(0) {
        docEndISR = std::make_unique<ISREndDoc>(reader);
    }
    unsigned CountContained, CountExcluded;

    SeekObj * Seek(Location target) {
        //1. Seek all the included ISRs to the first occurrence beginning at the target location.
        if (terms.empty()) return nullptr;
        while(true)
        {
            excludedNotFound = true;
            for (size_t i = 0; i < terms.size(); ++i)
            {
                SeekObj * seekObj = terms[i]->Seek(target);
                if (!seekObj) return nullptr;

                SeekObj * farthestPost = nullptr;
                SeekObj * nearestPost = nullptr;

                if (i != 0)
                {
                    farthestPost = terms[farthestTerm]->GetCurrentPost();
                    nearestPost = terms[nearestTerm]->GetCurrentPost();
                }
                if (!farthestPost || seekObj->location > farthestPost->location)
                {
                    farthestTerm = i;
                }
                if (!nearestPost || seekObj->location < nearestPost->location)
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
        return terms[nearestTerm]->GetCurrentPost();
    }
    SeekObj * Next() override 
    {
        SeekObj * seekObj = terms[nearestTerm]->GetCurrentPost();
        if (!seekObj) return Seek(0);
        return Seek(seekObj->location + 1);
    }
    SeekObj * NextDocument() override
    {
        return Seek(nearestEndLocation + 1);
    }


    Location getStartLocation() override
    {
        return nearestStartLocation;
    }
    Location getEndLocation() override
    {
        return nearestEndLocation;
    }
 private:
     unsigned nearestTerm, farthestTerm;
     Location nearestStartLocation, nearestEndLocation;
};