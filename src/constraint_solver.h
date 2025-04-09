#include "isr.h"
#include "ranker/dynamic_rank.h"

// returns outer index as the first element of the pair and the inner index as the second element of the pair
pair<int, int> get_anchor_ISR(vector<vector<ISRWord*>> orderedQueryTerms) {
     // Loop over ISR words vector to get the anchor term
     int anchorOuterIndex = -1;
     int anchorInnerIndex = -1; 
     int minOccurences = INT_MAX; 
     for (int i = 0; i < orderedQueryTerms.size(); i++) {
         for (int j = 0; j < orderedQueryTerms[i].size(); j++) {
             if (orderedQueryTerms[i][j]->GetNumberOfOccurrences() < minOccurences) {
                 anchorOuterIndex = i;
                 anchorInnerIndex = j;
                 minOccurences = orderedQueryTerms[i][j]->GetNumberOfOccurrences();
             }
         }
     }
     assert(anchorOuterIndex != -1);
     assert(anchorInnerIndex != -1);
     return {anchorOuterIndex, anchorInnerIndex}; 
    
}
// TO DO: Convert to STL
void insertionSort(std::pair<cstring_view, int> *first, std::pair<cstring_view, int> *last, std::pair<cstring_view, int> current, int N)
{
  if (last - first == N && current.second <= (last - 1)->second)
    return;
  auto marker = first;
  while (marker != last && marker->second >= current.second)
    ++marker;
  for (auto it = last < first + N - 1 ? last : first + N - 1; it > marker; --it)
  {
    *it = *(it - 1);
  }
  *marker = current;
}

// For the constraint solver
void TopN(vector<pair<cstring_view, int>> &rankedDocs, pair<cstring_view, int> rankedDoc, int N, int &currElements)
{
  // Find the top N pairs based on the values and return
  // as a dynamically-allocated array of pointers.  If there
  // are less than N pairs in the hash, remaining pointers
  // will be null.

  // Your code here.
  const auto last = rankedDocs.end();
  insertionSort(rankedDocs.data(), rankedDocs.data() + currElements, rankedDoc, N);
  if (currElements < N) ++currElements;
}

// actual constraint solver function
std::vector<pair<cstring_view, int>> constraint_solver(ISR* queryISR, vector<vector<ISRWord*>> orderedQueryTerms) {
    // create an ISR for document seeking
    vector<pair<cstring_view, int>> returnedResults;
    auto indices = get_anchor_ISR(orderedQueryTerms);
    int anchorOuterIndex = indices.first;
    int anchorInnerIndex = indices.second;
    int currElements = 0; 
    // seek to the first occurence
    Post* currPost = queryISR->NextDocument(); 
    while (currPost) {
        int docStartLoc = queryISR->GetStartLocation(); 
        int docEndLoc = queryISR->GetEndLocation(); 
        // TO DO use the relative loc to get relevant doc data
        int relativeLoc = queryISR->GetCurrentPost()->location;
        assert(false);

        // TO DO: Figure out how to get the document URL
        assert(false); 
        cstring_view documentURL; 

        // TO DO: Update this function call with actual values and figure out how to determine if its in the title or body
        assert(false); 
        int dynamic_score = get_dynamic_rank(orderedQueryTerms[anchorOuterIndex][anchorInnerIndex], orderedQueryTerms, docStartLoc, 
                            docEndLoc, "\0");
        
        // TO DO: get static rank and add it to the dynamic rank
        pair<cstring_view, int> urlPair = {documentURL, dynamic_score + static_score}; 

        TopN(returnedResults, urlPair, 10, currElements); 
        currPost = queryISR->NextDocument(); 
    }
    return returnedResults; 
}

    
    
