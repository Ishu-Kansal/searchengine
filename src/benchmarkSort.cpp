
#include <iostream>
#include <vector>
#include <queue>      
#include <functional>  
#include <algorithm>   
#include <chrono>   
#include <random>     
#include <iomanip>     

using namespace std;

const int TOTAL_DOCS_TO_RETURN = 100;
const int NUM_ELEMENTS_TO_PROCESS = 1000000;
void insertionSortAdd(vector<int>& topRankedDocs, int rankedDoc) {

    const size_t maxSize = TOTAL_DOCS_TO_RETURN;
    size_t curSize = topRankedDocs.size();
    if (curSize == TOTAL_DOCS_TO_RETURN && topRankedDocs.back() >= rankedDoc)
    {
      return;
    }
    auto docToInsert = rankedDoc;
    size_t pos;
    if (curSize < maxSize) 
    { 
      topRankedDocs.emplace_back();
      pos = curSize;
    } else 
    {
      pos = curSize - 1;
    }
    while (pos > 0 && topRankedDocs[pos - 1] < docToInsert) 
    {
      topRankedDocs[pos] = std::move(topRankedDocs[pos - 1]);
      pos--;
    }
    topRankedDocs[pos] = std::move(docToInsert);
}

using MinHeap = priority_queue<int, vector<int>, greater<int>>;

void heapAdd(MinHeap& topKHeap, int rankedDoc) {
    if (topKHeap.size() < TOTAL_DOCS_TO_RETURN) {
        topKHeap.push(rankedDoc);
    } else if (rankedDoc > topKHeap.top()) {
        topKHeap.pop(); 
        topKHeap.push(rankedDoc);
    }

}


int main() {
    cout << "Generating " << NUM_ELEMENTS_TO_PROCESS << " random integers..." << endl;
    vector<int> inputStream;
    inputStream.reserve(NUM_ELEMENTS_TO_PROCESS);
    mt19937 rng(chrono::steady_clock::now().time_since_epoch().count());
    uniform_int_distribution<int> dist(0, NUM_ELEMENTS_TO_PROCESS * 10);

    for (int i = 0; i < NUM_ELEMENTS_TO_PROCESS; ++i) {
        inputStream.push_back(dist(rng));
    }
    cout << "Data generation complete." << endl << endl;

    cout << "Benchmarking Insertion Sort Method (N=" << TOTAL_DOCS_TO_RETURN << ")..." << endl;
    vector<int> topDocsVector;
    topDocsVector.reserve(TOTAL_DOCS_TO_RETURN); 

    auto start_vector = chrono::high_resolution_clock::now();
    for (int doc : inputStream) {
        insertionSortAdd(topDocsVector, doc);
    }
    auto end_vector = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> duration_vector = end_vector - start_vector;
    cout << "Insertion Sort Time: " << fixed << setprecision(3) << duration_vector.count() << " ms" << endl;

     std::sort(topDocsVector.begin(), topDocsVector.end(), std::greater<int>());
    cout << "\nBenchmarking Heap Method (N=" << TOTAL_DOCS_TO_RETURN << ")..." << endl;
    MinHeap topDocsHeap;

    auto start_heap = chrono::high_resolution_clock::now();
    for (int doc : inputStream) {
        heapAdd(topDocsHeap, doc);
    }
    auto end_heap = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> duration_heap = end_heap - start_heap;
    cout << "Heap Time:           " << fixed << setprecision(3) << duration_heap.count() << " ms" << endl;
    return 0;
}