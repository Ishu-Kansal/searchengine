#include <stdio.h>
#include <curl/curl.h>
#include <string>

/* GET html code for website with url
Prints html to file with name outfilename
*/
CURLcode getHTMLFromURL(std::string url, std::string outfilename);