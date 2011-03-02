/* mbr.cc -- Functions for loading, saving, and manipulating legacy MBR partition
   data. */

/* Initial coding by Rod Smith, January to February, 2009 */

/* This program is copyright (c) 2009 by Roderick W. Smith. It is distributed
  under the terms of the GNU GPL version 2, as detailed in the COPYING file. */

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include <iostream>
#include "mbr.h"

using namespace std;

/****************************************
 *                                      *
 * MBRData class and related structures *
 *                                      *
 ****************************************/

// Assignment operator -- copy entire set of MBR data.
MBRData & MBRData::operator=(const MBRData & orig) {
   BasicMBRData::operator=(orig);
   return *this;
} // MBRData::operator=() */

/*****************************************************
 *                                                   *
 * Functions to create, delete, or change partitions *
 *                                                   *
 *****************************************************/

// Create a protective MBR. Clears the boot loader area if clearBoot > 0.
void MBRData::MakeProtectiveMBR(int clearBoot) {

   EmptyMBR(clearBoot);

   // Initialize variables
   nulls = 0;
   MBRSignature = MBR_SIGNATURE;
   diskSignature = UINT32_C(0);

   partitions[0].status = UINT8_C(0); // Flag the protective part. as unbootable

   partitions[0].partitionType = UINT8_C(0xEE);
   partitions[0].firstLBA = UINT32_C(1);
   if (diskSize < UINT32_MAX) { // If the disk is under 2TiB
      partitions[0].lengthLBA = (uint32_t) diskSize - UINT32_C(1);
   } else { // disk is too big to represent, so fake it...
      partitions[0].lengthLBA = UINT32_MAX;
   } // if/else

   // Write CHS data. This maxes out the use of the disk, as much as
   // possible -- even to the point of exceeding the capacity of sub-8GB
   // disks. The EFI spec says to use 0xffffff as the ending value,
   // although normal MBR disks max out at 0xfeffff. FWIW, both GNU Parted
   // and Apple's Disk Utility use 0xfeffff, and the latter puts that
   // value in for the FIRST sector, too!
   LBAtoCHS(1, partitions[0].firstSector);
   if (LBAtoCHS(partitions[0].lengthLBA, partitions[0].lastSector) == 0)
      partitions[0].lastSector[0] = 0xFF;

   state = gpt;
} // MBRData::MakeProtectiveMBR()

// Optimizes the size of the 0xEE (EFI GPT) partition
void MBRData::OptimizeEESize(void) {
   int i, typeFlag = 0;
   uint32_t after;

   for (i = 0; i < 4; i++) {
      // Check for non-empty and non-0xEE partitions
      if ((partitions[i].partitionType != 0xEE) && (partitions[i].partitionType != 0x00))
         typeFlag++;
      if (partitions[i].partitionType == 0xEE) {
         // Blank space before this partition; fill it....
         if (IsFree(partitions[i].firstLBA - 1)) {
            partitions[i].firstLBA = FindFirstInFree(partitions[i].firstLBA - 1);
         } // if
         // Blank space after this partition; fill it....
         after = partitions[i].firstLBA + partitions[i].lengthLBA;
         if (IsFree(after)) {
            partitions[i].lengthLBA = FindLastInFree(after) - partitions[i].firstLBA + 1;
         } // if free space after
         if (after > diskSize) {
            if (diskSize < UINT32_MAX) { // If the disk is under 2TiB
               partitions[0].lengthLBA = (uint32_t) diskSize - partitions[i].firstLBA;
            } else { // disk is too big to represent, so fake it...
               partitions[0].lengthLBA = UINT32_MAX - partitions[i].firstLBA;
            } // if/else
         } // if protective partition is too big
         RecomputeCHS(i);
      } // if partition is 0xEE
   } // for partition loop
   if (typeFlag == 0) { // No non-hybrid partitions found
      MakeProtectiveMBR(); // ensure it's a fully compliant protective MBR.
   } // if
} // MBRData::OptimizeEESize()

// Delete a partition if one exists at the specified location.
// Returns 1 if a partition was deleted, 0 otherwise....
// Used to help keep GPT & hybrid MBR partitions in sync....
int MBRData::DeleteByLocation(uint64_t start64, uint64_t length64) {
   uint32_t start32, length32;
   int i, deleted = 0;

   if ((start64 < UINT32_MAX) && (length64 < UINT32_MAX)) {
      start32 = (uint32_t) start64;
      length32 = (uint32_t) length64;
      for (i = 0; i < MAX_MBR_PARTS; i++) {
         if ((partitions[i].firstLBA == start32) && (partitions[i].lengthLBA == length32) &&
            (partitions[i].partitionType != 0xEE)) {
            DeletePartition(i);
         if (state == hybrid)
            OptimizeEESize();
         deleted = 1;
         } // if (match found)
      } // for i (partition scan)
   } // if (hybrid & GPT partition < 2TiB)
   return deleted;
} // MBRData::DeleteByLocation()

/******************************************************
 *                                                    *
 * Functions that extract data on specific partitions *
 *                                                    *
 ******************************************************/

// Return the MBR data as a GPT partition....
GPTPart MBRData::AsGPT(int i) {
   MBRRecord* origPart;
   GPTPart newPart;
   uint8_t origType;
   uint64_t firstSector, lastSector;

   newPart.BlankPartition();
   origPart = GetPartition(i);
   if (origPart != NULL) {
      origType = origPart->partitionType;

      // don't convert extended, hybrid protective, or null (non-existent)
      // partitions (Note similar protection is in GPTData::XFormPartitions(),
      // but I want it here too in case I call this function in another
      // context in the future....)
      if ((origType != 0x05) && (origType != 0x0f) && (origType != 0x85) &&
          (origType != 0x00) && (origType != 0xEE)) {
         firstSector = (uint64_t) origPart->firstLBA;
         newPart.SetFirstLBA(firstSector);
         lastSector = firstSector + (uint64_t) origPart->lengthLBA;
         if (lastSector > 0) lastSector--;
         newPart.SetLastLBA(lastSector);
         newPart.SetType(((uint16_t) origType) * 0x0100);
         newPart.RandomizeUniqueGUID();
         newPart.SetAttributes(0);
         newPart.SetName(newPart.GetTypeName());
      } // if not extended, protective, or non-existent
   } // if (origPart != NULL)
   return newPart;
} // MBRData::AsGPT()

