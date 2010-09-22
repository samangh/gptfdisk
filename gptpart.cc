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
   int i;

   partitionType.Zero();
   uniqueGUID.Zero();
   firstLBA = 0;
   lastLBA = 0;
   attributes = 0;

   for (i = 0; i < NAME_SIZE; i++)
      name[i] = '\0';
} // Default constructor

GPTPart::~GPTPart(void) {
} // destructor

// Return the gdisk-specific two-byte hex code for the partition
uint16_t GPTPart::GetHexType(void) {
   return partitionType.GetHexType();
} // GPTPart::GetHexType()

// Return a plain-text description of the partition type (e.g., "Linux/Windows
// data" or "Linux swap").
string GPTPart::GetTypeName(void) {
   return partitionType.TypeName();
} // GPTPart::GetNameType()

// Compute and return the partition's length (or 0 if the end is incorrectly
// set before the beginning).
uint64_t GPTPart::GetLengthLBA(void) {
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
      if ((i > 0) && (i <= NAME_SIZE))
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

// Set the name for the partition based on the current GUID partition type
// code's associated name
void GPTPart::SetDefaultDescription(void) {
   SetName(partitionType.TypeName());
} // GPTPart::SetDefaultDescription()

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

// Display summary information; does nothing if the partition is empty.
void GPTPart::ShowSummary(int partNum, uint32_t blockSize) {
   string sizeInSI;
   int i;

   if (firstLBA != 0) {
      sizeInSI = BytesToSI(blockSize * (lastLBA - firstLBA + 1));
      cout.fill(' ');
      cout.width(4);
      cout << partNum + 1 << "  ";
      cout.width(14);
      cout << firstLBA << "  ";
      cout.width(14);
      cout << lastLBA  << "   ";
      cout << BytesToSI(blockSize * (lastLBA - firstLBA + 1)) << "  ";
      for (i = 0; i < 10 - (int) sizeInSI.length(); i++)
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
      cout << "Partition GUID code: " << partitionType.AsString();
      cout << " (" << partitionType.TypeName() << ")\n";
      cout << "Partition unique GUID: " << uniqueGUID.AsString() << "\n";

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
      cout << hex;
      cout << attributes << "\n";
      cout << dec;
      cout << "Partition name: " << GetDescription() << "\n";
      cout.fill(' ');
   }  // if
} // GPTPart::ShowDetails()

// Blank (delete) a single partition
void GPTPart::BlankPartition(void) {
   int j;

   uniqueGUID.Zero();
   partitionType.Zero();
   firstLBA = 0;
   lastLBA = 0;
   attributes = 0;
   for (j = 0; j < NAME_SIZE; j++)
      name[j] = '\0';
} // GPTPart::BlankPartition

// Returns 1 if the two partitions overlap, 0 if they don't
int GPTPart::DoTheyOverlap(const GPTPart & other) {
   int theyDo = 0;

   // Don't bother checking unless these are defined (both start and end points
   // are 0 for undefined partitions, so just check the start points)
   if ((firstLBA != 0) && (other.firstLBA != 0)) {
      if ((firstLBA < other.lastLBA) && (lastLBA >= other.firstLBA))
         theyDo = 1;
      if ((other.firstLBA < lastLBA) && (other.lastLBA >= firstLBA))
         theyDo = 1;
   } // if
   return (theyDo);
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
   char* junk;
   unsigned int changeName = 0;
   PartType tempType = (GUIDData) "00000000-0000-0000-0000-000000000000";

   if (GetDescription() == GetTypeName())
      changeName = UINT16_C(1);

   cout << "Current type is '" << GetTypeName() << "'\n";
   do {
      cout << "Hex code or GUID (L to show codes, Enter = 0700): ";
      junk = fgets(line, 255, stdin);
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
