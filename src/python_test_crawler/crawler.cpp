#include <stdio.h>
#include <curl/curl.h>
#include "URL_Getter.h"
#include "HTMLParser.h"
#include <string>

using namespace std;
 
int main(void)
{
  CURL *curl;
  CURLcode res;
 
  curl = curl_easy_init();
  string baseURL = "https://www.urbandictionary.com";
  string baseTerm = "search";
  res = getHTMLFromURL(baseURL + "/define.php?term=" + baseTerm, baseTerm + ".txt");
  FILE *loggerFile = fopen("logs.txt", "w");
  if(res == CURLE_OK)
    fprintf(loggerFile, "Successfully loaded html for url %s\n", (baseURL + "/define.php?term=" + baseTerm).c_str());
  else 
    fprintf(loggerFile, "Error loading html for url %s\n", (baseURL + "/define.php?term=" + baseTerm).c_str());

  vector<string> urls = parseHTMLFile(baseTerm + ".txt");

  cout << urls.size() << endl;
  for(int i = 0; i < urls.size(); i++) {
    cout << urls[i] << endl;
    string term = urls[i].substr(urls[i].find('=')+1);
    res = getHTMLFromURL(baseURL + urls[i], term + ".txt");
    if(res == CURLE_OK)
        fprintf(loggerFile, "Successfully loaded html for url %s\n", (baseURL + urls[i]).c_str());
    else 
        fprintf(loggerFile, "Error loading html for url %s\n", (baseURL + urls[i]).c_str());
  }
}