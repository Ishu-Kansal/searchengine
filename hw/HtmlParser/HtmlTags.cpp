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

   // if (!nameEnd) {
   //    nameEnd = name + strlen(name);
   // }

   // size_t left = 0;
   // size_t right = NumberOfTags - 1;
   // int tag_size = nameEnd - name;

   // std::cout << std::string(name, nameEnd) << std::endl;
   // for (size_t pos = 0; pos <= tag_size; pos++)
   // {

   //    if (left == right)
   //       break;
      
   //    char curr_char = tolower(name[pos]);

   //    size_t mid = (left + right) / 2;

   //    //std::cout << curr_char << std::endl;
   //    if (strlen(TagsRecognized[mid].Tag) < pos)
   //    {
   //       left = mid + 1;
   //    }
   //    else if ((TagsRecognized[mid].Tag[pos]) < curr_char || TagsRecognized[mid].Tag[pos] == '\0')
   //    {
   //       left = mid + 1;
   //    }
   //    else if ((TagsRecognized[mid].Tag[pos]) > curr_char)
   //    {
   //       right = mid - 1;
   //    }

   //    if (left > right)
   //       return DesiredAction::OrdinaryText;
   // }
   // std::cout << left << std::endl;
   // std::cout << right << std::endl;

   // // std::cout << TagsRecognized[left].Tag  << std::endl;
   // for (int index = left; index <= right; ++index) 
   // {
   //    std::cout << TagsRecognized[index].Tag << std::endl;
   //    if (strlen(TagsRecognized[index].Tag) == tag_size + 1)
   //       return TagsRecognized[index].Action;
         
   // }

   // return DesiredAction::OrdinaryText;



   // if (!nameEnd) {
   //      nameEnd = name + strlen(name);
   //  }

   //  size_t left = 0;
   //  size_t right = NumberOfTags - 1;
   //  int tag_size = nameEnd - name;

   //  while (left <= right) {
   //      size_t mid = (left + right) / 2;
   //      const char *tag = TagsRecognized[mid].Tag;
        
   //      int cmp = 0;
   //      for (size_t pos = 0; pos < tag_size; pos++) {
   //          char curr_char = tolower(name[pos]);
   //          char tag_char = tolower(tag[pos]);

   //          if (tag_char < curr_char) {
   //              cmp = -1;
   //              break;
   //          } else if (tag_char > curr_char) {
   //              cmp = 1;
   //              break;
   //          }
   //      }

   //      if (cmp == 0) {
   //          if (strlen(tag) == static_cast<size_t>(tag_size)) {
   //              return TagsRecognized[mid].Action;
   //          }
   //          cmp = (strlen(tag) < static_cast<size_t>(tag_size)) ? -1 : 1;
   //      }

   //      if (cmp < 0) {
   //          left = mid + 1;
   //      } else {
   //          right = mid - 1;
   //      }
   //  }

   // return DesiredAction::OrdinaryText;



   // size_t len = nameEnd - name + 1;
   // if (len > LongestTagLength)
   //    return DesiredAction::OrdinaryText;

   // char* lowerName = new char[len + 1];
   // std::strncpy(lowerName, name, len);
   // lowerName[len] = '\0';
   
   // // Convert to lowercase
   // for (size_t i = 0; i < len; ++i) {
   //    lowerName[i] = tolower((unsigned char)lowerName[i]);
   // }

   // // Binary search
   // int right = NumberOfTags - 1;
   // int left = 0;

   // while (left <= right) {
   //    int mid = (left + right) / 2;

   //    int compare = std::strcmp(TagsRecognized[mid].Tag, lowerName);
   //    if (compare == 0) {
   //       return TagsRecognized[mid].Action;
   //    }
   //    if (compare < 0) {
   //       left = mid + 1;;
   //    }
   //    else {
   //       right = mid - 1;
   //    }
   // }

   // delete[] lowerName;

   // return DesiredAction::OrdinaryText;

   size_t len = nameEnd - name + 1;
   if (len > LongestTagLength)
      return DesiredAction::OrdinaryText;

   // Use a stack-allocated buffer instead of heap allocation
   char lowerName[LongestTagLength + 1];
   
   // Convert to lowercase in place without extra allocation
   for (size_t i = 0; i < len; ++i) {
      lowerName[i] = std::tolower(static_cast<unsigned char>(name[i]));
   }
   lowerName[len] = '\0'; // Null-terminate

   // Perform binary search
   int left = 0, right = NumberOfTags - 1;
   while (left <= right) {
      int mid = (left + right) / 2;
      
      // Use std::tolower on the fly instead of modifying original string
      int compare = strcasecmp(TagsRecognized[mid].Tag, lowerName);
      if (compare == 0) {
         return TagsRecognized[mid].Action;
      }
      if (compare < 0) {
         left = mid + 1;
      } else {
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