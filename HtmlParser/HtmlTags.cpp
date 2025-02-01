#include <ctype.h>
#include <cstring>
#include <cassert>
#include "HtmlTags.h"
#include <iostream>

// name points to beginning of the possible HTML tag name.
// nameEnd points to one past last character.
// Comparison is case-insensitive.
// Use a binary search.
// If the name is found in the TagsRecognized table, return
// the corresponding action.
// If the name is not found, return OrdinaryText.

DesiredAction LookupPossibleTag( const char *name, const char *nameEnd ) {
   // Your code here.
   size_t len = nameEnd - name + 1;
   char *lowerName = new char[len + 1];
   std::strncpy(lowerName, name, len);
   lowerName[len] = '\0';
   // std::cout <<"name: " << lowerName << "btw" << std::endl;

   // Convert to lowercase
   for (size_t i = 0; i < len; ++i) {
      lowerName[i] = tolower((unsigned char)lowerName[i]);
   }

   // Binary search
   int right = NumberOfTags - 1;
   int left = 0;

   while (left <= right) {
      int mid = (left + right) / 2;
      int compare = std::strcmp(TagsRecognized[mid].Tag, lowerName);
      if (compare == 0) {
         return TagsRecognized[mid].Action;
      }
      if (compare < 0) {
         left = mid + 1;;
      }
      else {
         right = mid - 1;
      }
   }

   return DesiredAction::OrdinaryText;
}


const char* printAction(enum DesiredAction action) {
   switch(action) {
      case DesiredAction::OrdinaryText:
         return "OrdinaryText";
      case DesiredAction::Title:
         return "Title";
      case DesiredAction::Comment:
         return "Comment";
      case DesiredAction::Discard:
         return "Discard";
      case DesiredAction::DiscardSection:
         return "DiscardSection";
      case DesiredAction::Anchor:
         return "Anchor";
      case DesiredAction::Base:
         return "Base";
      case DesiredAction::Embed:
         return "Embed";
      default:
         return "false tag";
   }
}

// #include <iostream>
// int main(int argc, char** argv) {
//    int len = atoi(argv[2]);
//    const char* tag = "n <base";
//    std::cout << *(tag+3) << std::endl;
//    // DesiredAction act = LookupPossibleTag(argv[1], argv[1]+len);
//    DesiredAction act = LookupPossibleTag(tag+3, tag + 3 + 4);

//    std::cout << printAction(act) << std::endl;
// }