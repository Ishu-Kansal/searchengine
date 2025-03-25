/*
Crawler Header File
Defines several useful functions to startup crawler,
crawl, log data into file structure, store data in global mem, etc
*/

#include <queue>  // need to figure out how to implement
#include <set>    // same

#include "../../StringVector/string.h"
#include "../../StringVector/vector.h"

int get_and_parse_url(const char *url, int fd);
int get_file_size(int fd);
int partition(int left, int right, int pivot_index);
void quickselect(int left, int right, int k);
void fill_queue();
std::string get_next_url();
void *runner(void *);


/* Loads seed list into memory, reads and loads state from logger files
   arg: filename of either seed list or checkpoint file */
void startup(std::string &filename);
