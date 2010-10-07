/*
    partnotes.cc -- Class that takes notes on GPT partitions for purpose of MBR conversion
    Copyright (C) 2010 Roderick W. Smith

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*/

#include <iostream>
#include <stdio.h>
#include "partnotes.h"
#include "gpt.h"

using namespace std;

PartNotes::PartNotes() {
   notes = NULL;
   currentNote = NULL;
   currentIndex = 0;
   gptParts = NULL;
   gptTableSize = 0;
   blockSize = 512;
   dummyNote.active = 0;
   dummyNote.gptPartNum = MBR_EMPTY;
   dummyNote.hexCode = 0xEE;
   dummyNote.next = NULL;
   dummyNote.spaceBefore = 0;
   dummyNote.type = WILL_NOT_CONVERT;
} // PartNotes constructor

PartNotes::PartNotes(GPTPart *parts, GPTData *gpt, int num, int s) {
   PassPartitions(parts, gpt, num, s);
} // PartNotes constructor passing partition information

// Destructor. Note that we do NOT delete the gptParts array, since we just
// store a pointer to an array that's allocated by the calling function.
PartNotes::~PartNotes() {
   DeleteNotes();
} // PartNotes destructor

// Delete the partition notes....
void PartNotes::DeleteNotes(void) {
   struct PartInfo *nextNote;

   while (notes != NULL) {
      nextNote = notes->next;
      delete notes;
      notes = nextNote;
   } // while
   notes = NULL;
} // PartNotes::DeleteNotes()

/*************************************************************************
 *                                                                       *
 * Begin functions that add data to the notes, either by whole notes     *
 * or in smaller units. By and large these functions perform little      *
 * or no error checking on the added data, so they can create completely *
 * bogus layouts if used carelessly.                                     *
 *                                                                       *
 *************************************************************************/

// Creates the notes linked list with as many entries as there are
// in-use GPT partitions. Note that the parts array must be pre-sorted!
// If not, this function will reject the partition table.
// Returns the number of partitions -- normally identical to num,
// unless there were problems, in which case it returns 0.
int PartNotes::PassPartitions(GPTPart *parts, GPTData *gpt, int num, int s) {
   int i;
   struct PartInfo *tempNote = NULL, *lastNote = NULL;

   if ((parts != NULL) && (num > 0)) {
      blockSize = s;
      gptParts = parts;
      gptTableSize = num;
      if (notes != NULL)
         DeleteNotes();
      for (i = 0; i < num; i++) {
         if (gptParts[i].IsUsed()){
            tempNote = new struct PartInfo;
            tempNote->next = NULL;
            tempNote->firstLBA = gptParts[i].GetFirstLBA();
            tempNote->lastLBA = gptParts[i].GetLastLBA();
            if (gpt->IsFree(gptParts[i].GetFirstLBA() - 1)) {
               tempNote->spaceBefore = 1;
               tempNote->type = LOGICAL;
            } else {
               tempNote->spaceBefore = 0;
               tempNote->type = PRIMARY;
            } // if/else
            tempNote->gptPartNum = i;
            tempNote->active = 0;
            if (lastNote == NULL) {
               lastNote = tempNote;
               notes = tempNote;
            } else {
               lastNote->next = tempNote;
               lastNote = tempNote;
            } // if/else
         } // if GPT partition in use
      } // for
      if (!IsSorted()) {
         DeleteNotes();
         gptParts = NULL;
         gptTableSize = 0;
         cerr << "The partition table must be sorted before calling PartNotes::PassPartitions()!\n";
      } // if
   } else {
      notes = NULL;
      gptParts = NULL;
      gptTableSize = 0;
   } // if/else
   return gptTableSize;
} // PartNotes::PassPartitions

// Add a single partition to the end of the linked list.
// Returns 1 on success, 0 on failures.
int PartNotes::AddToEnd(struct PartInfo *newOne) {
   struct PartInfo *temp;
   int allOK = 1;

   if (newOne != NULL) {
      newOne->next = NULL;
      if (notes == NULL) {
         notes = newOne;
      } else {
         temp = notes;
	      while (temp->next != NULL) {
		      temp = temp->next;
         } // while
	      temp->next = newOne;
	  } // if/else
   } else allOK = 0;
   return allOK;
} // PartNotes::AddToEnd()

// Add a single partition to the start of the linked list.
// Returns 1 on success, 0 on failures.
int PartNotes::AddToStart(struct PartInfo *newOne) {
   int allOK = 1;

   if (newOne != NULL) {
      newOne->next = notes;
      notes = newOne;
   } else allOK = 0;
   return allOK;
} // PartNotes::AddToStart()

// Set the type of the partition to PRIMARY, LOGICAL, or WILL_NOT_CONVERT.
// If partNum is <0, sets type on first partition; if partNum > number
// of defined partitions, sets type on last partition; if notes list is
// NULL, does nothing.
void PartNotes::SetType(int partNum, int type) {
   int count = 0;
   struct PartInfo *theNote;

   theNote = notes;
   if (theNote != NULL) {
      while ((count++ < partNum) && (theNote->next != NULL))
         theNote = theNote->next;
      theNote->type = type;
   } // if
} // PartNotes::SetType()

// Set the MBR hex type to be used
void PartNotes::SetMbrHexType(int partNum, uint8_t type) {
   int count = 0;
   struct PartInfo *theNote;

   theNote = notes;
   if ((type != 0x0f) && (type != 0x05) && (type != 0x85)) {
      if (theNote != NULL) {
         while ((count++ < partNum) && (theNote->next != NULL))
            theNote = theNote->next;
         theNote->hexCode = type;
      } // if
   } else {
      cout << "Refusing to set MBR hex code to one used for an extended partition!\n";
   } // if/else
} // PartNotes::SetMBRHexType()

void PartNotes::ToggleActiveStatus(int partNum) {
   int count = 0;
   struct PartInfo *theNote;

   theNote = notes;
   if (theNote != NULL) {
      while ((count++ < partNum) && (theNote->next != NULL))
         theNote = theNote->next;
      if (theNote->active)
         theNote->active = 0;
      else
         theNote->active = 0x80;
   } // if
} // PartNotes::ToggleActiveStatus


/***************************************************************************
 *                                                                         *
 * The following functions retrieve data, either in whole PartInfo units   *
 * or in smaller chunks. Some functions perform computations and           *
 * comparisons that may require traversing the entire linked list, perhaps *
 * multiple times.                                                         *
 *                                                                         *
 ***************************************************************************/

// Reset currentNotes pointer to the start of the linked list and adjust
// currentIndex to 0. (Doesn't really retrieve data, but it should be
// called before most loops that employ PartNotes::GetNextInfo().)
void PartNotes::Rewind(void) {
   currentNote = notes;
   currentIndex = 0;
} // PartNotes::Rewind()

// Retrieve the PartInfo structure pointed to by the currentNote pointer,
// then point currentNote to the next one in the linked list.
// Returns the number of the returned structure (counting from 0),
// or -1 if the end has been reached and it returns a dummy value
int PartNotes::GetNextInfo(struct PartInfo* info) {
   if (currentNote != NULL) {
      *info = *currentNote;
      currentNote = currentNote->next;
      currentIndex++;
      return (currentIndex - 1);
   } else return -1;
} // PartNotes::GetNextInfo

// Count up the number of partitions that are flagged as either
// primary or logical
int PartNotes::GetNumParts() {
   int num = 0;
   struct PartInfo *theNote;

   theNote = notes;
   while (theNote != NULL) {
      if (((theNote->type == PRIMARY) || (theNote->type == LOGICAL)))
         num++;
      theNote = theNote->next;
   } // while()
   return num;
} // PartNotes::GetNumParts()

// Count up the number of partitions that are flagged as MBR primary
// partitions and that have non-negative partition numbers. Note that
// this value can be greater than 4.
int PartNotes::GetNumPrimary() {
   int num = 0;
   struct PartInfo *theNote;

   theNote = notes;
   while (theNote != NULL) {
      if (theNote->type == PRIMARY)
         num++;
      theNote = theNote->next;
   } // while()
   return num;
} // PartNotes::GetNumPrimary()

// Return the number of extended partitions required to house the
// partitions that are currently flagged as being logical. This
// number should be 0 or 1 to make a legal configuration, but this
// function will return higher values if the current configuration
// is illegal because it requires more than one extended partition.
int PartNotes::GetNumExtended() {
   int num = 0, lastWasLogical = 0;
   struct PartInfo *theNote;

   theNote = notes;
   while (theNote != NULL) {
      switch (theNote->type) {
         case PRIMARY:
            lastWasLogical = 0;
            break;
         case LOGICAL:
            if (!lastWasLogical)
               num++;
            lastWasLogical = 1;
            break;
         case WILL_NOT_CONVERT:
            break; // do nothing
      } // switch
      theNote = theNote->next;
   } // while
   return num;
} // PartNotes::GetNumExtended()

// Count up the number of partitions that are flagged as MBR logical
// partitions. Note that these may be discontiguous, and so represent
// an illegal configuration...
int PartNotes::GetNumLogical() {
   int num = 0;
   struct PartInfo *theNote;

   theNote = notes;
   while (theNote != NULL) {
      if (theNote->type == LOGICAL)
         num++;
      theNote = theNote->next;
   } // while()
   return num;
} // PartNotes::GetNumLogical()

// Return the partition type (PRIMARY, LOGICAL, or WILL_NOT_CONVERT)
// If partNum is <0, returns status of first partition; if partNum >
// number of defined partitions, returns status of last partition;
// if notes list is NULL, returns WILL_NOT_CONVERT.
int PartNotes::GetType(int partNum) {
   int count = 0;
   struct PartInfo *theNote;

   theNote = notes;
   if (theNote != NULL) {
      while ((count++ < partNum) && (theNote->next != NULL))
         theNote = theNote->next;
      return theNote->type;
   } else {
      return WILL_NOT_CONVERT;
   } // if/else
} // PartNotes::GetType()

// Return the scheduled MBR hex code
uint8_t PartNotes::GetMbrHexType(int partNum) {
   int count = 0;
   struct PartInfo *theNote;

   theNote = notes;
   if (theNote != NULL) {
      while ((count++ < partNum) && (theNote->next != NULL))
         theNote = theNote->next;
      return theNote->hexCode;
   } else {
      return 0x00;
   } // if/else
} // PartNotes::GetMBRHexType()

// Return the GPT partition number associated with this note, -1 if
// the notes list is empty, or the GPT partition number of the last
// partition if partNum is too high.
int PartNotes::GetGptNum(int partNum) {
   int count = 0;
   struct PartInfo *theNote;

   theNote = notes;
   if (theNote != NULL) {
      while ((count++ < partNum) && (theNote->next != NULL))
         theNote = theNote->next;
      return theNote->gptPartNum;
   } else {
      return -1;
   } // if/else
} // PartNotes::GetGptNum()

// Return whether or not the partition is flagged as active (bootable)
int PartNotes::GetActiveStatus(int partNum) {
   int count = 0;
   struct PartInfo *theNote;

   theNote = notes;
   if (theNote != NULL) {
      while ((count++ < partNum) && (theNote->next != NULL))
         theNote = theNote->next;
      return theNote->active;
   } else {
      return 0;
   } // if/else
} // PartNotes::GetActiveStatus()

// Returns 1 if the partition can be a logical partition (ignoring whether this
// will make the set as a whole illegal), 0 if it must be a primary
int PartNotes::CanBeLogical(int partNum) {
   int count = 0;
   struct PartInfo *theNote;

   theNote = notes;
   if (theNote != NULL) {
      while ((count++ < partNum) && (theNote->next != NULL))
         theNote = theNote->next;
      return theNote->spaceBefore;
   } else {
      return 0;
   } // if/else
} // PartNotes::CanBeLogical()

// Find the logical partition that begins the first extended
// partition, starting at start.
// Returns the number of logicals in that partition, or 0 if nothing
// is found. The start value is modified to point to the first
// logical partition in the extended partition (it's called by
// reference!).
int PartNotes::FindExtended(int &start) {
   int length = 0, next = 0;
   struct PartInfo *theNote;

   if ((notes != NULL) && (start >= 0)) {
      theNote = notes;
      while ((theNote->next != NULL) && ((next < start) || (theNote->type != LOGICAL))) {
         theNote = theNote->next;
         next++;
      } // while()
      start = next;
      while ((theNote->next != NULL) && (theNote->type == LOGICAL)) {
         length ++;
         theNote = theNote->next;
      } // while
   } // if arrays exist
   return length;
} // PartNotes::FindExtended()

// Returns 1 if the GPT partition table is sorted, 0 if not (or if the
// pointer is NULL)
int PartNotes::IsSorted(void) {
   int i, sorted = 1;
   uint64_t lastStartLBA = 0;

   if (gptParts == NULL) {
      sorted = 0;
   } else {
      for (i = 0; i < gptTableSize; i++) {
         if (lastStartLBA > gptParts[i].GetFirstLBA())
            sorted = 0;
      } // for
   } // if/else
   return sorted;
} // PartNotes::IsSorted()

// Returns 1 if the set as a whole makes a legal MBR partition table
// (possibly with logicals), 0 if not
int PartNotes::IsLegal(void) {
   int p, e;

   p = GetNumPrimary();
   e = GetNumExtended();
   return (((p+e) <= 4) && (e <= 1));
} // PartNotes::IsLegal()

/*************************************************************************
 *                                                                       *
 * The following partitions manipulate the data in the quest to create a *
 * legal MBR layout.                                                     *
 *                                                                       *
 *************************************************************************/

// Remove duplicate partitions from the list.
void PartNotes::RemoveDuplicates(void) {
   struct PartInfo *n1, *n2;

   n1 = notes;
   while (n1 != NULL) {
      n2 = n1->next;
      while (n2 != NULL) {
         if ((n1->firstLBA == n2->firstLBA) && (n1->lastLBA == n2->lastLBA)) {
            n1->next = n2->next;
            delete n2;
            n2 = n1->next;
         } else {
            n2 = n2->next;
         } // if/else
      } // while (n2 != NULL)
      n1 = n1->next;
   } // while (n1 != NULL)
} // PartNotes::RemoveDuplicates()

// Creates a legal mix of primaries and logicals, maximizing the number
// of included partitions. Also removes duplicates.
// Returns 1 if successful, 0 if not (if missing notes list, say)
int PartNotes::MakeItLegal(void) {
   struct PartInfo *theNote, *lastPrimary;

   if (notes == NULL)
      return 0;

   RemoveDuplicates();

   if (!IsLegal()) {
      // Start by eliminating or converting excessive extended partitions...
      while (GetNumExtended() > 1)
         TrimSmallestExtended();
      // If that was insufficient, cut primary partitions...
      lastPrimary = notes;
      while (!IsLegal() && (lastPrimary != NULL)) {
         lastPrimary = NULL;
         theNote = notes;
         do {
            if (theNote->type == PRIMARY)
               lastPrimary = theNote;
            theNote = theNote->next;
         } while (theNote != NULL);
         if (lastPrimary != NULL)
            lastPrimary->type = WILL_NOT_CONVERT;
      } // if
   } // if

   // If four or fewer partitions were converted, make them all primaries
   if ((GetNumPrimary() + GetNumLogical()) <= 4) {
      theNote = notes;
      do {
         if (theNote->type == LOGICAL)
            theNote->type = PRIMARY;
         theNote = theNote->next;
      } while (theNote != NULL);
   } // if

   // Try to make the first partition a primary...
   if ((GetNumExtended() + GetNumPrimary()) < 4) {
      theNote = notes;
      do {
         theNote = theNote->next;
      } while ((theNote != NULL) && (theNote->type != WILL_NOT_CONVERT));
      if ((theNote != NULL) && (theNote->type == LOGICAL))
         theNote->type = PRIMARY;
   } // if

   return IsLegal();
} // PartNotes::MakeItLegal()

// Change the type flag in the notes array for all the partitions associated
// with the smallest extended partition to WILL_NOT_CONVERT or (if possible)
// PRIMARY
void PartNotes::TrimSmallestExtended() {
   struct ExtendedInfo *extendeds;
   int i, numExtended, shortestNum, shortestIndex = -1;
   struct PartInfo *theNote;

   if (notes != NULL) {
      numExtended = GetNumExtended();
      extendeds = new struct ExtendedInfo[numExtended];

      // Find the start and number of partitions in each extended partition....
      for (i = 0; i < numExtended; i++) {
         if (i == 0)
            extendeds[i].startNum = 0;
         else
            extendeds[i].startNum = extendeds[i - 1].startNum + extendeds[i - 1].numLogicals;
         extendeds[i].numLogicals = FindExtended(extendeds[i].startNum);
      } // for

      // Find the smallest extended partition....
      shortestNum = gptTableSize + 1;
      for (i = 0; i < numExtended; i++) {
         if (extendeds[i].numLogicals < shortestNum) {
            shortestNum = extendeds[i].numLogicals;
            shortestIndex = extendeds[i].startNum;
         } // if
      } // for

      // Now flag its partitions as PRIMARY (if possible) or
      // WILL_NOT_CONVERT
      theNote = notes;
      i = 0;
      while ((i++ < shortestIndex) && (theNote->next != NULL))
         theNote = theNote->next;
      do {
         if (GetNumPrimary() < 3)
            theNote->type = PRIMARY;
         else
            theNote->type = WILL_NOT_CONVERT;
         theNote = theNote->next;
      } while ((i++ < shortestNum + shortestIndex) && (theNote != NULL));

      delete[] extendeds;
   } // if
} // PartNotes::TrimSmallestExtended


/*************************************************************************
 *                                                                       *
 * Interact with users, presenting data and/or collecting responses. May *
 * change data with error detection and correction.                      *
 *                                                                       *
 *************************************************************************/

// Display summary information for the user
void PartNotes::ShowSummary(void) {
   int j;
   string sizeInSI;
   struct PartInfo *theNote;

   if ((gptParts != NULL) && (notes != NULL)) {
      theNote = notes;
      cout << "Sorted GPT partitions and their current conversion status:\n";
      cout << "                                      Can Be\n";
      cout << "Number    Boot    Size       Status   Logical   Code   GPT Name\n";
      while (theNote != NULL) {
         if (gptParts[theNote->gptPartNum].IsUsed()) {
            cout.fill(' ');
            cout.width(4);
            cout << theNote->gptPartNum + 1 << "    ";
            if (theNote->active)
               cout << "   *    ";
            else
               cout << "        ";
            sizeInSI = BytesToSI((gptParts[theNote->gptPartNum].GetLastLBA() -
                                 gptParts[theNote->gptPartNum].GetFirstLBA() + 1), blockSize);
            cout << " " << sizeInSI;
            for (j = 0; j < 12 - (int) sizeInSI.length(); j++)
               cout << " ";
            switch (theNote->type) {
               case PRIMARY:
                  cout << "primary     ";
                  break;
               case LOGICAL:
                  cout << "logical     ";
                  break;
               case WILL_NOT_CONVERT:
                  cout << "OMITTED     ";
                  break;
               default:
                  cout << "**ERROR**   ";
                  break;
            } // switch
            if (theNote->spaceBefore)
               cout << "Y       ";
            else
               cout << "-       ";
            cout.fill('0');
            cout.width(2);
            cout.setf(ios::uppercase);
            cout << hex << (int) theNote->hexCode << "    " << dec;
            cout.fill(' ');
            cout << gptParts[theNote->gptPartNum].GetDescription().substr(0, 25) << "\n";
         } // if
         theNote = theNote->next;
      } // for
   } // if
} // PartNotes::ShowSummary()

// Interact with the user to create a change in the specified
// partition.
// Returns 1 if successful, 0 if not. Note that aborting the
// change counts as successful; 0 should be returned only in
// case of dire bugs.
int PartNotes::MakeChange(int partNum) {
   int allOK = 1;
   int type = 0;
   char *junk;
   char line[255], command;

   if (notes != NULL) {
      cout << "What do you want to do?\n";
      cout << " a - toggle active flag\n";
      switch (GetType(partNum)) {
         case PRIMARY:
            cout << " d - drop partition from MBR\n";
            cout << " l - convert partition to logical\n";
            break;
         case LOGICAL:
            cout << " d - drop partition from MBR\n";
            cout << " p - convert partition to primary\n";
            break;
         case WILL_NOT_CONVERT:
            cout << " p - add partition as primary\n";
            cout << " l - add partition as logical\n";
            break;
      } // switch
      cout << " t - change MBR type code\n";
      cout << "Action: ";
      junk = fgets(line, 255, stdin);
      sscanf(line, "%c", &command);
      switch (command) {
         case 'a': case 'A':
            ToggleActiveStatus(partNum);
            break;
         case 'd': case 'D':
            allOK = ChangeType(partNum, WILL_NOT_CONVERT);
            break;
         case 'l': case 'L':
            allOK = ChangeType(partNum, LOGICAL);
            break;
         case 'p': case 'P':
            allOK = ChangeType(partNum, PRIMARY);
            break;
         case 't': case 'T':
            while (type == 0) {
               cout << "Enter a 2-byte hexadecimal MBR type code: ";
               junk = fgets(line, 255, stdin);
               sscanf(line, "%x", &type);
            } // while
            SetMbrHexType(partNum, (uint8_t) type);
            break;
         default:
            cout << "Unrecognized command; making no change.\n";
            break;
      } // switch
   } else allOK = 0; // if
   return allOK;
} // PartNotes::MakeChange()

// Turn the partition into the specified type, if this is legal.
// Returns 1 unless there's a dire bug.
int PartNotes::ChangeType(int partNum, int newType) {
   int origType, allOK = 1;
   char *junk, line[255];

   if ((notes != NULL) && IsLegal()) {
      origType = GetType(partNum);
      SetType(partNum, newType);
      if (!IsLegal()) {
         cout << "The requested change is not possible.\n";
         if (newType == LOGICAL) {
            if (!CanBeLogical(partNum))
               cout << "At least one free sector must exist before each logical partition.\n";
            else
               cout << "All logical partitions must be contiguous.\n";
         } // if
         if ((newType == PRIMARY) && (GetNumPrimary() + GetNumExtended()) > 4)
            cout << "You can have only four primary partitions (all logical partitions "
                 << "count as one\nprimary partition).\n";
         if ((newType == PRIMARY) && (GetNumExtended()) > 1)
            cout << "Logical partitions must form a single contiguous group.\n";
         cout << "\nYou may be able to achieve your desired goal by making changes in "
              << "another\norder, such as deleting partitions before changing others' "
              << "types.\n";
         cout << "\nReverting change.\nPress <Enter> to continue: ";
         junk = fgets(line, 255, stdin);
         SetType(partNum, origType);
      } // if
   } else allOK = 0; // if
   return allOK;
} // PartNotes::ChangeType()
