#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>

using namespace std;
/*
If html element contains dictionary word for urban dictionary, returns the URL of word in element
Else, returns empty string
Parameter: html tag starting with '<' and ending with '>'
*/
string getUrbDictWordURL(string htmlElem);


/*
Takes in HTML file and parses tags. If tag contains link to another dictionary word, adds to vector.
Returns vector of partial URLs of word definitions to crawl. 
*/

vector<string> parseHTMLFile(string filename);

