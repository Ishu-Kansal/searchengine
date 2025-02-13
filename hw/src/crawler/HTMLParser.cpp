#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>

using namespace std;

string wordURL = "/define.php?term=";

string getUrbDictWordURL(string htmlElem) {
    int exists = htmlElem.find(wordURL);
    if(exists != string::npos) {
        string substr = htmlElem.substr(exists);
        int endOfStr = substr.find('\"');
        // cout << "substr " << substr << endl;
        // cout << "end of str " << endOfStr << endl;
        // cout << "init pos " << exists << endl;
        return htmlElem.substr(exists, endOfStr);
    }
    else return "";
}

vector<string> parseHTMLFile(string filename) {
    string tag;
    vector<string> urls;
    ifstream file("pages/" + filename);

    while(getline(file, tag, '<')) {
        string partialURL = getUrbDictWordURL(tag);
        if(partialURL != "") {
            urls.push_back(partialURL);
        }
    }

    return urls;
}