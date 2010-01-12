//
// C++ Interface: gptpart
//
// Description: Class to implement a single GPT partition
//
//
// Author: Rod Smith <rodsmith@rodsbooks.com>, (C) 2009
//
// Copyright: See COPYING file that comes with this distribution
//
//
// This program is copyright (c) 2009 by Roderick W. Smith. It is distributed
// under the terms of the GNU GPL version 2, as detailed in the COPYING file.

#ifndef __GPTPART_H
#define __GPTPART_H

#include <stdint.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include "support.h"
#include "parttypes.h"

using namespace std;

/****************************************
 *                                      *
 * GPTPart class and related structures *
 *                                      *
 ****************************************/

class GPTPart {
   protected:
      // Caution: The non-static data in GPTPart is precisely the right size
      // to enable easy loading of the data directly from disk. If any
      // non-static variables are added to the below, the data size will
      // change and the program will stop working. This can be corrected by
      // adjusting the data-load operation in GPTData::LoadMainTable() and
      // GPTData::LoadSecondTableAsMain() and then removing the GPTPart
      // size check in SizesOK() (in gpt.cc file).
      struct GUIDData partitionType;
      struct GUIDData uniqueGUID;
      uint64_t firstLBA;
      uint64_t lastLBA;
      uint64_t attributes;
      unsigned char name[NAME_SIZE];

      static PartTypes typeHelper;
   public:
      GPTPart(void);
      ~GPTPart(void);

      // Simple data retrieval:
      struct GUIDData GetType(void) {return partitionType;}
      uint16_t GetHexType(void);
      char* GetNameType(char* theName);
      struct GUIDData GetUniqueGUID(void) {return uniqueGUID;}
      uint64_t GetFirstLBA(void) {return firstLBA;}
      uint64_t GetLastLBA(void) {return lastLBA;}
      uint64_t GetLengthLBA(void);
      uint64_t GetAttributes(void) {return attributes;}
      unsigned char* GetName(unsigned char* theName);

      // Simple data assignment:
      void SetType(struct GUIDData t) {partitionType = t;}
      void SetType(uint16_t hex) {partitionType = typeHelper.IDToGUID(hex);}
      void SetUniqueGUID(struct GUIDData u) {uniqueGUID = u;}
      void SetUniqueGUID(int zeroOrRandom);
      void SetFirstLBA(uint64_t f) {firstLBA = f;}
      void SetLastLBA(uint64_t l) {lastLBA = l;}
      void SetAttributes(uint64_t a) {attributes = a;}
      void SetName(unsigned char* n);

      // Additional functions
      GPTPart & operator=(const GPTPart & orig);
      void ShowSummary(int partNum, uint32_t blockSize); // display summary information (1-line)
      void ShowDetails(uint32_t blockSize); // display detailed information (multi-line)
      void BlankPartition(void); // empty partition of data
      int DoTheyOverlap(GPTPart* other); // returns 1 if there's overlap
      void ReversePartBytes(void); // reverse byte order of all integer fields

      // Functions requiring user interaction
      void ChangeType(void); // Change the type code
}; // struct GPTPart

// A support function that doesn't quite belong in the class....
void QuickSortGPT(GPTPart* partitions, int start, int finish);

#endif
