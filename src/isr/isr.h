#pragma once

#include <memory>
#include "../inverted_index/IndexFileReader.h"

typedef size_t Location; // Location 0 is the null location.
typedef size_t FileOffset;

constexpr Location NULL_LOCATION = 0;
static std::string endDoc = "!#$%!#13513sfas";



class ISR {
public:

    virtual SeekObj* Seek(Location target, int chunkNum) = 0;
    virtual SeekObj* NextDocument(Location docEnd, int chunkNum) = 0;

    virtual SeekObj* Next(int chunkNum) = 0;
   
    virtual Location getStartLocation() const {
        return nearestStartLocation;
    }
    virtual Location getEndLocation() const {
        return nearestEndLocation;
    }
    
    virtual std::shared_ptr<SeekObj> GetCurrentPost() const {
        return currPost;
    }
    ISR() = default;
    virtual ~ISR() = default;

protected:
    shared_ptr<SeekObj> currPost = nullptr;
    int currChunk = -1;
    Location postingListOffset = NULL_LOCATION;
    Location nearestStartLocation = NULL_LOCATION;
    Location nearestEndLocation = NULL_LOCATION;

};

class ISRWord : public ISR {
public:
    ISRWord() = delete;
    ISRWord(std::string word_in, const IndexFileReader& reader)
        : reader_(reader), word(std::move(word_in)) {}
    
    SeekObj* Seek(Location target, int chunkNum) override {
        auto curr = GetCurrentPost();
        uint32_t seekTableIdx = curr ? curr->seekTableIndex : 0;

        if (currChunk != chunkNum)
        {
            seekTableIdx = 0;
            currChunk = chunkNum;
        }

        currPost = reader_.Find(word, target, chunkNum, seekTableIdx);

        return currPost.get();
    }

    SeekObj* Next(int chunkNum) override {
        if (!currPost) return Seek(0, chunkNum);
        return Seek(currPost->location + 1, chunkNum);
    }

    SeekObj* NextDocument(Location docEnd, int chunkNum) override {
        return Seek(docEnd + 1, chunkNum);
    }

    unsigned GetNumberOfOccurrences() const{
        if (!currPost) return 0;
        return currPost->numOccurrences;
    }
    int GetSeekTableIndex() const
    {
        if (!currPost) return 0;
        return currPost->seekTableIndex;
    }
    const std::string& GetWord() const {
        return word;
    }

protected:
    const IndexFileReader& reader_;
    std::string word;
};

class ISREndDoc : public ISRWord {
public:
    ISREndDoc(const IndexFileReader& reader)
        : ISRWord(endDoc, reader) {}
        
    SeekObj* Seek(Location target, int chunkNum) override {
        return ISRWord::Seek(target, chunkNum);
    }

    unsigned GetDocumentLength() const {
        if (!currPost) return 0;
        return currPost->delta; 
    }
    
};  

class ISROr : public ISR {
public:
    std::vector<std::unique_ptr<ISR>> terms;
    std::unique_ptr<ISREndDoc> docEndISR;

    ISROr(std::vector<std::unique_ptr<ISR>> childISRs, const IndexFileReader& reader)
    : terms(std::move(childISRs)), nearestTerm(0) {
        docEndISR = make_unique<ISREndDoc>(reader);
    }

    SeekObj* Seek(Location target, int chunkNum) override {
        if (terms.empty()) 
        {
            currPost = nullptr;
            nearestStartLocation = NULL_LOCATION;
            nearestEndLocation = NULL_LOCATION;
            return nullptr;
        }
        // Seek all the ISRs to the first occurrence beginning at
        // the target location. Return null if there is no match.
        // The document is the document containing the nearest term.
        Location minLocation = std::numeric_limits<Location>::max();
        std::shared_ptr<SeekObj> nearestResult = nullptr;

        for (auto& termPtr : terms) 
        {   
            termPtr->Seek(target, chunkNum);
            std::shared_ptr<SeekObj> childResult = termPtr->GetCurrentPost();
            if (childResult)
            {
                if (childResult->location < minLocation)
                {
                    minLocation = childResult->location;
                    minLocation = childResult->location;
                    nearestResult = childResult;
                }
            }
        }
        currPost = nearestResult; 
        if (currPost)
        {
            SeekObj* docEnd = docEndISR->Seek(currPost->location, chunkNum);
            if (docEnd) 
            {
                nearestStartLocation = docEnd->location - docEndISR->GetDocumentLength();
                nearestEndLocation = docEnd->location;
            }
        }
        else
        {
            nearestStartLocation = NULL_LOCATION;
            nearestEndLocation = NULL_LOCATION;
        }
        return currPost.get();
    }

    SeekObj* Next(int chunkNum) override {
        // Do a next on the nearest term, then return
        // the new nearest match.
        if (!currPost) {
            return Seek(0, chunkNum);
        }
        return Seek(currPost->location + 1, chunkNum);
    }
    SeekObj* NextDocument(Location docEnd, int chunkNum)  override {
        // Seek all the ISRs to the first occurrence just past
        // the end of this document.
        if (docEnd) 
        {
            return Seek(docEnd + 1, chunkNum);
        }
        Location currEnd = getEndLocation();
        return Seek(currEnd + 1, chunkNum);
    }

private:
    unsigned nearestTerm;
    // nearStartLocation and nearestEndLocation are the start and end of the nearestTerm.
};

class ISRAnd : public ISR {
public:
    std::vector<std::unique_ptr<ISR>> terms;
    std::unique_ptr<ISREndDoc> docEndISR;
    ISRAnd(std::vector<std::unique_ptr<ISR>> childISRs, const IndexFileReader& reader)
        : terms(std::move(childISRs)), nearestTerm(0), farthestTerm(0) {
            docEndISR = make_unique<ISREndDoc>(reader);
        }

    SeekObj * Seek(Location target, int chunkNum) override {
        if (terms.empty()) 
        {
            currPost = nullptr;
            nearestStartLocation = NULL_LOCATION;
            nearestEndLocation = NULL_LOCATION;
            return nullptr;
        }

        bool anotherOne = false;

        Location minLocation = std::numeric_limits<Location>::max();
        Location farthestLocation = NULL_LOCATION;
        std::shared_ptr<SeekObj> nearestResult = nullptr;

        // 1. Seek all the ISRs to the first occurrence beginning at the target location.
        for (auto & termPtr : terms)
        {
            termPtr->Seek(target, chunkNum);
            std::shared_ptr<SeekObj> childResult = termPtr->GetCurrentPost();

            // 5. If any ISR reaches the end, there is no match.
            if (!childResult)
            {
                currPost = nullptr;
                nearestStartLocation = NULL_LOCATION;
                nearestEndLocation = NULL_LOCATION;
                return nullptr;
            }
            if (childResult->location > farthestLocation)
            {
                farthestLocation = childResult->location;
            }
        }

        while(true)
        {
            // 2. Move the document end ISR to just past the furthest word, then calculate the document begin location.
            anotherOne = false;
            SeekObj* docEnd = docEndISR->Seek(farthestLocation, chunkNum);
            
            if (!docEnd) 
            {
                currPost = nullptr;
                nearestStartLocation = NULL_LOCATION;
                nearestEndLocation = NULL_LOCATION;
                return nullptr;
            }

            nearestStartLocation = docEnd->location - docEndISR->GetDocumentLength();
            nearestEndLocation = docEnd->location;

            // 3. Seek all the other terms to past the document begin.
            for (auto & termPtr : terms)
            {
                termPtr->Seek(nearestStartLocation, chunkNum);
                std::shared_ptr<SeekObj> childResult = termPtr->GetCurrentPost();

                if (!childResult)
                {
                    currPost = nullptr;
                    nearestStartLocation = NULL_LOCATION;
                    nearestEndLocation = NULL_LOCATION;
                    return nullptr;
                }
                 // 4. If any term is past the document end, return to step 2.
                if (childResult->location > nearestEndLocation)
                {
                    farthestLocation = childResult->location;
                    minLocation = std::numeric_limits<Location>::max();
                    nearestResult = nullptr;
                    anotherOne = true;
                    break;
                }
                if (childResult->location < minLocation)
                {
                    minLocation = childResult->location;
                    nearestResult = childResult;
                }
            }
            if (!anotherOne) break;
        }
        currPost = nearestResult;
        return currPost.get();
    
}
    SeekObj * Next(int chunkNum) override 
    {
        if (!currPost) 
        {
            return Seek(0, chunkNum);
        }
        return Seek(currPost->location + 1, chunkNum);
    }
    SeekObj * NextDocument(Location docEnd, int chunkNum) override
    {   
        if (docEnd) 
        {
            return Seek(docEnd + 1, chunkNum);
        }
        auto seekResult = Seek(nearestEndLocation + 1, chunkNum);
        return seekResult;
    }


private:
    unsigned nearestTerm, farthestTerm;
};

class ISRPhrase : public ISR {
public:
    std::vector<std::unique_ptr<ISR>> terms;
    std::unique_ptr<ISREndDoc> docEndISR;
    bool anotherOne = false; // Need to know if we need return to step 2

    ISRPhrase(std::vector<std::unique_ptr<ISR>> childISRs, const IndexFileReader& reader)
        : terms(std::move(childISRs)), farthestTerm(0) {
        docEndISR = std::make_unique<ISREndDoc>(reader);
    }
    SeekObj * Seek(Location target, int chunkNum) override {

        if (terms.empty())
        {
            currPost = nullptr;
            nearestStartLocation = NULL_LOCATION;
            nearestEndLocation = NULL_LOCATION;
            return nullptr;
        }
        
        Location maxLocation = NULL_LOCATION;
        
        // 1. Seek all ISRs to the first occurrence beginning at the target location.
        for (size_t i = 0; i < terms.size(); ++i)
        {
            terms[i]->Seek(target, chunkNum);
            shared_ptr<SeekObj> childResult = terms[i]->GetCurrentPost();
            // 4. If any ISR reaches the end, there is no match.
            if (!childResult)
            {
                currPost = nullptr;
                nearestEndLocation = NULL_LOCATION;
                nearestStartLocation = NULL_LOCATION;
                return nullptr;
            }
            if (childResult->location > maxLocation)
            {
                maxLocation = childResult->location;
                farthestTerm = i;
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
                    Location relativePos = maxLocation + i - farthestTerm;
                    SeekObj * tempObj = terms[i]->Seek(relativePos, chunkNum);
                    if (!tempObj)       
                    {
                        currPost = nullptr;
                        nearestStartLocation = NULL_LOCATION;
                        nearestEndLocation = NULL_LOCATION;
                        return nullptr;
                    }
                    if (tempObj->location > relativePos)
                    {
                        // Out of relative order, have to go back to step 2
                        farthestTerm = i;
                        maxLocation = tempObj->location;
                        anotherOne = true;
                        break;
                    }
                }
            }
            // 3. If any term is past the desired location, return to step 2.
            if (!anotherOne) break;
        }
        SeekObj * docEnd = docEndISR->Seek(maxLocation, chunkNum);

        if (!docEnd)
        {
            currPost = nullptr;
            nearestStartLocation = NULL_LOCATION;
            nearestEndLocation = NULL_LOCATION;
            return nullptr;
        }

        nearestStartLocation = docEnd->location - docEndISR->GetDocumentLength();
        nearestEndLocation = docEnd->location;

        currPost = terms[0]->GetCurrentPost();
        return currPost.get();

    }   
    SeekObj * Next(int chunkNum) override {
        // Finds overlapping phrase matches.
        auto firstPost =  terms[0]->GetCurrentPost();
        if (!firstPost) return Seek(0, chunkNum);
        auto seekResult = Seek(firstPost->location + 1, chunkNum);
        return seekResult;
    }
    SeekObj * NextDocument(Location docEnd, int chunkNum) override
    {   
        if (docEnd) 
        {
            return Seek(docEnd + 1, chunkNum);
        }
        auto seekResult = Seek(nearestEndLocation + 1, chunkNum);
        return seekResult;
    }
private:
    unsigned farthestTerm;
};

class ISRContainer : public ISR {
public:
    std::vector<std::unique_ptr<ISR>> terms;
    std::vector<std::unique_ptr<ISR>> excluded;
    std::unique_ptr<ISREndDoc> docEndISR;
    bool anotherOne = false;
    bool excludedTermFound = false;
    ISRContainer(std::vector<std::unique_ptr<ISR>> childISRs, std::vector<std::unique_ptr<ISR>> excludedISRs, const IndexFileReader& reader)
        : terms(std::move(childISRs)), excluded(std::move(excludedISRs)) {

    }
    unsigned CountContained, CountExcluded;

    SeekObj * Seek(Location target, int chunkNum) override 
    {
        if (terms.empty())
        {
            currPost = nullptr;
            nearestStartLocation = NULL_LOCATION;
            nearestEndLocation = NULL_LOCATION;
            return nullptr;
        }

        Location farthestLocation = NULL_LOCATION;
        Location minLocation = std::numeric_limits<Location>::max();
        std::shared_ptr<SeekObj> nearestResult = nullptr;

        while (true)
        {
            excludedTermFound = false;
            // 1. Seek all the ISRs to the first occurrence beginning at the target location.
            for (auto & termPtr : terms)
            {
                termPtr->Seek(target, chunkNum);
                std::shared_ptr<SeekObj> childResult = termPtr->GetCurrentPost();
                // 5. If any ISR reaches the end, there is no match.
                if (!childResult)
                {
                    currPost = nullptr;
                    nearestStartLocation = NULL_LOCATION;
                    nearestEndLocation = NULL_LOCATION;
                    return nullptr;
                }
                if (childResult->location > farthestLocation)
                {
                    farthestLocation = childResult->location;
                }
            }
            while(true)
            {
                // 2. Move the document end ISR to just past the furthest contained ISR, then calculate the document begin location.
                anotherOne = false;
                SeekObj * docEnd = docEndISR->Seek(farthestLocation, chunkNum);
                if (!docEnd)
                {
                    currPost = nullptr;
                    nearestStartLocation = NULL_LOCATION;
                    nearestEndLocation = NULL_LOCATION;
                    return nullptr;
                }

                nearestStartLocation = docEnd->location - docEndISR->GetDocumentLength();
                nearestEndLocation = docEnd->location;

                // 3. Seek all the other contained terms to past the document begin.
                for (auto & termPtr : terms)
                {
                    termPtr->Seek(target, chunkNum);
                    std::shared_ptr<SeekObj> childResult = termPtr->GetCurrentPost();
                    // 5. If any ISR reaches the end, there is no match.
                    if (!childResult)
                    {
                        currPost = nullptr;
                        nearestStartLocation = NULL_LOCATION;
                        nearestEndLocation = NULL_LOCATION;
                        return nullptr;
                    }
                    // 4. If any contained term is past the doc end, return to step 2.
                    if (childResult->location > nearestEndLocation)
                    {
                        anotherOne = true;
                        farthestLocation = childResult->location;
                        minLocation = std::numeric_limits<Location>::max();
                        nearestResult = nullptr;
                        break;
                    }
                    if (childResult->location < minLocation)
                    {
                        nearestResult = childResult;
                        minLocation = childResult->location;
                    }
                }
                if (!anotherOne)
                {
                    // 6. Seek all the excluded ISRs to the first occurrence beginning at the document begin location.
                    for (auto & excludedTermsPtr : excluded)
                    {
                        excludedTermsPtr->Seek(nearestStartLocation, chunkNum);
                        shared_ptr<SeekObj> excludedChildResult = excludedTermsPtr->GetCurrentPost();

                        if (!excludedChildResult) continue;

                        // 7. If any excluded ISR falls within the document, reset the target to one past the end of the document and return to step 1.
                        if (excludedChildResult->location < nearestEndLocation)
                        {
                            target = nearestEndLocation + 1;
                            farthestLocation = NULL_LOCATION;
                            minLocation = std::numeric_limits<Location>::max();
                            nearestResult = nullptr;
                            excludedTermFound = true;
                            break;
                        }
                    }
                }
            }
            if (!excludedTermFound) break;
        }
        currPost = nearestResult;
        return currPost.get();
    }
    SeekObj * Next(int chunkNum) override 
    {
        if (!currPost) 
        {
            return Seek(0, chunkNum);
        }
        return Seek(currPost->location + 1, chunkNum);
    }
    SeekObj * NextDocument(Location docEnd, int chunkNum) override
    {   
        if (docEnd) 
        {
            return Seek(docEnd + 1, chunkNum);
        }
        auto seekResult = Seek(nearestEndLocation + 1, chunkNum);
        return seekResult;
    }
 private:
     unsigned nearestTerm, farthestTerm;

};
