/*
    Copyright (C) <2010>  <Roderick W. Smith>

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

/* This class implements an interactive text-mode interface atop the
   GPTData class */

#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <iostream>
#include <sstream>
#include <cstdio>
#include "attributes.h"
#include "gpttext.h"
#include "partnotes.h"
#include "support.h"

using namespace std;

/*******************************************
*                                          *
* GPTDataText class and related structures *
*                                          *
********************************************/

GPTDataTextUI::GPTDataTextUI(void) : GPTData() {
} // default constructor

GPTDataTextUI::GPTDataTextUI(string filename) : GPTData(filename) {
} // constructor passing filename

GPTDataTextUI::~GPTDataTextUI(void) {
} // default destructor

/*********************************************************************
 *                                                                   *
 * The following functions are extended (interactive) versions of    *
 * simpler functions in the base class....                           *
 *                                                                   *
 *********************************************************************/

// Overridden function; calls base-class function and then makes
// additional queries of the user, if the base-class function can't
// decide what to do.
WhichToUse GPTDataTextUI::UseWhichPartitions(void) {
   WhichToUse which;
   MBRValidity mbrState;
   int answer;

   which = GPTData::UseWhichPartitions();
   if ((which != use_abort) || beQuiet)
      return which;

   // If we get past here, it means that the non-interactive tests were
   // inconclusive, so we must ask the user which table to use....
   mbrState = protectiveMBR.GetValidity();

   if ((state == gpt_valid) && (mbrState == mbr)) {
      cout << "Found valid MBR and GPT. Which do you want to use?\n";
      answer = GetNumber(1, 3, 2, " 1 - MBR\n 2 - GPT\n 3 - Create blank GPT\n\nYour answer: ");
      if (answer == 1) {
         which = use_mbr;
      } else if (answer == 2) {
         which = use_gpt;
         cout << "Using GPT and creating fresh protective MBR.\n";
      } else which = use_new;
   } // if

   // Nasty decisions here -- GPT is present, but corrupt (bad CRCs or other
   // problems)
   if (state == gpt_corrupt) {
      if ((mbrState == mbr) || (mbrState == hybrid)) {
         cout << "Found valid MBR and corrupt GPT. Which do you want to use? (Using the\n"
              << "GPT MAY permit recovery of GPT data.)\n";
         answer = GetNumber(1, 3, 2, " 1 - MBR\n 2 - GPT\n 3 - Create blank GPT\n\nYour answer: ");
         if (answer == 1) {
            which = use_mbr;
         } else if (answer == 2) {
            which = use_gpt;
         } else which = use_new;
      } else if (mbrState == invalid) {
         cout << "Found invalid MBR and corrupt GPT. What do you want to do? (Using the\n"
              << "GPT MAY permit recovery of GPT data.)\n";
         answer = GetNumber(1, 2, 1, " 1 - GPT\n 2 - Create blank GPT\n\nYour answer: ");
         if (answer == 1) {
            which = use_gpt;
         } else which = use_new;
      } // if/else/else
   } // if (corrupt GPT)

   return which;
} // UseWhichPartitions()

// Ask the user for a partition number; and prompt for verification
// if the requested partition isn't of a known BSD type.
// Lets the base-class function do the work, and returns its value (the
// number of converted partitions).
int GPTDataTextUI::XFormDisklabel(void) {
   uint32_t partNum;
   uint16_t hexCode;
   int goOn = 1, numDone = 0;
   BSDData disklabel;

   partNum = GetPartNum();

   // Now see if the specified partition has a BSD type code....
   hexCode = partitions[partNum].GetHexType();
   if ((hexCode != 0xa500) && (hexCode != 0xa900)) {
      cout << "Specified partition doesn't have a disklabel partition type "
           << "code.\nContinue anyway? ";
      goOn = (GetYN() == 'Y');
   } // if

   if (goOn)
      numDone = GPTData::XFormDisklabel(partNum);

   return numDone;
} // GPTData::XFormDisklabel(int i)


/*********************************************************************
 *                                                                   *
 * Begin functions that obtain information from the users, and often *
 * do something with that information (call other functions)         *
 *                                                                   *
 *********************************************************************/

// Prompts user for partition number and returns the result.
uint32_t GPTDataTextUI::GetPartNum(void) {
   uint32_t partNum;
   uint32_t low, high;
   ostringstream prompt;

   if (GetPartRange(&low, &high) > 0) {
      prompt << "Partition number (" << low + 1 << "-" << high + 1 << "): ";
      partNum = GetNumber(low + 1, high + 1, low, prompt.str());
   } else partNum = 1;
   return (partNum - 1);
} // GPTDataTextUI::GetPartNum()

// What it says: Resize the partition table. (Default is 128 entries.)
void GPTDataTextUI::ResizePartitionTable(void) {
   int newSize;
   ostringstream prompt;
   uint32_t curLow, curHigh;

   cout << "Current partition table size is " << mainHeader.numParts << ".\n";
   GetPartRange(&curLow, &curHigh);
   curHigh++; // since GetPartRange() returns numbers starting from 0...
   // There's no point in having fewer than four partitions....
   if (curHigh < (blockSize / GPT_SIZE))
      curHigh = blockSize / GPT_SIZE;
   prompt << "Enter new size (" << curHigh << " up, default " << NUM_GPT_ENTRIES << "): ";
   newSize = GetNumber(4, 65535, 128, prompt.str());
   if (newSize < 128) {
      cout << "Caution: The partition table size should officially be 16KB or larger,\n"
           << "which works out to 128 entries. In practice, smaller tables seem to\n"
           << "work with most OSes, but this practice is risky. I'm proceeding with\n"
           << "the resize, but you may want to reconsider this action and undo it.\n\n";
   } // if
   SetGPTSize(newSize);
} // GPTDataTextUI::ResizePartitionTable()

// Interactively create a partition
void GPTDataTextUI::CreatePartition(void) {
   uint64_t firstBlock, firstInLargest, lastBlock, sector;
   uint32_t firstFreePart = 0;
   ostringstream prompt1, prompt2, prompt3;
   int partNum;

   // Find first free partition...
   while (partitions[firstFreePart].GetFirstLBA() != 0) {
      firstFreePart++;
   } // while

   if (((firstBlock = FindFirstAvailable()) != 0) &&
       (firstFreePart < mainHeader.numParts)) {
      lastBlock = FindLastAvailable();
      firstInLargest = FindFirstInLargest();

      // Get partition number....
      do {
         prompt1 << "Partition number (" << firstFreePart + 1 << "-" << mainHeader.numParts
                 << ", default " << firstFreePart + 1 << "): ";
         partNum = GetNumber(firstFreePart + 1, mainHeader.numParts,
                             firstFreePart + 1, prompt1.str()) - 1;
         if (partitions[partNum].GetFirstLBA() != 0)
            cout << "partition " << partNum + 1 << " is in use.\n";
      } while (partitions[partNum].GetFirstLBA() != 0);

      // Get first block for new partition...
      prompt2 << "First sector (" << firstBlock << "-" << lastBlock << ", default = "
              << firstInLargest << ") or {+-}size{KMGT}: ";
      do {
         sector = GetSectorNum(firstBlock, lastBlock, firstInLargest, prompt2.str());
      } while (IsFree(sector) == 0);
      Align(&sector); // Align sector to correct multiple
      firstBlock = sector;

      // Get last block for new partitions...
      lastBlock = FindLastInFree(firstBlock);
      prompt3 << "Last sector (" << firstBlock << "-" << lastBlock << ", default = "
            << lastBlock << ") or {+-}size{KMGT}: ";
      do {
         sector = GetSectorNum(firstBlock, lastBlock, lastBlock, prompt3.str());
      } while (IsFree(sector) == 0);
      lastBlock = sector;

      firstFreePart = GPTData::CreatePartition(partNum, firstBlock, lastBlock);
      partitions[partNum].ChangeType();
      partitions[partNum].SetDefaultDescription();
   } else {
      cout << "No free sectors available\n";
   } // if/else
} // GPTDataTextUI::CreatePartition()

// Interactively delete a partition (duh!)
void GPTDataTextUI::DeletePartition(void) {
   int partNum;
   uint32_t low, high;
   ostringstream prompt;

   if (GetPartRange(&low, &high) > 0) {
      prompt << "Partition number (" << low + 1 << "-" << high + 1 << "): ";
      partNum = GetNumber(low + 1, high + 1, low, prompt.str());
      GPTData::DeletePartition(partNum - 1);
   } else {
      cout << "No partitions\n";
   } // if/else
} // GPTDataTextUI::DeletePartition()

// Prompt user for a partition number, then change its type code
// using ChangeGPTType(struct GPTPartition*) function.
void GPTDataTextUI::ChangePartType(void) {
   int partNum;
   uint32_t low, high;

   if (GetPartRange(&low, &high) > 0) {
      partNum = GetPartNum();
      partitions[partNum].ChangeType();
   } else {
      cout << "No partitions\n";
   } // if/else
} // GPTDataTextUI::ChangePartType()

// Partition attributes seem to be rarely used, but I want a way to
// adjust them for completeness....
void GPTDataTextUI::SetAttributes(uint32_t partNum) {
   Attributes theAttr;

   theAttr.SetAttributes(partitions[partNum].GetAttributes());
   theAttr.DisplayAttributes();
   theAttr.ChangeAttributes();
   partitions[partNum].SetAttributes(theAttr.GetAttributes());
} // GPTDataTextUI::SetAttributes()

// Ask user for two partition numbers and swap them in the table. Note that
// this just reorders table entries; it doesn't adjust partition layout on
// the disk.
// Returns 1 if successful, 0 if not. (If user enters identical numbers, it
// counts as successful.)
int GPTDataTextUI::SwapPartitions(void) {
   int partNum1, partNum2, didIt = 0;
   uint32_t low, high;
   ostringstream prompt;
   GPTPart temp;

   if (GetPartRange(&low, &high) > 0) {
      partNum1 = GetPartNum();
      if (high >= mainHeader.numParts - 1)
         high = 0;
      prompt << "New partition number (1-" << mainHeader.numParts
             << ", default " << high + 2 << "): ";
      partNum2 = GetNumber(1, mainHeader.numParts, high + 2, prompt.str()) - 1;
      didIt = GPTData::SwapPartitions(partNum1, partNum2);
   } else {
      cout << "No partitions\n";
   } // if/else
   return didIt;
} // GPTDataTextUI::SwapPartitionNumbers()

// This function destroys the on-disk GPT structures. Returns 1 if the user
// confirms destruction, 0 if the user aborts or if there's a disk error.
int GPTDataTextUI::DestroyGPTwPrompt(void) {
   int allOK = 1;

   if ((apmFound) || (bsdFound)) {
      cout << "WARNING: APM or BSD disklabel structures detected! This operation could\n"
           << "damage any APM or BSD partitions on this disk!\n";
   } // if APM or BSD
   cout << "\a\aAbout to wipe out GPT on " << device << ". Proceed? ";
   if (GetYN() == 'Y') {
      if (DestroyGPT()) {
         // Note on below: Touch the MBR only if the user wants it completely
         // blanked out. Version 0.4.2 deleted the 0xEE partition and re-wrote
         // the MBR, but this could wipe out a valid MBR that the program
         // had subsequently discarded (say, if it conflicted with older GPT
         // structures).
         cout << "Blank out MBR? ";
         if (GetYN() == 'Y') {
            DestroyMBR();
         } else {
            cout << "MBR is unchanged. You may need to delete an EFI GPT (0xEE) partition\n"
                 << "with fdisk or another tool.\n";
         } // if/else
      } else allOK = 0; // if GPT structures destroyed
   } else allOK = 0; // if user confirms destruction
   return (allOK);
} // GPTDataTextUI::DestroyGPTwPrompt()

// Get partition number from user and then call ShowPartDetails(partNum)
// to show its detailed information
void GPTDataTextUI::ShowDetails(void) {
   int partNum;
   uint32_t low, high;

   if (GetPartRange(&low, &high) > 0) {
      partNum = GetPartNum();
      ShowPartDetails(partNum);
   } else {
      cout << "No partitions\n";
   } // if/else
} // GPTDataTextUI::ShowDetails()

// Create a hybrid MBR -- an ugly, funky thing that helps GPT work with
// OSes that don't understand GPT.
void GPTDataTextUI::MakeHybrid(void) {
   uint32_t partNums[3];
   char line[255];
   char* junk;
   int numParts, i, j, mbrNum, bootable = 0;
   unsigned int hexCode = 0;
   struct PartInfo *newNote;
   PartNotes notes;
   char eeFirst = 'Y'; // Whether EFI GPT (0xEE) partition comes first in table

   cout << "\nWARNING! Hybrid MBRs are flaky and potentially dangerous! If you decide not\n"
        << "to use one, just hit the Enter key at the below prompt and your MBR\n"
        << "partition table will be untouched.\n\n\a";

   // Now get the numbers of up to three partitions to add to the
   // hybrid MBR....
   cout << "Type from one to three GPT partition numbers, separated by spaces, to be\n"
        << "added to the hybrid MBR, in sequence: ";
   junk = fgets(line, 255, stdin);
   numParts = sscanf(line, "%d %d %d", &partNums[0], &partNums[1], &partNums[2]);

   if (numParts > 0) {
      cout << "Place EFI GPT (0xEE) partition first in MBR (good for GRUB)? ";
      eeFirst = GetYN();
   } // if

   for (i = 0; i < numParts; i++) {
      newNote = new struct PartInfo;
      j = newNote->gptPartNum = partNums[i] - 1;
      mbrNum = i + (eeFirst == 'Y');
      cout << "\nCreating entry for GPT partition #" << j + 1
           << " (MBR partition #" << mbrNum + 1 << ")\n";
      newNote->hexCode = GetMBRTypeCode(partitions[j].GetHexType() / 256);
      newNote->firstLBA = partitions[j].GetFirstLBA();
      newNote->lastLBA = partitions[j].GetLastLBA();
      newNote->type = PRIMARY;
      cout << "Set the bootable flag? ";
      if (GetYN() == 'Y')
         newNote->active = 1;
      else
         newNote->active = 0;
      notes.AddToEnd(newNote);
   } // for

   if (numParts > 0) { // User opted to create a hybrid MBR....
      // Create EFI protective partition that covers the start of the disk.
      // If this location (covering the main GPT data structures) is omitted,
      // Linux won't find any partitions on the disk.
      newNote = new struct PartInfo;
      newNote->gptPartNum = MBR_EFI_GPT;
      newNote->active = 0;
      newNote->hexCode = 0xEE;
      newNote->type = PRIMARY;
      // newNote firstLBA and lastLBA are computed later...
      if (eeFirst == 'Y') {
	     notes.AddToStart(newNote);
      } else {
	     notes.AddToEnd(newNote);
      } // else
      protectiveMBR.SetHybrid();

      // ... and for good measure, if there are any partition spaces left,
      // optionally create another protective EFI partition to cover as much
      // space as possible....
      if (notes.GetNumPrimary() < 4) { // unused entry....
         cout << "\nUnused partition space(s) found. Use one to protect more partitions? ";
         if (GetYN() == 'Y') {
            while ((hexCode <= 0) || (hexCode > 255)) {
               cout << "Enter an MBR hex code (EE is EFI GPT, but may confuse MacOS): ";
               // Comment on above: Mac OS treats disks with more than one
               // 0xEE MBR partition as MBR disks, not as GPT disks.
               junk = fgets(line, 255, stdin);
               sscanf(line, "%x", &hexCode);
               if (line[0] == '\n')
                  hexCode = 0x00;
            } // while
            newNote = new struct PartInfo;
            newNote->gptPartNum = MBR_EFI_GPT;
            newNote->active = 0;
            newNote->hexCode = hexCode;
            newNote->type = PRIMARY;
            // newNote firstLBA and lastLBA are computed later...
            notes.AddToEnd(newNote);
         } // if (GetYN() == 'Y')
      } // if unused entry
      PartsToMBR(notes);
      if (bootable > 0)
         protectiveMBR.SetPartBootable(bootable);
   } // if (numParts > 0)
} // GPTDataTextUI::MakeHybrid()

// Assign GPT partitions to primary or logical status for conversion. The
// function first presents a suggested layout with as many logicals as
// possible, but gives the user the option to override this suggestion.
// Returns the number of partitions assigned (0 if problems or if the
// user aborts)
int GPTDataTextUI::AssignPrimaryOrLogical(PartNotes & notes) {
   int i, partNum, allOK = 1, changesWanted = 1, countedParts, numPrimary = 0, numLogical = 0;
   int newNumParts; // size of GPT table

   // Sort and resize the GPT table. Resized to clear the sector before the
   // first available sector, if possible, so as to enable turning the
   // first partition into a logical, should that be desirable/necessary.
   SortGPT();
   countedParts = newNumParts = CountParts();
   i = blockSize / GPT_SIZE;
   if ((newNumParts % i) != 0) {
      newNumParts = ((newNumParts / i) + 1) * i;
   } // if
   SetGPTSize(newNumParts);

   // Takes notes on existing partitions: Create an initial assignment as
   // primary or logical, set default MBR types, and then make it legal
   // (drop partitions as required to fit in the MBR and as logicals).
   allOK = (notes.PassPartitions(partitions, this, mainHeader.numParts, blockSize) ==
            (int) mainHeader.numParts);
   for (i = 0; i < countedParts; i++)
      notes.SetMbrHexType(i, partitions[i].GetHexType() / 255);
   notes.MakeItLegal();

   while (allOK && changesWanted) {
      notes.ShowSummary();
      cout << "\n";
      partNum = GetNumber(-1, countedParts, -2,
                          "Type partition to change, 0 to accept, -1 to abort: ");
      switch (partNum) {
         case -1:
            allOK = 0;
            break;
         case 0:
            changesWanted = 0;
            break;
         default:
            allOK = notes.MakeChange(partNum - 1);
            break;
      } // switch
   } // while

   i = 0;
   if (allOK)
      for (i = 0; i < countedParts; i++) {
         switch (notes.GetType(i)) {
            case PRIMARY:
               numPrimary++;
               break;
            case LOGICAL:
               numLogical++;
               break;
         } // switch
         if (notes.GetActiveStatus(i))
            protectiveMBR.SetPartBootable(i);
      } // for

   if (numPrimary > 4) {
      cerr << "Warning! More than four primary partitions in "
           << "GPTDataTextUI::AssignPrimaryOrLogical()!\n";
      allOK = 0;
   } // if
   return (allOK * (numPrimary + numLogical));
} // GPTDataTextUI::AssignPrimaryOrLogical()

// Convert the GPT to MBR form, storing partitions in the protectiveMBR
// variable. This function is necessarily limited; it may not be able to
// convert all partitions, depending on the disk size and available space
// before each partition (one free sector is required to create a logical
// partition, which are necessary to convert more than four partitions).
// Returns the number of converted partitions; if this value
// is over 0, the calling function should call DestroyGPT() to destroy
// the GPT data, call SaveMBR() to save the MBR, and then exit.
int GPTDataTextUI::XFormToMBR(void) {
   int numParts, numToConvert, numReallyConverted = 0;
   int origNumParts;
   PartNotes notes;
   GPTPart *tempGptParts;
   uint32_t i;

   // Back up partition array, since we'll be sorting it and we want to
   // be able to restore it in case the user aborts....
   origNumParts = mainHeader.numParts;
   tempGptParts = new GPTPart[mainHeader.numParts];
   for (i = 0; i < mainHeader.numParts; i++)
      tempGptParts[i] = partitions[i];

   numParts = CountParts();

   numToConvert = AssignPrimaryOrLogical(notes);

   if (numToConvert > 0) {
      numReallyConverted = PartsToMBR(notes);
      if (numReallyConverted != numToConvert) {
         cerr << "Error converting partitions to MBR; tried to convert "
              << numToConvert << " partitions,\nbut converted " << numReallyConverted
              << ". Aborting operation!\n";
         numReallyConverted = 0;
         protectiveMBR.MakeProtectiveMBR();
      } // if/else
   } // if

   // A problem or the user aborted; restore backup of unsorted and
   // original-sized partition table and delete the sorted and
   // resized one; otherwise free the backup table's memory....
   if (numReallyConverted == 0) {
      delete[] partitions;
      partitions = tempGptParts;
      SetGPTSize(origNumParts);
   } else {
      delete[] tempGptParts;
   } // if
   return numReallyConverted;
} // GPTDataTextUI::XFormToMBR()

/*********************************************************************
 *                                                                   *
 * The following doesn't really belong in the class, since it's MBR- *
 * specific, but it's also user I/O-related, so I want to keep it in *
 * this file....                                                     *
 *                                                                   *
 *********************************************************************/

// Get an MBR type code from the user and return it
int GetMBRTypeCode(int defType) {
   char line[255];
   char* junk;
   int typeCode;

   cout.setf(ios::uppercase);
   cout.fill('0');
   do {
      cout << "Enter an MBR hex code (default " << hex;
      cout.width(2);
      cout << defType << "): " << dec;
      junk = fgets(line, 255, stdin);
      if (line[0] == '\n')
         typeCode = defType;
      else
         sscanf(line, "%x", &typeCode);
   } while ((typeCode <= 0) || (typeCode > 255));
   cout.fill(' ');
   return typeCode;
} // GetMBRTypeCode
