#include <stdio.h>
#include <iostream>
#include <curl/curl.h>
#include <string>


/* GET html code for website with url
Prints html to file with name outfilename
*/
CURLcode getHTMLFromURL(std::string url, std::string outfilename) {
    CURLcode res;
    CURL *curl = curl_easy_init();
    std::cout << "inital url: " << url << std::endl;
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        FILE *fptr = fopen(("pages/" + outfilename).c_str(), "wb");
        if(!fptr)
            fprintf(stderr, "cannot write to file");
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fptr);
        res = curl_easy_perform(curl);
        // Check for errors
        if(res != CURLE_OK)
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
    
        // cleanup
        curl_easy_cleanup(curl);
    }

    return res;
}