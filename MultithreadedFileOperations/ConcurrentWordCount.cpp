#include <iostream>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

using namespace std;

void* word_count( void* args );

struct Data {
   char* filename;
   int word_count = 0;
};

void print_word_count( int count ) {
   /* You may modify/not use this function, but make sure your output matches */
   std::cout << "Total Words: " << count << std::endl;
}

int main( int argc, char** argv ) {
   if ( argc <= 1 ) {
      std::cerr << "Usage: ./wordCount.exe <files>" << std::endl;
      return 1;
   }
   
   /* Your main function goes here. This might be a good place to initialize your threads. 
    * This lab will not be graded for style. 
    * You must have a multithreaded program using pthreads.
    * You may use anything from the stl except stl threads.
    */

   int num_files = argc - 1;

   pthread_t threads[num_files];
   Data inputs[num_files];

   // Create a thread for each file
   for (int i = 0; i < num_files; i++) {
      inputs[i].filename = argv[i + 1];
      pthread_create(&threads[i], NULL, word_count, &inputs[i]);
   }

   int total_words = 0;

   // Join threads and add up total words
   for (int i = 0; i < num_files; i++) {
      pthread_join(threads[i], NULL);
      total_words += inputs[i].word_count;
   }

   print_word_count(total_words);

   return 0;
}

int FileSize(int f) {
   return lseek(f, 0, SEEK_END);
}

void *word_count(void *args) {
   /* Your thread function goes here */

   Data *arg = (Data *)args;
   int f = open(arg->filename, O_RDONLY);

   if (f != -1) {
      size_t file_size = FileSize(f);
      char *map = (char *)mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, f, 0);

      if (map != MAP_FAILED) {
         bool mid_word = false;
         char *end = map + file_size;
         for (char *c = map; c < end; c++) {
            switch (*c) {
               case ' ':
               case '\t':
               case '\n':
               case '\r':
                     if (mid_word) {
                        mid_word = false;
                        arg->word_count++;
                     }
                     break;
               default:
                     mid_word = true;
            }
         }
         if (mid_word) {
            arg->word_count++;
         }
      }
      close(f);
   }
}