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
#include <unicode/ustdio.h>
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

// Return a Unicode description of the partition type (e.g., "Linux/Windows
// data" or "Linux swap").
UnicodeString GPTPart::GetUTypeName(void) {
   return partitionType.UTypeName();
/*   UnicodeString temp;

   temp = temp.fromUTF8(partitionType.TypeName());
   return temp; */
} // GPTPart::GetNameType()

// Compute and return the partition's length (or 0 if the end is incorrectly
// set before the beginning).
uint64_t GPTPart::GetLengthLBA(void) const {
   uint64_t length = 0;

   if (firstLBA <= lastLBA)
      length = lastLBA - firstLBA + UINT64_C(1);
   return length;
} // GPTPart::GetLengthLBA()

/* // Return partition's name field, converted to a C++ ASCII string
string GPTPart::GetDescription(void) {
   string theName;
   int i;

   theName = "";
   for (i = 0; i < NAME_SIZE; i += 2) {
      if (name[i] != '\0')
         theName += name[i];
   } // for
   return theName;
} // GPTPart::GetDescription() */

UnicodeString GPTPart::GetDescription(void) {
   UnicodeString theName;
   UChar *temp;
   int i;

   theName = "";
   temp = (UChar*) name;
   for (i = 0; i < NAME_SIZE / 2; i++) {
      if (temp[i] != '\0')
         theName += temp[i];
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
   if (GetDescription() == partitionType.UTypeName()) {
      SetName(t.TypeName());
   } // if
   partitionType = t;
} // GPTPart::SetType()

// Set the name for a partition to theName, or prompt for a name if
// theName is empty, using a C++-style string as input.
void GPTPart::SetName(string theName) {
   UnicodeString uString;

   uString = theName.c_str();
   SetName(uString);
} // GPTPart::SetName()

// Set the name for a partition to theName, or prompt for a name
// if theName is empty, using a Unicode string as input.
void GPTPart::SetName(UnicodeString theName) {
   int i;
   UChar temp[NAME_SIZE / 2];

   if (theName == "") { // No name specified, so get one from the user
      cout << "Enter name: ";
      theName = ReadUString();
   } // if

   // Copy the C++-style string from newName into a form that the GPT
   // table will accept....
   memset(temp, 0, NAME_SIZE);
   for (i = 0; i < theName.length(); i++)
      temp[i] = theName[i];
   memcpy(name, temp, NAME_SIZE);
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
   string sizeInIeee;
   UnicodeString description;
   size_t i;

   if (firstLBA != 0) {
      sizeInIeee = BytesToIeee(lastLBA - firstLBA + 1, blockSize);
      cout.fill(' ');
      cout.width(4);
      cout << partNum + 1 << "  ";
      cout.width(14);
      cout << firstLBA << "  ";
      cout.width(14);
      cout << lastLBA  << "   ";
      cout << BytesToIeee(lastLBA - firstLBA + 1, blockSize) << "  ";
      if (sizeInIeee.length() < 10)
         for (i = 0; i < 10 - sizeInIeee.length(); i++)
            cout << " ";
      cout.fill('0');
      cout.width(4);
      cout.setf(ios::uppercase);
      cout << hex << partitionType.GetHexType() << "  " << dec;
      cout.fill(' ');
//      description = GetDescription();
      GetDescription().extractBetween(0, 24, description);
      cout << description << "\n";
//      for (i = 0; i < 23; i++)
//         cout << (char) description.;
//      cout << GetDescription().tempSubString(0, 23) << "\n";
//      cout << GetDescription().substr(0, 23) << "\n";
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
            << BytesToIeee(firstLBA, blockSize) << ")\n";
      cout << "Last sector: " << lastLBA << " (at "
            << BytesToIeee(lastLBA, blockSize) << ")\n";
      size = (lastLBA - firstLBA + 1);
      cout << "Partition size: " << size << " sectors ("
            << BytesToIeee(size, blockSize) << ")\n";
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

// Reverse the bytes of integral data types and of the UTF-16LE name;
// used on big-endian systems.
void GPTPart::ReversePartBytes(void) {
   int i;

   ReverseBytes(&firstLBA, 8);
   ReverseBytes(&lastLBA, 8);
   ReverseBytes(&attributes, 8);
   for (i = 0; i < NAME_SIZE; i += 2)
      ReverseBytes(name + i, 2);
} // GPTPart::ReverseBytes()

/****************************************
 * Functions requiring user interaction *
 ****************************************/

// Change the type code on the partition. Also changes the name if the original
// name is the generic one for the partition type.
void GPTPart::ChangeType(void) {
   string line;
   int changeName;
   PartType tempType = (GUIDData) "00000000-0000-0000-0000-000000000000";

   changeName = (GetDescription() == GetUTypeName());

   cout << "Current type is '" << GetTypeName() << "'\n";
   do {
      cout << "Hex code or GUID (L to show codes, Enter = 0700): ";
      line = ReadString();
      if ((line[0] == 'L') || (line[0] == 'l')) {
         partitionType.ShowAllTypes();
      } else {
         if (line.length() == 0)
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
