//
// C++ Implementation: GUIDData
//
// Description: GUIDData class header
// Implements the GUIDData data structure and support methods
//
//
// Author: Rod Smith <rodsmith@rodsbooks.com>, (C) 2010
//
// Copyright: See COPYING file that comes with this distribution
//
//

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include <stdio.h>
#include <string>
#include <iostream>
#include "guid.h"
#include "support.h"

using namespace std;

GUIDData::GUIDData(void) {
   Zero();
} // constructor

GUIDData::GUIDData(const GUIDData & orig) {
   int i;

   for (i = 0; i < 16; i++)
      uuidData[i] = orig.uuidData[i];
} // copy constructor

GUIDData::GUIDData(const char * orig) {
   operator=(orig);
} // copy (from char*) constructor

GUIDData::~GUIDData(void) {
} // destructor

GUIDData & GUIDData::operator=(const GUIDData & orig) {
   int i;

   for (i = 0; i < 16; i++)
      uuidData[i] = orig.uuidData[i];
   return *this;
} // GUIDData::operator=(const GUIDData & orig)

// Assign the GUID from a string input value. A GUID is normally formatted
// with four dashes as element separators, for a total length of 36
// characters. If the input string is this long or longer, this function
// assumes standard separator positioning; if the input string is less
// than 36 characters long, this function assumes the input GUID has
// been compressed by removal of separators. In either event, there's
// little in the way of sanity checking, so garbage in = garbage out!
GUIDData & GUIDData::operator=(const string & orig) {
   string copy, fragment;
   size_t len;
   // Break points for segments, either with or without characters separating the segments....
   size_t longSegs[6] = {0, 9, 14, 19, 24, 36};
   size_t shortSegs[6] = {0, 8, 12, 16, 20, 32};
   size_t *segStart = longSegs; // Assume there are separators between segments */

   Zero();

   // Delete stray spaces....
   copy = DeleteSpaces(orig);

   // If length is too short, assume there are no separators between segments
   len = copy.length();
   if (len < 36) {
      segStart = shortSegs;
   };

   // Extract data fragments at fixed locations and convert to
   // integral types....
   if (len >= segStart[1]) {
      uuidData[3] = StrToHex(copy, 0);
      uuidData[2] = StrToHex(copy, 2);
      uuidData[1] = StrToHex(copy, 4);
      uuidData[0] = StrToHex(copy, 6);
   } // if
   if (len >= segStart[2]) {
      uuidData[5] = StrToHex(copy, segStart[1]);
      uuidData[4] = StrToHex(copy, segStart[1] + 2);
   } // if
   if (len >= segStart[3]) {
      uuidData[7] = StrToHex(copy, segStart[2]);
      uuidData[6] = StrToHex(copy, segStart[2] + 2);
   } // if
   if (len >= segStart[4]) {
      uuidData[8] = StrToHex(copy, segStart[3]);
      uuidData[9] = StrToHex(copy, segStart[3] + 2);
   } // if
   if (len >= segStart[5]) {
      uuidData[10] = StrToHex(copy, segStart[4]);
      uuidData[11] = StrToHex(copy, segStart[4] + 2);
      uuidData[12] = StrToHex(copy, segStart[4] + 4);
      uuidData[13] = StrToHex(copy, segStart[4] + 6);
      uuidData[14] = StrToHex(copy, segStart[4] + 8);
      uuidData[15] = StrToHex(copy, segStart[4] + 10);
   } // if

   return *this;
} // GUIDData::operator=(const string & orig)

// Assignment from C-style string; rely on C++ casting....
GUIDData & GUIDData::operator=(const char * orig) {
   return operator=((string) orig);
} // GUIDData::operator=(const char * orig)

// Read a GUIDData from stdin, prompting the user along the way for the
// correct form. This is a bit more flexible than it claims; it parses
// any entry of 32 or 36 characters as a GUID (
GUIDData & GUIDData::GetGUIDFromUser(void) {
   string part1, part2, part3, part4, part5;
   char line[255];
   int entered = 0;

   cout << "\nA GUID is entered in five segments of from two to six bytes, with\n"
        << "dashes between segments.\n";
   cout << "Enter the entire GUID, a four-byte hexadecimal number for the first segment, or\n"
        << "'R' to generate the entire GUID randomly: ";
   cin.get(line, 255);
   part1 = DeleteSpaces(line);

   // If user entered 'r' or 'R', generate GUID randomly....
   if ((part1[0] == 'r') || (part1[0] == 'R')) {
      Randomize();
      entered = 1;
   } // if user entered 'R' or 'r'

   // If string length is right for whole entry, try to parse it....
   if (((part1.length() == 36) || (part1.length() == 32)) && (entered == 0)) {
      operator=(part1);
      entered = 1;
   } // if

   // If neither of the above methods of entry was used, use prompted
   // entry....
   if (entered == 0) {
      cout << "Enter a two-byte hexadecimal number for the second segment: ";
      cin >> part2;
      cout << "Enter a two-byte hexadecimal number for the third segment: ";
      cin >> part3;
      cout << "Enter a two-byte hexadecimal number for the fourth segment: ";
      cin >> part4;
      cout << "Enter a six-byte hexadecimal number for the fifth segment: ";
      cin >> part5;
      operator=(part1 += (string) "-" += part2 += (string) "-" += part3
            += (string) "-" += part4 += (string) "-" += part5);
   } // if/else
   cin.ignore(255, '\n');
   cout << "New GUID: " << AsString() << "\n";
   return *this;
} // GUIDData::GetGUIDData(void)

// Erase the contents of the GUID
void GUIDData::Zero(void) {
   int i;

   for (i = 0; i < 16; i++) {
      uuidData[i] = 0;
   } // for
} // GUIDData::Zero()

// Set a completely random GUID value....
// The uuid_generate() function returns a value that needs to have its
// first three fields byte-reversed to conform to Intel's GUID layout.
// If that function isn't defined (e.g., on Windows), set a completely
// random GUID -- not completely kosher, but it doesn't seem to cause
// any problems (so far...)
void GUIDData::Randomize(void) {
#ifdef _UUID_UUID_H
   uuid_generate(uuidData);
   ReverseBytes(&uuidData[0], 4);
   ReverseBytes(&uuidData[4], 2);
   ReverseBytes(&uuidData[6], 2);
#else
   int i;
   for (i = 0; i < 16; i++)
      uuidData[i] = rand();
#endif
} // GUIDData::Randomize

// Equality operator; returns 1 if the GUIDs are equal, 0 if they're unequal
int GUIDData::operator==(const GUIDData & orig) {
   int retval = 1; // assume they're equal
   int i;

   for (i = 0; i < 16; i++)
      if (uuidData[i] != orig.uuidData[i])
         retval = 0;

   return retval;
} // GUIDData::operator==

// Inequality operator; returns 1 if the GUIDs are unequal, 0 if they're equal
int GUIDData::operator!=(const GUIDData & orig) {
   return !operator==(orig);
} // GUIDData::operator!=

// Return the GUID as a string, suitable for display to the user.
string GUIDData::AsString(void) {
   char theString[40];

   sprintf(theString,
           "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
           uuidData[3], uuidData[2], uuidData[1], uuidData[0], uuidData[5],
           uuidData[4], uuidData[7], uuidData[6], uuidData[8], uuidData[9],
           uuidData[10], uuidData[11], uuidData[12], uuidData[13], uuidData[14],
           uuidData[15]);
   return theString;
} // GUIDData::AsString(void)

// Delete spaces or braces (which often enclose GUIDs) from the orig string,
// returning modified string.
string GUIDData::DeleteSpaces(const string & orig) {
   string copy;
   size_t position;

   copy = orig;
   for (position = copy.length(); position > 0; position--) {
      if ((copy[position - 1] == ' ') || (copy[position - 1] == '{') || (copy[position - 1] == '}')) {
         copy.erase(position - 1, 1);
      } // if
   } // for
   return copy;
} // GUIDData::DeleteSpaces()
