#include <stdlib.h>
#include <iostream>
#include <fstream>
#include "HtmlParser.h"

using namespace std;

char *ReadFile(char *filename, size_t &fileSize)
{
    // Read the file into memory.
    // You'll soon learn a much more efficient way to do this.

    // Attempt to Create an istream and seek to the end
    // to get the size.
    ifstream ifs(filename, ios::ate | ios::binary);
    if (!ifs.is_open())
        return nullptr;
    fileSize = ifs.tellg();

    // Allocate a block of memory big enough to hold it.
    char *buffer = new char[fileSize];

    // Seek back to the beginning of the file, read it into
    // the buffer, then return the buffer.
    ifs.seekg(0);
    ifs.read(buffer, fileSize);
    return buffer;
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        cout << "Usage:  HtmlParser [wtab] filename" << endl;
        exit(1);
    }

    size_t fileSize = 0;
    char *buffer = ReadFile(argv[1 + (argc == 3)], fileSize);
    if (!buffer)
    {
        cerr << "Could not open the file." << endl;
        exit(1);
    }

    HtmlParser parser(buffer, fileSize);

    cout << "base:" << parser.base << "\n\n";

    cout << "title words:\n";
    for (string word : parser.titleWords) {
        cout << word << '\n';
    }
    cout << '\n';

    cout << "image count:" << parser.img_count << "\n\n";

    cout << "words:\n";
    for (string word : parser.words) {
        cout << word << '\n';
    }
    cout << '\n';

    cout << "links:\n";
    for (Link link : parser.links) {
        cout << link.URL << '\n';
    }

    cout << "description:\n";
    for (string desc : parser.description) {
        cout << desc << '\n';
    }

    delete[] buffer;
}