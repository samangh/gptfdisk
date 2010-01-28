// support.cc
// Non-class support functions for gdisk program.
// Primarily by Rod Smith, February 2009, but with a few functions
// copied from other sources (see attributions below).

/* This program is copyright (c) 2009 by Roderick W. Smith. It is distributed
  under the terms of the GNU GPL version 2, as detailed in the COPYING file. */

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <string>
#include <iostream>
#include "support.h"

#include <sys/types.h>

// As of 1/2010, BLKPBSZGET is very new, so I'm explicitly defining it if
// it's not already defined. This should become unnecessary in the future.
// Note that this is a Linux-only ioctl....
#ifndef BLKPBSZGET
#define BLKPBSZGET _IO(0x12,123)
#endif

using namespace std;

// Get a numeric value from the user, between low and high (inclusive).
// Keeps looping until the user enters a value within that range.
// If user provides no input, def (default value) is returned.
// (If def is outside of the low-high range, an explicit response
// is required.)
int GetNumber(int low, int high, int def, const string & prompt) {
   int response, num;
   char line[255];

   if (low != high) { // bother only if low and high differ...
      response = low - 1; // force one loop by setting response outside range
      while ((response < low) || (response > high)) {
         cout << prompt;
         cin.getline(line, 255);
         num = sscanf(line, "%d", &response);
         if (num == 1) { // user provided a response
            if ((response < low) || (response > high))
               cout << "Value out of range\n";
         } else { // user hit enter; return default
            response = def;
         } // if/else
      } // while
   } else { // low == high, so return this value
      cout << "Using " << low << "\n";
      response = low;
   } // else
   return (response);
} // GetNumber()

// Gets a Y/N response (and converts lowercase to uppercase)
char GetYN(void) {
   char line[255];
   char response = '\0';
   char *junk;

   while ((response != 'Y') && (response != 'N')) {
      cout << "(Y/N): ";
      junk = fgets(line, 255, stdin);
      sscanf(line, "%c", &response);
      if (response == 'y') response = 'Y';
      if (response == 'n') response = 'N';
   } // while
   return response;
} // GetYN(void)

// Obtains a sector number, between low and high, from the
// user, accepting values prefixed by "+" to add sectors to low,
// or the same with "K", "M", "G", or "T" as suffixes to add
// kilobytes, megabytes, gigabytes, or terabytes, respectively.
// If a "-" prefix is used, use the high value minus the user-
// specified number of sectors (or KiB, MiB, etc.). Use the def
 //value as the default if the user just hits Enter
uint64_t GetSectorNum(uint64_t low, uint64_t high, uint64_t def, const string & prompt) {
   unsigned long long response;
   int num, plusFlag = 0;
   uint64_t mult = 1;
   char suffix;
   char line[255];

   response = low - 1; // Ensure one pass by setting a too-low initial value
   while ((response < low) || (response > high)) {
      cout << prompt;
      cin.getline(line, 255);

      // Remove leading spaces, if present
      while (line[0] == ' ')
         strcpy(line, &line[1]);

      // If present, flag and remove leading plus sign
      if (line[0] == '+') {
         plusFlag = 1;
         strcpy(line, &line[1]);
      } // if

      // If present, flag and remove leading minus sign
      if (line[0] == '-') {
         plusFlag = -1;
         strcpy(line, &line[1]);
      } // if

      // Extract numeric response and, if present, suffix
      num = sscanf(line, "%llu%c", &response, &suffix);

      // If no response, use default (def)
      if (num <= 0) {
         response = (unsigned long long) def;
	 suffix = ' ';
         plusFlag = 0;
      } // if

      // Set multiplier based on suffix
      switch (suffix) {
         case 'K':
         case 'k':
            mult = (uint64_t) 1024 / SECTOR_SIZE;
	    break;
         case 'M':
	 case 'm':
	    mult = (uint64_t) 1048576 / SECTOR_SIZE;
            break;
         case 'G':
         case 'g':
            mult = (uint64_t) 1073741824 / SECTOR_SIZE;
            break;
         case 'T':
	 case 't':
            mult = ((uint64_t) 1073741824 * (uint64_t) 1024) / (uint64_t) SECTOR_SIZE;
            break;
         default:
            mult = 1;
      } // switch

      // Adjust response based on multiplier and plus flag, if present
      response *= (unsigned long long) mult;
      if (plusFlag == 1) {
         // Recompute response based on low part of range (if default = high
         // value, which should be the case when prompting for the end of a
         // range) or the defaut value (if default != high, which should be
         // the case for the first sector of a partition).
         if (def == high)
            response = response + (unsigned long long) low - UINT64_C(1);
         else
            response = response + (unsigned long long) def - UINT64_C(1);
      } // if
      if (plusFlag == -1) {
         response = (unsigned long long) high - response;
      } // if
   } // while
   return ((uint64_t) response);
} // GetSectorNum()

// Takes a size in bytes (in size) and converts this to a size in
// SI units (KiB, MiB, GiB, TiB, or PiB), returned in C++ string
// form
string BytesToSI(uint64_t size) {
   string units;
   char theValue[99];
   float sizeInSI;

   theValue[0] = '\0';
   sizeInSI = (float) size;
   units = " bytes";
   if (sizeInSI > 1024.0) {
      sizeInSI /= 1024.0;
      units = " KiB";
   } // if
   if (sizeInSI > 1024.0) {
      sizeInSI /= 1024.0;
      units = " MiB";
   } // if
   if (sizeInSI > 1024.0) {
      sizeInSI /= 1024.0;
      units = " GiB";
   } // if
   if (sizeInSI > 1024.0) {
      sizeInSI /= 1024.0;
      units = " TiB";
   } // if
   if (sizeInSI > 1024.0) {
      sizeInSI /= 1024.0;
      units = " PiB";
   } // if
   if (units == " bytes") { // in bytes, so no decimal point
      sprintf(theValue, "%1.0f%s", sizeInSI, units.c_str());
   } else {
      sprintf(theValue, "%1.1f%s", sizeInSI, units.c_str());
   } // if/else
   return theValue;
} // BlocksToSI()

// Return a plain-text name for a partition type.
// Convert a GUID to a string representation, suitable for display
// to humans....
string GUIDToStr(struct GUIDData theGUID) {
   unsigned long long blocks[11], block;
   char theString[40];

   theString[0] = '\0';;
   blocks[0] = (theGUID.data1 & UINT64_C(0x00000000FFFFFFFF));
   blocks[1] = (theGUID.data1 & UINT64_C(0x0000FFFF00000000)) >> 32;
   blocks[2] = (theGUID.data1 & UINT64_C(0xFFFF000000000000)) >> 48;
   blocks[3] = (theGUID.data2 & UINT64_C(0x00000000000000FF));
   blocks[4] = (theGUID.data2 & UINT64_C(0x000000000000FF00)) >> 8;
   blocks[5] = (theGUID.data2 & UINT64_C(0x0000000000FF0000)) >> 16;
   blocks[6] = (theGUID.data2 & UINT64_C(0x00000000FF000000)) >> 24;
   blocks[7] = (theGUID.data2 & UINT64_C(0x000000FF00000000)) >> 32;
   blocks[8] = (theGUID.data2 & UINT64_C(0x0000FF0000000000)) >> 40;
   blocks[9] = (theGUID.data2 & UINT64_C(0x00FF000000000000)) >> 48;
   blocks[10] = (theGUID.data2 & UINT64_C(0xFF00000000000000)) >> 56;
   sprintf(theString,
            "%08llX-%04llX-%04llX-%02llX%02llX-%02llX%02llX%02llX%02llX%02llX%02llX",
            blocks[0], blocks[1], blocks[2], blocks[3], blocks[4], blocks[5],
            blocks[6], blocks[7], blocks[8], blocks[9], blocks[10]);
   return theString;
} // GUIDToStr()

// Get a GUID from the user
GUIDData GetGUID(void) {
   unsigned long long part1, part2, part3, part4, part5;
   int entered = 0;
   char temp[255], temp2[255];
   char* junk;
   GUIDData theGUID;

   cout << "\nA GUID is entered in five segments of from two to six bytes, with\n"
        << "dashes between segments.\n";
   cout << "Enter the entire GUID, a four-byte hexadecimal number for the first segment, or\n"
        << "'R' to generate the entire GUID randomly: ";
   junk = fgets(temp, 255, stdin);

   // If user entered 'r' or 'R', generate GUID randomly....
   if ((temp[0] == 'r') || (temp[0] == 'R')) {
      theGUID.data1 = (uint64_t) rand() * (uint64_t) rand();
      theGUID.data2 = (uint64_t) rand() * (uint64_t) rand();
      entered = 1;
   } // if user entered 'R' or 'r'

   // If string length is right for whole entry, try to parse it....
   if ((strlen(temp) == 37) && (entered == 0)) {
      strncpy(temp2, &temp[0], 8);
      temp2[8] = '\0';
      sscanf(temp2, "%llx", &part1);
      strncpy(temp2, &temp[9], 4);
      temp2[4] = '\0';
      sscanf(temp2, "%llx", &part2);
      strncpy(temp2, &temp[14], 4);
      temp2[4] = '\0';
      sscanf(temp2, "%llx", &part3);
      theGUID.data1 = (part3 << 48) + (part2 << 32) + part1;
      strncpy(temp2, &temp[19], 4);
      temp2[4] = '\0';
      sscanf(temp2, "%llx", &part4);
      strncpy(temp2, &temp[24], 12);
      temp2[12] = '\0';
      sscanf(temp2, "%llx", &part5);
      theGUID.data2 = ((part4 & UINT64_C(0x000000000000FF00)) >> 8) +
                      ((part4 & UINT64_C(0x00000000000000FF)) << 8) +
                      ((part5 & UINT64_C(0x0000FF0000000000)) >> 24) +
                      ((part5 & UINT64_C(0x000000FF00000000)) >> 8) +
                      ((part5 & UINT64_C(0x00000000FF000000)) << 8) +
                      ((part5 & UINT64_C(0x0000000000FF0000)) << 24) +
                      ((part5 & UINT64_C(0x000000000000FF00)) << 40) +
                      ((part5 & UINT64_C(0x00000000000000FF)) << 56);
      entered = 1;
   } // if

   // If neither of the above methods of entry was used, use prompted
   // entry....
   if (entered == 0) {
      sscanf(temp, "%llx", &part1);
      cout << "Enter a two-byte hexadecimal number for the second segment: ";
      junk = fgets(temp, 255, stdin);
      sscanf(temp, "%llx", &part2);
      cout << "Enter a two-byte hexadecimal number for the third segment: ";
      junk = fgets(temp, 255, stdin);
      sscanf(temp, "%llx", &part3);
      theGUID.data1 = (part3 << 48) + (part2 << 32) + part1;
      cout << "Enter a two-byte hexadecimal number for the fourth segment: ";
      junk = fgets(temp, 255, stdin);
      sscanf(temp, "%llx", &part4);
      cout << "Enter a six-byte hexadecimal number for the fifth segment: ";
      junk = fgets(temp, 255, stdin);
      sscanf(temp, "%llx", &part5);
      theGUID.data2 = ((part4 & UINT64_C(0x000000000000FF00)) >> 8) +
                      ((part4 & UINT64_C(0x00000000000000FF)) << 8) +
                      ((part5 & UINT64_C(0x0000FF0000000000)) >> 24) +
                      ((part5 & UINT64_C(0x000000FF00000000)) >> 8) +
                      ((part5 & UINT64_C(0x00000000FF000000)) << 8) +
                      ((part5 & UINT64_C(0x0000000000FF0000)) << 24) +
                      ((part5 & UINT64_C(0x000000000000FF00)) << 40) +
                      ((part5 & UINT64_C(0x00000000000000FF)) << 56);
      entered = 1;
   } // if/else
   cout << "New GUID: " << GUIDToStr(theGUID) << "\n";
   return theGUID;
} // GetGUID()

// Return 1 if the CPU architecture is little endian, 0 if it's big endian....
int IsLittleEndian(void) {
   int littleE = 1; // assume little-endian (Intel-style)
   union {
      uint32_t num;
      unsigned char uc[sizeof(uint32_t)];
   } endian;

   endian.num = 1;
   if (endian.uc[0] != (unsigned char) 1) {
      littleE = 0;
   } // if
   return (littleE);
} // IsLittleEndian()

// Reverse the byte order of theValue; numBytes is number of bytes
void ReverseBytes(void* theValue, int numBytes) {
   char* tempValue = NULL;
   int i;

   tempValue = (char*) malloc(numBytes);
   if (tempValue != NULL) {
      memcpy(tempValue, theValue, numBytes);
      for (i = 0; i < numBytes; i++)
         ((char*) theValue)[i] = tempValue[numBytes - i - 1];
      free(tempValue);
   } // if
} // ReverseBytes()

// Compute (2 ^ value). Given the return type, value must be 63 or less.
// Used in some bit-fiddling functions
uint64_t PowerOf2(int value) {
   uint64_t retval = 1;
   int i;

   if ((value < 64) && (value >= 0)) {
      for (i = 0; i < value; i++) {
         retval *= 2;
      } // for
   } else retval = 0;
   return retval;
} // PowerOf2()
