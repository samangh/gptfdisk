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
      do {
         cout << prompt;
         cin.getline(line, 255);
         num = sscanf(line, "%d", &response);
         if (num == 1) { // user provided a response
            if ((response < low) || (response > high))
               cout << "Value out of range\n";
         } else { // user hit enter; return default
            response = def;
         } // if/else
      } while ((response < low) || (response > high));
   } else { // low == high, so return this value
      cout << "Using " << low << "\n";
      response = low;
   } // else
   return (response);
} // GetNumber()

// Gets a Y/N response (and converts lowercase to uppercase)
char GetYN(void) {
   char line[255];
   char response;

   do {
      cout << "(Y/N): ";
      if (!fgets(line, 255, stdin)) {
         cerr << "Critical error! Failed fgets() in GetYN()\n";
         exit(1);
      } // if
      sscanf(line, "%c", &response);
      if (response == 'y')
         response = 'Y';
      if (response == 'n')
         response = 'N';
   } while ((response != 'Y') && (response != 'N'));
   return response;
} // GetYN(void)

// Obtains a sector number, between low and high, from the
// user, accepting values prefixed by "+" to add sectors to low,
// or the same with "K", "M", "G", "T", or "P" as suffixes to add
// kilobytes, megabytes, gigabytes, terabytes, or petabytes,
// respectively. If a "-" prefix is used, use the high value minus
// the user-specified number of sectors (or KiB, MiB, etc.). Use the
// def value as the default if the user just hits Enter. The sSize is
// the sector size of the device.
uint64_t GetSectorNum(uint64_t low, uint64_t high, uint64_t def, uint64_t sSize,
                      const string & prompt) {
   uint64_t response;
   char line[255];

   do {
      cout << prompt;
      cin.getline(line, 255);
      response = SIToInt(line, sSize, low, high, def);
   } while ((response < low) || (response > high));
   return response;
} // GetSectorNum()

// Convert an SI value (K, M, G, T, or P) to its equivalent in
// number of sectors. If no units are appended, interprets as the number
// of sectors; otherwise, interprets as number of specified units and
// converts to sectors. For instance, with 512-byte sectors, "1K" converts
// to 2. If value includes a "+", adds low and subtracts 1; if SIValue
// inclues a "-", subtracts from high. If SIValue is empty, returns def.
// Returns integral sector value.
uint64_t SIToInt(string SIValue, uint64_t sSize, uint64_t low, uint64_t high, uint64_t def) {
   int plusFlag = 0, badInput = 0;
   uint64_t response = def, mult = 1, divide = 1;
   char suffix;

   if (sSize == 0) {
      sSize = SECTOR_SIZE;
      cerr << "Bug: Sector size invalid in SIToInt()!\n";
   } // if

   // Remove leading spaces, if present
   while (SIValue[0] == ' ')
      SIValue.erase(0, 1);

   // If present, flag and remove leading plus sign
   if (SIValue[0] == '+') {
      plusFlag = 1;
      SIValue.erase(0, 1);
   } // if

   // If present, flag and remove leading minus sign
   if (SIValue[0] == '-') {
      plusFlag = -1;
      SIValue.erase(0, 1);
   } // if

   // Extract numeric response and, if present, suffix
   istringstream inString(SIValue);
   if (((inString.peek() < '0') || (inString.peek() > '9')) && (inString.peek() != -1))
      badInput = 1;
   inString >> response >> suffix;

   // If no response, or if response == 0, use default (def)
   if ((SIValue.length() == 0) || (response == 0)) {
      response = def;
      suffix = ' ';
      plusFlag = 0;
   } // if

    // Set multiplier based on suffix
   switch (suffix) {
      case 'K':
      case 'k':
         mult = UINT64_C(1024) / sSize;
         divide = sSize / UINT64_C(1024);
         break;
      case 'M':
      case 'm':
         mult = UINT64_C(1048576) / sSize;
         divide = sSize / UINT64_C(1048576);
         break;
      case 'G':
      case 'g':
         mult = UINT64_C(1073741824) / sSize;
         break;
      case 'T':
      case 't':
         mult = UINT64_C(1099511627776) / sSize;
         break;
      case 'P':
      case 'p':
         mult = UINT64_C(1125899906842624) / sSize;
         break;
      default:
         mult = 1;
   } // switch

   // Adjust response based on multiplier and plus flag, if present
   if (mult > 1)
      response *= mult;
   else if (divide > 1)
      response /= divide;
   if (plusFlag == 1) {
      // Recompute response based on low part of range (if default = high
      // value, which should be the case when prompting for the end of a
      // range) or the defaut value (if default != high, which should be
      // the case for the first sector of a partition).
      if (def == high)
         response = response + low - UINT64_C(1);
      else
         response = response + def;
   } // if
   if (plusFlag == -1) {
      response = high - response;
   } // if

   if (badInput)
      response = high + UINT64_C(1);

   return response;
} // SIToInt()

// Takes a size and converts this to a size in SI units (KiB, MiB, GiB,
// TiB, or PiB), returned in C++ string form. The size is either in units
// of the sector size or, if that parameter is omitted, in bytes.
// (sectorSize defaults to 1).
string BytesToSI(uint64_t size, uint32_t sectorSize) {
   string units;
   ostringstream theValue;
   float sizeInSI;

   sizeInSI = (float) size * (float) sectorSize;
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

// Returns 1 if input can be interpreted as a hexadecimal number --
// all characters must be spaces, digits, or letters A-F (upper- or
// lower-case), with at least one valid hexadecimal digit; otherwise
// returns 0.
int IsHex(const string & input) {
   int isHex = 1, foundHex = 0, i;

   for (i = 0; i < (int) input.length(); i++) {
      if ((input[i] < '0') || (input[i] > '9')) {
         if ((input[i] < 'A') || (input[i] > 'F')) {
            if ((input[i] < 'a') || (input[i] > 'f')) {
               if ((input[i] != ' ') && (input[i] != '\n')) {
                  isHex = 0;
               }
            } else foundHex = 1;
         } else foundHex = 1;
      } else foundHex = 1;
   } // for
   if (!foundHex)
      isHex = 0;
   return isHex;
} // IsHex()

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
   uint64_t retval;

   istringstream inString(GetString(argument, itemNum));
   inString >> retval;
   return retval;
} // GetInt()

// Extract string data from argument string, which should be colon-delimited
string GetString(const string & argument, int itemNum) {
   size_t startPos = -1, endPos = -1;

   while (itemNum-- > 0) {
      startPos = endPos + 1;
      endPos = argument.find(':', startPos);
   }
   if (endPos == string::npos)
      endPos = argument.length();
   endPos--;

   return argument.substr(startPos, endPos - startPos + 1);
} // GetString()