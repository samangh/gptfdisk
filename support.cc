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
#include <sstream>
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
   uint64_t response, mult = 1;
   int plusFlag = 0;
   char suffix, line[255];

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
      istringstream inString(line);
      inString >> response >> suffix;

      // If no response, use default (def)
      if (strlen(line) == 0) {
         response = def;
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
      response *= mult;
      if (plusFlag == 1) {
         // Recompute response based on low part of range (if default = high
         // value, which should be the case when prompting for the end of a
         // range) or the defaut value (if default != high, which should be
         // the case for the first sector of a partition).
         if (def == high)
            response = response + low - UINT64_C(1);
         else
            response = response + def - UINT64_C(1);
      } // if
      if (plusFlag == -1) {
         response = high - response;
      } // if
   } // while
   return response;
} // GetSectorNum()

// Takes a size in bytes (in size) and converts this to a size in
// SI units (KiB, MiB, GiB, TiB, or PiB), returned in C++ string
// form
string BytesToSI(uint64_t size) {
   string units;
   ostringstream theValue;
   float sizeInSI;

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
   theValue.setf(ios::fixed);
   if (units == " bytes") { // in bytes, so no decimal point
      theValue.precision(0);
   } else {
      theValue.precision(1);
   } // if/else
   theValue << sizeInSI << units;
   return theValue.str();
} // BlocksToSI()

// Converts two consecutive characters in the input string into a
// number, interpreting the string as a hexadecimal number, starting
// at the specified position.
unsigned char StrToHex(const string & input, unsigned int position) {
   unsigned char retval = 0x00;
   unsigned int temp;

   if (input.length() >= (position + 2)) {
      sscanf(input.substr(position, 2).c_str(), "%x", &temp);
      retval = (unsigned char) temp;
   } // if
   return retval;
} // StrToHex()

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

   tempValue = new char [numBytes];
   if (tempValue != NULL) {
      memcpy(tempValue, theValue, numBytes);
      for (i = 0; i < numBytes; i++)
         ((char*) theValue)[i] = tempValue[numBytes - i - 1];
      delete[] tempValue;
   } // if
} // ReverseBytes()

// Extract integer data from argument string, which should be colon-delimited
uint64_t GetInt(const string & argument, int itemNum) {
   int startPos = -1, endPos = -1;
   uint64_t retval = 0;

   while (itemNum-- > 0) {
      startPos = endPos + 1;
      endPos = argument.find(':', startPos);
   }
   if (endPos == (int) string::npos)
      endPos = argument.length();
   endPos--;

   istringstream inString(argument.substr(startPos, endPos - startPos + 1));
   inString >> retval;
   return retval;
} // GetInt()

// Extract string data from argument string, which should be colon-delimited
string GetString(const string & argument, int itemNum) {
   int startPos = -1, endPos = -1;

   while (itemNum-- > 0) {
      startPos = endPos + 1;
      endPos = argument.find(':', startPos);
   }
   if (endPos == (int) string::npos)
      endPos = argument.length();
   endPos--;

   return argument.substr(startPos, endPos - startPos + 1);
} // GetString()