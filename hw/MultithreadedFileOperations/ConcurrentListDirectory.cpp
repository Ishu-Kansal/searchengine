#include <iostream>
#include <pthread.h>
#include <queue>
#include <string>
#include <set>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

using namespace std;

set<string> seen;

queue<string> directory_queue;

pthread_mutex_t queue_lock;

void* read_directory(void* args);

void print_directory(const set<string>& seen) {
   /* You may modify/not use this function, but make sure your output matches */
    for (std::string s : seen) {
      if (s.back() == '/')
         s.pop_back();
      std::cout << s << "\n";
   }
}

int main(int argc, char** argv) {
   if (argc != 2) {
      std::cerr << "Usage: ./listDirectory.exe <directory>" << std::endl;
      return 1;
   }
   
   /* Your main function goes here. This might be a good place to initialize your threads. 
    * This lab will not be graded for style. 
    * You must have a multithreaded program using pthreads.
    * You may use anything from the stl except stl threads.
    */

   string root_directory = argv[1];

   directory_queue.push(root_directory);

   int num_threads = 2;

   pthread_t threads[num_threads];

   // Create threads
   for (int i = 0; i < num_threads; i++) {
      pthread_create(&threads[i], NULL, read_directory, NULL);
   }

   // Join threads
   for (int i = 0; i < num_threads; i++) {
      pthread_join(threads[i], NULL);
   }

   print_directory(seen);

   return 0;
}

const char *Filetype(mode_t mode) {
   switch (mode & S_IFMT) {
      case S_IFSOCK:
         return "socket";
      case S_IFLNK:
         return "symbolic link";
      case S_IFREG:
         return "regular file";
      case S_IFBLK:
         return "block device";
      case S_IFDIR:
         return "directory";
      case S_IFCHR:
         return "character device";
      case S_IFIFO:
         return "FIFO";
      default:
         return "unknown";
   }
}

bool DotName(const char *name) {
   return name[0] == '.' &&
      (name[1] == 0 || name[1] == '.' && name[2] == 0);
}

void *read_directory(void *args) {
   /* Your thread function goes here */
   while (true) {
      pthread_mutex_lock(&queue_lock);
      if (directory_queue.empty()) {
         pthread_mutex_unlock(&queue_lock);
         break;
      }
      string filePath = directory_queue.front();
      directory_queue.pop();
      pthread_mutex_unlock(&queue_lock);

      DIR *handle = opendir(filePath.c_str());

      if (handle) {
         struct dirent *entry;
         while ((entry = readdir(handle))) {
            struct stat statbuf;
            string childPath = filePath + '/';
            childPath += entry->d_name;
            if (DotName(entry->d_name)) {
               continue; 
            }
            seen.insert(childPath);

            if (stat(childPath.c_str(), &statbuf))
               cerr << "stat of " << filePath << " failed, errno = " << errno << endl;
            else {
               if ((statbuf.st_mode & S_IFDIR) == S_IFDIR) {
                  if (!DotName(entry->d_name)) {
                     pthread_mutex_lock(&queue_lock);
                     directory_queue.push(childPath);
                     pthread_mutex_unlock(&queue_lock);
                  }
               }
            }
         }
         closedir(handle);
      }
   }
}