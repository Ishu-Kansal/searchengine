#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#include <string>
#include <sstream>
#include <vector>
#include <cstring>
#include <algorithm>

#include "query_parser/parser.h"
#include "constraint_solver.h"
#include "../HtmlParser/HtmlParser.h"

std::vector<vector<ISRWord*>> sequences;

struct SearchResult {
   std::string url;
   std::string title;
   std::string snippet;
};

// Helper to join vector<string> with spaces
std::string join_words(const std::vector<std::string>& words, size_t max_words = SIZE_MAX) {
   std::ostringstream oss;
   size_t count = std::min(max_words, words.size());
   for (size_t i = 0; i < count; ++i) {
       oss << words[i];
       if (i != count - 1) oss << ' ';
   }
   return oss.str();
}

// Fetch content from URL using LinuxGetSsl and return SearchResult
SearchResult get_and_parse_url(const cstring_view& url) {
   static const char *const proc = "../../LinuxGetUrl/LinuxGetSsl";
   int pipefd[2];
   if (pipe(pipefd) == -1) {
       perror("pipe");
       exit(1);
   }
   pid_t pid = fork();
   if (pid == -1) {
       perror("fork");
       exit(1);
   }
   if (pid == 0) {
       // child
       close(pipefd[0]);
       dup2(pipefd[1], STDOUT_FILENO);
       close(pipefd[1]);
       execl(proc, proc, url.data(), nullptr); // Use url.data() to get the C-string
       perror("exec");
       exit(1);
   }
   // parent
   close(pipefd[1]);
   std::string content;
   char buffer[4096];
   ssize_t bytesRead;
   while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
       content.append(buffer, bytesRead);
   }
   close(pipefd[0]);
   waitpid(pid, nullptr, 0);

   SearchResult result;
   result.url = std::string(url.data());

   try {
      HtmlParser parser(content.c_str(), content.size());
      result.title = join_words(parser.titleWords);
      result.snippet = join_words(parser.words, 30);
   } catch (...) {
       result.title = "";
       result.snippet = "";
   }
   return result;
}

// driver function for the search engine
std::vector<cstring_view> run_engine(std::string& query) {
   QueryParser parser(query);
   Constraint *c = parser.Parse();

   if (c) {
      ISR* isrs = c->Eval();
      std::vector<std::pair<cstring_view, int>> raw_results = constraint_solver(isrs, sequences);

      std::vector<cstring_view> urls;
      urls.reserve(raw_results.size());
      std::transform(raw_results.begin(), raw_results.end(), std::back_inserter(urls),
                     [](const std::pair<cstring_view, int>& p) { return p.first; });

      return urls;

   } else {
      return {};  // Return empty vector on failure
   }
}
