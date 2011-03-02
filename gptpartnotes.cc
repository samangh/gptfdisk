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
#include "gptpartnotes.h"
#include "gpt.h"

using namespace std;

GptPartNotes::GptPartNotes() {
   gptParts = NULL;
} // GptPartNotes constructor

GptPartNotes::GptPartNotes(GPTPart *parts, GPTData *gpt, int num, int s) {
   gptParts = NULL;
   PassPartitions(parts, gpt, num, s);
} // GptPartNotes constructor passing partition information

/* // Destructor. Note that we do NOT delete the gptParts array, since we just
// store a pointer to an array that's allocated by the calling function.
GptPartNotes::~GptPartNotes() {
} // PartNotes destructor */

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
int GptPartNotes::PassPartitions(GPTPart *parts, GPTData *gpt, int num, int s) {
   int i;
   struct PartInfo *tempNote = NULL, *lastNote = NULL;

   if ((parts != NULL) && (num > 0)) {
      blockSize = s;
      gptParts = parts;
      origTableSize = num;
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
            tempNote->origPartNum = i;
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
         origTableSize = 0;
         cerr << "The partition table must be sorted before calling GptPartNotes::PassPartitions()!\n";
      } // if
   } else {
      notes = NULL;
      gptParts = NULL;
      origTableSize = 0;
   } // if/else
   return origTableSize;
} // GptPartNotes::PassPartitions

/***************************************************************************
 *                                                                         *
 * The following functions retrieve data, either in whole PartInfo units   *
 * or in smaller chunks. Some functions perform computations and           *
 * comparisons that may require traversing the entire linked list, perhaps *
 * multiple times.                                                         *
 *                                                                         *
 ***************************************************************************/

// Returns 1 if the GPT partition table is sorted, 0 if not (or if the
// pointer is NULL)
int GptPartNotes::IsSorted(void) {
   int i, sorted = 1;
   uint64_t lastStartLBA = 0;

   if (gptParts == NULL) {
      sorted = 0;
   } else {
      for (i = 0; i < origTableSize; i++) {
         if (lastStartLBA > gptParts[i].GetFirstLBA())
            sorted = 0;
      } // for
   } // if/else
   return sorted;
} // GptPartNotes::IsSorted()

/*************************************************************************
 *                                                                       *
 * Interact with users, presenting data and/or collecting responses. May *
 * change data with error detection and correction.                      *
 *                                                                       *
 *************************************************************************/

// Display summary information for the user
void GptPartNotes::ShowSummary(void) {
   size_t j;
   string sizeInSI;
   struct PartInfo *theNote;

   if ((gptParts != NULL) && (notes != NULL)) {
      theNote = notes;
      cout << "Sorted GPT partitions and their current conversion status:\n";
      cout << "                                      Can Be\n";
      cout << "Number    Boot    Size       Status   Logical   Code   GPT Name\n";
      while (theNote != NULL) {
         if (gptParts[theNote->origPartNum].IsUsed()) {
            cout.fill(' ');
            cout.width(4);
            cout << theNote->origPartNum + 1 << "    ";
            if (theNote->active)
               cout << "   *    ";
            else
               cout << "        ";
            sizeInSI = BytesToSI((gptParts[theNote->origPartNum].GetLastLBA() -
                                 gptParts[theNote->origPartNum].GetFirstLBA() + 1), blockSize);
            cout << " " << sizeInSI;
            for (j = 0; j < 12 - sizeInSI.length(); j++)
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
            cout << gptParts[theNote->origPartNum].GetDescription().substr(0, 25) << "\n";
         } // if
         theNote = theNote->next;
      } // for
   } // if
} // GptPartNotes::ShowSummary()
