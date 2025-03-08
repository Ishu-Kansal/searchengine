/*
Crawler Header File
Defines several useful functions to startup crawler,
crawl, log data into file structure, store data in global mem, etc
*/

#include <queue>  // need to figure out how to implement
#include <set>    // same

#include "../../StringVector/string.h"
#include "../../StringVector/vector.h"

std::queue<url_struct> to_explore;
std::set<url_struct> explored;

struct url_struct {
    string url;
};

class Crawler {
   public:
    Crawler();
    void exploreURL();
    void shutdown();

   private:
    ~Crawler();
};

/* Loads seed list into memory, reads and loads state from logger files
   arg: filename of either seed list or checkpoint file */
void startup(string &filename);
