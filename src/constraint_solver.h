#include "isr.h"
#include "ranker/dynamic_ranker."
std::vector<pair<string, int>> constraint_solver(ISR* queryISR, vector<ISRWord*> orderedQueryTerms) {
    // create an ISR for document seeking
    ISREndDoc currDoc; 
    Post* currPost = queryISR->Seek(queryISR->GetStartLocation()); 
    // TO DO: Loop over ISR words vector to get the anchor term

    while (queryISR) {
        // TO DO: keep looping ISR and get the location and feed to dynamic ranker


    
    }
}
    
    
