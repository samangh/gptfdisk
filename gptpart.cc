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

GPTPart::GPTPart(void) {
   partitionType.Zero();
   uniqueGUID.Zero();
   firstLBA = 0;
   lastLBA = 0;
   attributes = 0;
   memset(name, 0, NAME_SIZE);
} // Default constructor

GPTPart::~GPTPart(void) {
} // destructor

// Return the gdisk-specific two-byte hex code for the partition
uint16_t GPTPart::GetHexType(void) const {
   return partitionType.GetHexType();
} // GPTPart::GetHexType()

// Return a plain-text description of the partition type (e.g., "Linux/Windows
// data" or "Linux swap").
string GPTPart::GetTypeName(void) {
   return partitionType.TypeName();
} // GPTPart::GetNameType()

// Compute and return the partition's length (or 0 if the end is incorrectly
// set before the beginning).
uint64_t GPTPart::GetLengthLBA(void) const {
   uint64_t length = 0;

   if (firstLBA <= lastLBA)
      length = lastLBA - firstLBA + UINT64_C(1);
   return length;
} // GPTPart::GetLengthLBA()

// Return partition's name field, converted to a C++ ASCII string
string GPTPart::GetDescription(void) {
   string theName;
   int i;

   theName = "";
   for (i = 0; i < NAME_SIZE; i += 2) {
      if (name[i] != '\0')
         theName += name[i];
   } // for
   return theName;
} // GPTPart::GetDescription()

// Return 1 if the partition is in use
int GPTPart::IsUsed(void) {
   return (firstLBA != UINT64_C(0));
} // GPTPart::IsUsed()

// Set the type code to the specified one. Also changes the partition
// name *IF* the current name is the generic one for the current partition
// type.
void GPTPart::SetType(PartType t) {
   if (GetDescription() == partitionType.TypeName()) {
      SetName(t.TypeName());
   } // if
   partitionType = t;
} // GPTPart::SetType()

// Set the name for a partition to theName, or prompt for a name if
// theName is empty. Note that theName is a standard C++-style ASCII
// string, although the GUID partition definition requires a UTF-16LE
// string. This function creates a simple-minded copy for this.
void GPTPart::SetName(const string & theName) {
   char newName[NAME_SIZE];
   size_t i;

   // Blank out new name string, so that it will terminate in a null
   // when data are copied to it....
   memset(newName, 0, NAME_SIZE);

   if (theName == "") { // No name specified, so get one from the user
      cout << "Enter name: ";
      ReadCString(newName, NAME_SIZE / 2 + 1);

      // Input is likely to include a newline, so remove it....
      i = strlen(newName);
      if (i && newName[i - 1] == '\n')
         newName[i - 1] = '\0';
   } else {
      strcpy(newName, theName.substr(0, NAME_SIZE / 2).c_str());
   } // if

   // Copy the C-style ASCII string from newName into a form that the GPT
   // table will accept....
   memset(name, 0, NAME_SIZE);
   for (i = 0; i < NAME_SIZE / 2; i++)
      name[i * 2] = newName[i];
} // GPTPart::SetName()

// Set the name for the partition based on the current GUID partition type
// code's associated name
void GPTPart::SetDefaultDescription(void) {
   SetName(partitionType.TypeName());
} // GPTPart::SetDefaultDescription()

GPTPart & GPTPart::operator=(const GPTPart & orig) {
   partitionType = orig.partitionType;
   uniqueGUID = orig.uniqueGUID;
   firstLBA = orig.firstLBA;
   lastLBA = orig.lastLBA;
   attributes = orig.attributes;
   memcpy(name, orig.name, NAME_SIZE);
   return *this;
} // assignment operator

// Compare the values, and return a bool result.
// Because this is intended for sorting and a firstLBA value of 0 denotes
// a partition that's not in use and so that should be sorted upwards,
// we return the opposite of the usual arithmetic result when either
// firstLBA value is 0.
bool GPTPart::operator<(const GPTPart &other) const {
   
   if (firstLBA && other.firstLBA)
      return (firstLBA < other.firstLBA);
   else
      return (other.firstLBA < firstLBA);
} // GPTPart::operator<()

// Display summary information; does nothing if the partition is empty.
void GPTPart::ShowSummary(int partNum, uint32_t blockSize) {
   string sizeInSI;
   size_t i;

   if (firstLBA != 0) {
      sizeInSI = BytesToSI(lastLBA - firstLBA + 1, blockSize);
      cout.fill(' ');
      cout.width(4);
      cout << partNum + 1 << "  ";
      cout.width(14);
      cout << firstLBA << "  ";
      cout.width(14);
      cout << lastLBA  << "   ";
      cout << BytesToSI(lastLBA - firstLBA + 1, blockSize) << "  ";
      for (i = 0; i < 10 - sizeInSI.length(); i++)
         cout << " ";
      cout.fill('0');
      cout.width(4);
      cout.setf(ios::uppercase);
      cout << hex << partitionType.GetHexType() << "  " << dec;
      cout.fill(' ');
      cout << GetDescription().substr(0, 23) << "\n";
      cout.fill(' ');
   } // if
} // GPTPart::ShowSummary()

// Show detailed partition information. Does nothing if the partition is
// empty (as determined by firstLBA being 0).
void GPTPart::ShowDetails(uint32_t blockSize) {
   uint64_t size;

   if (firstLBA != 0) {
      cout << "Partition GUID code: " << partitionType;
      cout << " (" << partitionType.TypeName() << ")\n";
      cout << "Partition unique GUID: " << uniqueGUID << "\n";

      cout << "First sector: " << firstLBA << " (at "
            << BytesToSI(firstLBA, blockSize) << ")\n";
      cout << "Last sector: " << lastLBA << " (at "
            << BytesToSI(lastLBA, blockSize) << ")\n";
      size = (lastLBA - firstLBA + 1);
      cout << "Partition size: " << size << " sectors ("
            << BytesToSI(size, blockSize) << ")\n";
      cout << "Attribute flags: ";
      cout.fill('0');
      cout.width(16);
      cout << hex;
      cout << attributes << "\n";
      cout << dec;
      cout << "Partition name: " << GetDescription() << "\n";
      cout.fill(' ');
   }  // if
} // GPTPart::ShowDetails()

// Blank (delete) a single partition
void GPTPart::BlankPartition(void) {
   uniqueGUID.Zero();
   partitionType.Zero();
   firstLBA = 0;
   lastLBA = 0;
   attributes = 0;
   memset(name, 0, NAME_SIZE);
} // GPTPart::BlankPartition

// Returns 1 if the two partitions overlap, 0 if they don't
int GPTPart::DoTheyOverlap(const GPTPart & other) {
   // Don't bother checking unless these are defined (both start and end points
   // are 0 for undefined partitions, so just check the start points)
   return firstLBA && other.firstLBA &&
          (firstLBA <= other.lastLBA) != (lastLBA < other.firstLBA);
} // GPTPart::DoTheyOverlap()

// Reverse the bytes of integral data types; used on big-endian systems.
void GPTPart::ReversePartBytes(void) {
   ReverseBytes(&firstLBA, 8);
   ReverseBytes(&lastLBA, 8);
   ReverseBytes(&attributes, 8);
} // GPTPart::ReverseBytes()

/****************************************
 * Functions requiring user interaction *
 ****************************************/

// Change the type code on the partition. Also changes the name if the original
// name is the generic one for the partition type.
void GPTPart::ChangeType(void) {
   char line[255];
   int changeName;
   PartType tempType = (GUIDData) "00000000-0000-0000-0000-000000000000";

   changeName = (GetDescription() == GetTypeName());

   cout << "Current type is '" << GetTypeName() << "'\n";
   do {
      cout << "Hex code or GUID (L to show codes, Enter = 0700): ";
      ReadCString(line, sizeof(line));
      if ((line[0] == 'L') || (line[0] == 'l')) {
         partitionType.ShowAllTypes();
      } else {
         if (strlen(line) == 1)
            tempType = 0x0700;
         else
            tempType = line;
      } // if/else
   } while (tempType == (GUIDData) "00000000-0000-0000-0000-000000000000");
   partitionType = tempType;
   cout << "Changed type of partition to '" << partitionType.TypeName() << "'\n";
   if (changeName) {
      SetDefaultDescription();
   } // if
} // GPTPart::ChangeType()
