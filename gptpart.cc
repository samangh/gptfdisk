//
// C++ Implementation: gptpart
//
// Description: Class to implement a SINGLE GPT partition
//
//
// Author: Rod Smith <rodsmith@rodsbooks.com>, (C) 2009-2011
//
// Copyright: See COPYING file that comes with this distribution
//
//
// This program is copyright (c) 2009 by Roderick W. Smith. It is distributed
// under the terms of the GNU GPL version 2, as detailed in the COPYING file.

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#ifdef USE_UTF16
#include <unicode/ustdio.h>
#else
#define UnicodeString string
#endif

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

// Return a Unicode description of the partition type (e.g., "Linux/Windows
// data" or "Linux swap").
UnicodeString GPTPart::GetUTypeName(void) {
   return partitionType.UTypeName();
} // GPTPart::GetNameType()

// Compute and return the partition's length (or 0 if the end is incorrectly
// set before the beginning).
uint64_t GPTPart::GetLengthLBA(void) const {
   uint64_t length = 0;

   if (firstLBA <= lastLBA)
      length = lastLBA - firstLBA + UINT64_C(1);
   return length;
} // GPTPart::GetLengthLBA()

#ifdef USE_UTF16
// Return partition's name field, converted to a Unicode string
UnicodeString GPTPart::GetDescription(void) {
   return (UChar*) name;
} // GPTPart::GetDescription()
#else
// Return partition's name field, converted to a C++ ASCII string
string GPTPart::GetDescription(void) {
   string theName;
   int i = 0;

   theName = "";
   while ((i < NAME_SIZE) && (name[i] != '\0')) {
      theName += name[i];
      i+=2;
   } // while
   return theName;
} // GPTPart::GetDescription() (Windows version)
#endif

// Return 1 if the partition is in use
int GPTPart::IsUsed(void) {
   return (partitionType != GUIDData("0x00"));
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

#ifdef USE_UTF16
// Set the name for a partition to theName, using a C++-style string as
// input.
void GPTPart::SetName(const string & theName) {
   SetName((UnicodeString) theName.c_str());
} // GPTPart::SetName()

// Set the name for a partition to theName, using a Unicode string as
// input.
void GPTPart::SetName(const UnicodeString & theName) {
   if (theName.isBogus()) {
      cerr << "Bogus UTF-16 name found in GPTPart::SetName()! Name not changed!\n";
   } else {
      memset(name, 0, NAME_SIZE);
      theName.extractBetween(0, NAME_SIZE / 2 - 1, (UChar*) name);
   } // if/else
} // GPTPart::SetName()

#else

// Set the name for a partition to theName. Note that theName is a
// standard C++-style ASCII string, although the GUID partition definition
// requires a UTF-16LE string. This function creates a simple-minded copy
// for this.
void GPTPart::SetName(const string & theName) {
   int i, length;

   if (theName.length() < (NAME_SIZE / 2))
      length = theName.length();
   else
      length = NAME_SIZE / 2;
   memset(name, 0, NAME_SIZE);
   for (i = 0; i < length; i++)
      name[i * 2] = theName[i];
} // GPTPart::SetName(), ASCII version
#endif

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
#ifdef USE_UTF16
      GetDescription().extractBetween(0, 23, description);
      cout << description << "\n";
#else
      cout << GetDescription().substr(0, 23) << "\n";
#endif
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
      cout << "Partition name: '" << GetDescription() << "'\n";
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
      cout << "Hex code or GUID (L to show codes, Enter = " << hex << DEFAULT_GPT_TYPE << dec << "): ";
      line = ReadString();
      if ((line[0] == 'L') || (line[0] == 'l')) {
         partitionType.ShowAllTypes();
      } else {
         if (line.length() == 0)
            tempType = DEFAULT_GPT_TYPE;
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
