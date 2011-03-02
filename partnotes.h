/*
    partnotes.h -- Class that takes notes on GPT partitions for purpose of MBR conversion
    Copyright (C) 2010-2011 Roderick W. Smith

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

#ifndef __PARTNOTES_H
#define __PARTNOTES_H

#include <stdint.h>
#include <sys/types.h>

using namespace std;

#define WILL_NOT_CONVERT 0
#define PRIMARY 1
#define LOGICAL 2

// Data structure used in GPT-to-MBR conversions; holds pointer to GPT
// partition number, start and end sectors, and a few MBR-specific details
struct PartInfo {
   int origPartNum;
   int spaceBefore; // boolean; if 1, can theoretically become a logical part.
   int active; // boolean
   int type; // WILL_NOT_CONVERT, PRIMARY, or LOGICAL
   uint64_t firstLBA;
   uint64_t lastLBA;
   unsigned int hexCode;
   struct PartInfo *next;
};

struct ExtendedInfo {
   int startNum;
   int numLogicals;
};

class PartNotes {
   protected:
      struct PartInfo dummyNote;
      struct PartInfo *notes;
      struct PartInfo *currentNote;
      int currentIndex;
      int blockSize;
      int origTableSize;

      void DeleteNotes(void);

   public:
      PartNotes();
      ~PartNotes();

      // Add partition notes (little or no error checking)
//      int PassPartitions(GPTPart *parts, GPTData *gpt, int num, int blockSize);
      int AddToEnd(struct PartInfo* newOne);
      int AddToStart(struct PartInfo* newOne);
      void SetType(int partNum, int type); // type is PRIMARY, LOGICAL, or WILL_NOT_CONVERT
      void SetMbrHexType(int partNum, uint8_t type);
      void ToggleActiveStatus(int partNum);

      // Retrieve data or metadata
      void Rewind(void);
      int GetNextInfo(struct PartInfo * info);
      int GetNumParts();
      int GetNumPrimary();
      int GetNumExtended();
      int GetNumLogical();
      int GetType(int partNum);
      uint8_t GetMbrHexType(int i);
      int GetOrigNum(int partNum);
      int GetActiveStatus(int partNum);
      int CanBeLogical(int partNum); // returns boolean
      int FindExtended(int &start);
      int IsLegal(void); // returns Boolean

      // Manipulate data or metadata
      void RemoveDuplicates(void);
      int MakeItLegal(void);
      void TrimSmallestExtended(void);

      // Interact with users, possibly changing data with error handling
      virtual void ShowSummary(void);
      int MakeChange(int partNum);
      int ChangeType(int partNum, int newType);
};

#endif // __PARTNOTES_H
