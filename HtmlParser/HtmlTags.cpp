#include <ctype.h>
#include <cstring>
#include <cassert>
#include "HtmlTags.h"
#include <cstddef>

#include <iostream>

// name points to beginning of the possible HTML tag name.
// nameEnd points to one past last character.
// Comparison is case-insensitive.
// Use a binary search.
// If the name is found in the TagsRecognized table, return
// the corresponding action.
// If the name is not found, return OrdinaryText.


/*
 * Given a character range representing a potential HTML tag name,
 * this function converts the name to lowercase and performs a binary search
 * against the `TagsRecognized` array to determine what action should be taken
 * during HTML parsing.
 */
DesiredAction LookupPossibleTag( const char *name, const char *nameEnd ) {

   size_t len = nameEnd - name + 1;
   if (len > LongestTagLength)
      return DesiredAction::Discard;

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

   return DesiredAction::Discard;


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
