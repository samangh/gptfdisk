//
// C++ Implementation: gptpart
//
// Description: Class to implement a SINGLE GPT partition
//
//
// Author: Rod Smith <rodsmith@rodsbooks.com>, (C) 2009
//
// Copyright: See COPYING file that comes with this distribution
//
//
// This program is copyright (c) 2009 by Roderick W. Smith. It is distributed
// under the terms of the GNU GPL version 2, as detailed in the COPYING file.

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include <string.h>
#include <stdio.h>
#include <iostream>
#include "gptpart.h"
#include "attributes.h"

using namespace std;

PartTypes GPTPart::typeHelper;

GPTPart::GPTPart(void) {
   int i;

   for (i = 0; i < NAME_SIZE; i++)
      name[i] = '\0';
} // Default constructor

GPTPart::~GPTPart(void) {
} // destructor

// Return partition's name field, converted to a C++ ASCII string
string GPTPart::GetName(void) {
   string theName;
   int i;

   for (i = 0; i < NAME_SIZE; i += 2) {
      theName += name[i];
   } // for
   return theName;
} // GPTPart::GetName()

// Return the gdisk-specific two-byte hex code for the partition
uint16_t GPTPart::GetHexType(void) {
   return typeHelper.GUIDToID(partitionType);
} // GPTPart::GetHexType()

// Return a plain-text description of the partition type (e.g., "Linux/Windows
// data" or "Linux swap").
string GPTPart::GetNameType(void) {
   return typeHelper.GUIDToName(partitionType);
} // GPTPart::GetNameType()

// Compute and return the partition's length (or 0 if the end is incorrectly
// set before the beginning).
uint64_t GPTPart::GetLengthLBA(void) {
   uint64_t length = 0;
   if (firstLBA <= lastLBA)
      length = lastLBA - firstLBA + UINT64_C(1);
   return length;
} // GPTPart::GetLengthLBA()

GPTPart & GPTPart::operator=(const GPTPart & orig) {
   int i;

   partitionType = orig.partitionType;
   uniqueGUID = orig.uniqueGUID;
   firstLBA = orig.firstLBA;
   lastLBA = orig.lastLBA;
   attributes = orig.attributes;
   for (i = 0; i < NAME_SIZE; i++)
      name[i] = orig.name[i];
   return *this;
} // assignment operator

// Sets the unique GUID to a value of 0 or a random value,
// depending on the parameter: 0 = 0, anything else = random
void GPTPart::SetUniqueGUID(int zeroOrRandom) {
   if (zeroOrRandom == 0) {
      uniqueGUID.data1 = 0;
      uniqueGUID.data2 = 0;
   } else {
      // rand() is only 32 bits on 32-bit systems, so multiply together to
      // fill a 64-bit value.
      uniqueGUID.data1 = (uint64_t) rand() * (uint64_t) rand();
      uniqueGUID.data2 = (uint64_t) rand() * (uint64_t) rand();
   }
} // GPTPart::SetUniqueGUID()

// Blank (delete) a single partition
void GPTPart::BlankPartition(void) {
   int j;
   GUIDData zeroGUID;

   zeroGUID.data1 = 0;
   zeroGUID.data2 = 0;
   uniqueGUID = zeroGUID;
   partitionType = zeroGUID;
   firstLBA = 0;
   lastLBA = 0;
   attributes = 0;
   for (j = 0; j < NAME_SIZE; j++)
      name[j] = '\0';
} // GPTPart::BlankPartition

// Returns 1 if the two partitions overlap, 0 if they don't
int GPTPart::DoTheyOverlap(GPTPart* other) {
   int theyDo = 0;

   // Don't bother checking unless these are defined (both start and end points
   // are 0 for undefined partitions, so just check the start points)
   if ((firstLBA != 0) && (other->firstLBA != 0)) {
      if ((firstLBA < other->lastLBA) && (lastLBA >= other->firstLBA))
         theyDo = 1;
      if ((other->firstLBA < lastLBA) && (other->lastLBA >= firstLBA))
         theyDo = 1;
   } // if
   return (theyDo);
} // GPTPart::DoTheyOverlap()

// Reverse the bytes of integral data types; used on big-endian systems.
void GPTPart::ReversePartBytes(void) {
   ReverseBytes(&partitionType.data1, 8);
   ReverseBytes(&partitionType.data2, 8);
   ReverseBytes(&uniqueGUID.data1, 8);
   ReverseBytes(&uniqueGUID.data2, 8);
   ReverseBytes(&firstLBA, 8);
   ReverseBytes(&lastLBA, 8);
   ReverseBytes(&attributes, 8);
} // GPTPart::ReverseBytes()

// Display summary information; does nothing if the partition is empty.
void GPTPart::ShowSummary(int partNum, uint32_t blockSize) {
   string sizeInSI;
   int i;

   if (firstLBA != 0) {
      sizeInSI = BytesToSI(blockSize * (lastLBA - firstLBA + 1));
      cout.width(4);
      cout << partNum + 1 << "  ";
      cout.width(14);
      cout << firstLBA << "  ";
      cout.width(14);
      cout << lastLBA  << "   ";
      cout << BytesToSI(blockSize * (lastLBA - firstLBA + 1)) << "   ";
      for (i = 0; i < 9 - sizeInSI.length(); i++) cout << " ";
      cout.fill('0');
      cout.width(4);
      cout.setf(ios::uppercase);
      cout << hex << typeHelper.GUIDToID(partitionType) << "  " << dec;
      cout.fill(' ');
      cout.setf(ios::right);
      cout << GetName().substr(0, 23) << "\n";
      cout.fill(' ');
   } // if
} // GPTPart::ShowSummary()

// Show detailed partition information. Does nothing if the partition is
// empty (as determined by firstLBA being 0).
void GPTPart::ShowDetails(uint32_t blockSize) {
   uint64_t size;

   if (firstLBA != 0) {
      cout << "Partition GUID code: " << GUIDToStr(partitionType);
      cout << " (" << typeHelper.GUIDToName(partitionType) << ")\n";
      cout << "Partition unique GUID: " << GUIDToStr(uniqueGUID) << "\n";

      cout << "First sector: " << firstLBA << " (at "
           << BytesToSI(firstLBA * blockSize) << ")\n";
      cout << "Last sector: " << lastLBA << " (at "
           << BytesToSI(lastLBA * blockSize) << ")\n";
      size = (lastLBA - firstLBA + 1);
      cout << "Partition size: " << size << " sectors ("
           << BytesToSI(size * ((uint64_t) blockSize)) << ")\n";
      cout << "Attribute flags: ";
      cout.fill('0');
      cout.width(16);
      cout << right;
      cout << hex;
      cout << attributes << "\n";
      cout << left;
      cout << dec;
      cout << "Partition name: " << GetName() << "\n";
   }  // if
} // GPTPart::ShowDetails()

/****************************************
 * Functions requiring user interaction *
 ****************************************/

// Change the type code on the partition.
void GPTPart::ChangeType(void) {
   char line[255];
   char* junk;
   int typeNum = 0xFFFF;
   GUIDData newType;

   cout << "Current type is '" << GetNameType() << "'\n";
   while ((!typeHelper.Valid(typeNum)) && (typeNum != 0)) {
      cout << "Hex code (L to show codes, 0 to enter raw code, Enter = 0700): ";
      junk = fgets(line, 255, stdin);
      sscanf(line, "%X", &typeNum);
      if ((line[0] == 'L') || (line[0] == 'l'))
         typeHelper.ShowTypes();
      if (line[0] == '\n') {
         typeNum = 0x0700;
      } // if
   } // while
   if (typeNum != 0) // user entered a code, so convert it
      newType = typeHelper.IDToGUID((uint16_t) typeNum);
   else // user wants to enter the GUID directly, so do that
      newType = GetGUID();
   partitionType = newType;
   cout << "Changed type of partition to '" << typeHelper.GUIDToName(partitionType) << "'\n";
} // GPTPart::ChangeType()

// Set the name for a partition to theName, or prompt for a name if
// theName is empty. Note that theName is a standard C++-style ASCII
// string, although the GUID partition definition requires a UTF-16LE
// string. This function creates a simple-minded copy for this.
void GPTPart::SetName(string theName) {
   char newName[NAME_SIZE]; // New name
   char *junk;
   int i;

   // Blank out new name string, just to be on the safe side....
   for (i = 0; i < NAME_SIZE; i++)
      newName[i] = '\0';

   if (theName == "") { // No name specified, so get one from the user
      cout << "Enter name: ";
      junk = fgets(newName, NAME_SIZE / 2, stdin);

      // Input is likely to include a newline, so remove it....
      i = strlen(newName);
      if (newName[i - 1] == '\n')
         newName[i - 1] = '\0';
   } else {
      strcpy(newName, theName.substr(0, NAME_SIZE / 2).c_str());
   } // if

   // Copy the C-style ASCII string from newName into a form that the GPT
   // table will accept....
   for (i = 0; i < NAME_SIZE; i++) {
      if ((i % 2) == 0) {
         name[i] = newName[(i / 2)];
      } else {
         name[i] = '\0';
      } // if/else
   } // for
} // GPTPart::SetName()

/***********************************
 * Non-class but related functions *
 ***********************************/

// Recursive quick sort algorithm for GPT partitions. Note that if there
// are any empties in the specified range, they'll be sorted to the
// start, resulting in a sorted set of partitions that begins with
// partition 2, 3, or higher.
void QuickSortGPT(GPTPart* partitions, int start, int finish) {
   uint64_t starterValue; // starting location of median partition
   int left, right;
   GPTPart temp;

   left = start;
   right = finish;
   starterValue = partitions[(start + finish) / 2].GetFirstLBA();
   do {
      while (partitions[left].GetFirstLBA() < starterValue)
         left++;
      while (partitions[right].GetFirstLBA() > starterValue)
         right--;
      if (left <= right) {
         temp = partitions[left];
         partitions[left] = partitions[right];
         partitions[right] = temp;
         left++;
         right--;
      } // if
   } while (left <= right);
   if (start < right) QuickSortGPT(partitions, start, right);
   if (finish > left) QuickSortGPT(partitions, left, finish);
} // QuickSortGPT()
