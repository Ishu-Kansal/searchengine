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
    unsigned GetDocumentCount();
    unsigned GetNumberOfOccurrences();
    virtual Post* GetCurrentPost();
};

class ISREndDoc : public ISRWord {
public:
    unsigned GetDocumentLength();
    unsigned GetTitleLength();
    unsigned GetUrlLength();
};

class ISROr : public ISR {
public:
    ISR** Terms;
    unsigned NumberOfTerms;

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

        for (unsigned i = 0; i < NumberOfTerms; i++) {
            Post* currPost = Terms[i]->Seek(target);
            if (currPost != nullptr) {
                Location currStart = Terms[i]->GetStartLocation();
                if (currStart < nearestStartLocation) {
                    nearestStartLocation = currStart; // Update the nearest start location
                    nearestEndLocation = Terms[i]->GetEndLocation();
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

        Terms[nearestTerm]->Next(); // Move the nearest term to the next post

        nearestStartLocation = std::numeric_limits<Location>::max();
        nearestEndLocation = std::numeric_limits<Location>::max();
        nearestTerm = -1;

        Post* nearestPost = nullptr;

        for (unsigned i = 0; i < NumberOfTerms; i++) {
            Post* currPost = Terms[i]->getCurrentPost(); // Get the current post for each term
            if (currPost != nullptr) {
                Location currStart = Terms[i]->GetStartLocation();
                if (currStart < nearestStartLocation) {
                    nearestStartLocation = currStart; // Update the nearest start location
                    nearestEndLocation = Terms[i]->GetEndLocation();
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
    ISR** Terms;
    unsigned NumberOfTerms;

    Post* Seek(Location target) {
        // 1. Seek all the ISRs to the first occurrence beginning at
        //    the target location.
        // 2. Move the document end ISR to just past the furthest word, then calculate the document begin location.
        // 3. Seek all the other terms to past the document begin.
        // 4. If any term is past the document end, return to step 2.
        // 5. If any ISR reaches the end, there is no match.
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
    ISR** Terms;
    unsigned NumberOfTerms;

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