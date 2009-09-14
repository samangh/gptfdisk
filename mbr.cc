/* mbr.cc -- Functions for loading, saving, and manipulating legacy MBR partition
   data. */

/* By Rod Smith, January to February, 2009 */

/* This program is copyright (c) 2009 by Roderick W. Smith. It is distributed
  under the terms of the GNU GPL version 2, as detailed in the COPYING file. */

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include "mbr.h"
#include "support.h"

using namespace std;

/****************************************
 *                                      *
 * MBRData class and related structures *
 *                                      *
 ****************************************/

MBRData::MBRData(void) {
   blockSize = SECTOR_SIZE;
   diskSize = 0;
   strcpy(device, "");
   state = invalid;
   srand((unsigned int) time(NULL));
   EmptyMBR();
} // MBRData default constructor

MBRData::MBRData(char *filename) {
   blockSize = SECTOR_SIZE;
   diskSize = 0;
   strcpy(device, filename);
   state = invalid;

   srand((unsigned int) time(NULL));
   // Try to read the specified partition table, but if it fails....
   if (!ReadMBRData(filename)) {
      EmptyMBR();
      strcpy(device, "");
   } // if
} // MBRData(char *filename) constructor

MBRData::~MBRData(void) {
} // MBRData destructor

// Empty all data. Meant mainly for calling by constructors, but it's also
// used by the hybrid MBR functions in the GPTData class.
void MBRData::EmptyMBR(int clearBootloader) {
   int i;

   // Zero out the boot loader section, the disk signature, and the
   // 2-byte nulls area only if requested to do so. (This is the
   // default.)
   if (clearBootloader == 1) {
      for (i = 0; i < 440; i++)
         code[i] = 0;
      diskSignature = (uint32_t) rand();
      nulls = 0;
   } // if

   // Blank out the partitions
   for (i = 0; i < 4; i++) {
      partitions[i].status = UINT8_C(0);
      partitions[i].firstSector[0] = UINT8_C(0);
      partitions[i].firstSector[1] = UINT8_C(0);
      partitions[i].firstSector[2] = UINT8_C(0);
      partitions[i].partitionType = UINT8_C(0);
      partitions[i].lastSector[0] = UINT8_C(0);
      partitions[i].lastSector[1] = UINT8_C(0);
      partitions[i].lastSector[2] = UINT8_C(0);
      partitions[i].firstLBA = UINT32_C(0);
      partitions[i].lengthLBA = UINT32_C(0);
   } // for
   MBRSignature = MBR_SIGNATURE;

   blockSize = SECTOR_SIZE;
   diskSize = 0;
   for (i = 0; i < NUM_LOGICALS; i++) {
      logicals[i].status = UINT8_C(0);
      logicals[i].firstSector[0] = UINT8_C(0);
      logicals[i].firstSector[1] = UINT8_C(0);
      logicals[i].firstSector[2] = UINT8_C(0);
      logicals[i].partitionType = UINT8_C(0);
      logicals[i].lastSector[0] = UINT8_C(0);
      logicals[i].lastSector[1] = UINT8_C(0);
      logicals[i].lastSector[2] = UINT8_C(0);
      logicals[i].firstLBA = UINT32_C(0);
      logicals[i].lengthLBA = UINT32_C(0);
   } // for
} // MBRData::EmptyMBR()

// Read data from MBR. Returns 1 if read was successful (even if the
// data isn't a valid MBR), 0 if the read failed.
int MBRData::ReadMBRData(char* deviceFilename) {
   int fd, allOK = 1;

   if ((fd = open(deviceFilename, O_RDONLY)) != -1) {
      ReadMBRData(fd);
   } else {
      allOK = 0;
   } // if

   close(fd);

   if (allOK)
      strcpy(device, deviceFilename);

   return allOK;
} // MBRData::ReadMBRData(char* deviceFilename)

// Read data from MBR.
void MBRData::ReadMBRData(int fd, int checkBlockSize) {
   int allOK = 1, i, j, maxLogicals = 0;
   int err;
   TempMBR tempMBR;

   // Clear logical partition array
   for (i = 0; i < NUM_LOGICALS; i++) {
      logicals[i].status = UINT8_C(0);
      logicals[i].firstSector[0] = UINT8_C(0);
      logicals[i].firstSector[1] = UINT8_C(0);
      logicals[i].firstSector[2] = UINT8_C(0);
      logicals[i].partitionType = UINT8_C(0);
      logicals[i].lastSector[0] = UINT8_C(0);
      logicals[i].lastSector[1] = UINT8_C(0);
      logicals[i].lastSector[2] = UINT8_C(0);
      logicals[i].firstLBA = UINT32_C(0);
      logicals[i].lengthLBA = UINT32_C(0);
   } // for

   err = lseek64(fd, 0, SEEK_SET);
   err = read(fd, &tempMBR, 512);
   for (i = 0; i < 440; i++)
      code[i] = tempMBR.code[i];
   diskSignature = tempMBR.diskSignature;
   nulls = tempMBR.nulls;
   for (i = 0; i < 4; i++) {
      partitions[i].status = tempMBR.partitions[i].status;
      partitions[i].partitionType = tempMBR.partitions[i].partitionType;
      partitions[i].firstLBA = tempMBR.partitions[i].firstLBA;
      partitions[i].lengthLBA = tempMBR.partitions[i].lengthLBA;
      for (j = 0; j < 3; j++) {
         partitions[i].firstSector[j] = tempMBR.partitions[i].firstSector[j];
         partitions[i].lastSector[j] = tempMBR.partitions[i].lastSector[j];
      } // for j...
   } // for i...
   MBRSignature = tempMBR.MBRSignature;

   // Reverse the byte order, if necessary
   if (IsLittleEndian() == 0) {
      ReverseBytes(&diskSignature, 4);
      ReverseBytes(&nulls, 2);
      ReverseBytes(&MBRSignature, 2);
      for (i = 0; i < 4; i++) {
         ReverseBytes(&partitions[i].firstLBA, 4);
         ReverseBytes(&partitions[i].lengthLBA, 4);
      } // for
   } // if

   if (MBRSignature != MBR_SIGNATURE) {
      allOK = 0;
      state = invalid;
   } // if

   // Find disk size
   diskSize = disksize(fd, &err);

   // Find block size
   if (checkBlockSize) {
      blockSize = GetBlockSize(fd);
//      if ((blockSize = GetBlockSize(fd)) == -1) {
//         blockSize = SECTOR_SIZE;
//         printf("Unable to determine sector size; assuming %lu bytes!\n",
//               (unsigned long) SECTOR_SIZE);
//      } // if
   } // if (checkBlockSize)

   // Load logical partition data, if any is found....
   if (allOK) {
      for (i = 0; i < 4; i++) {
         if ((partitions[i].partitionType == 0x05) || (partitions[i].partitionType == 0x0f)
             || (partitions[i].partitionType == 0x85)) {
            // Found it, so call a recursive algorithm to load everything from them....
            maxLogicals = ReadLogicalPart(fd, partitions[i].firstLBA, UINT32_C(0), maxLogicals);
            if ((maxLogicals < 0) || (maxLogicals > NUM_LOGICALS)) {
               allOK = 0;
               fprintf(stderr, "Error reading logical partitions! List may be truncated!\n");
            } // if maxLogicals valid
         } // if primary partition is extended
      } // for primary partition loop
      if (allOK) { // Loaded logicals OK
         state = mbr;
      } else {
         state = invalid;
      } // if
   } // if

   /* Check to see if it's in GPT format.... */
   if (allOK) {
      for (i = 0; i < 4; i++) {
         if (partitions[i].partitionType == UINT8_C(0xEE)) {
            state = gpt;
         } // if
      } // for
   } // if

   // If there's an EFI GPT partition, look for other partition types,
   // to flag as hybrid
   if (state == gpt) {
      for (i = 0 ; i < 4; i++) {
         if ((partitions[i].partitionType != UINT8_C(0xEE)) &&
             (partitions[i].partitionType != UINT8_C(0x00)))
            state = hybrid;
      } // for
   } // if hybrid

/*   // Tell the user what the MBR state is...
   switch (state) {
      case invalid:
         printf("Information: MBR appears to be empty or invalid.\n");
	 break;
      case gpt:
         printf("Information: MBR holds GPT placeholder partitions.\n");
         break;
      case hybrid:
         printf("Information: MBR holds hybrid GPT/MBR data.\n");
         break;
      case mbr:
         printf("Information: MBR data appears to be valid.\n");
         break;
   } // switch */
} // MBRData::ReadMBRData(int fd)

// Write the MBR data to the default defined device.
int MBRData::WriteMBRData(void) {
   int allOK = 1, fd;

   if ((fd = open(device, O_WRONLY | O_CREAT, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH)) != -1) {
      WriteMBRData(fd);
   } else {
      allOK = 0;
   } // if/else
   close(fd);
   return allOK;
} // MBRData::WriteMBRData(void)

// Save the MBR data to a file. Note that this function writes ONLY the
// MBR data, not the logical partitions (if any are defined).
void MBRData::WriteMBRData(int fd) {
   int i, j;
   TempMBR tempMBR;

   // Reverse the byte order, if necessary
   if (IsLittleEndian() == 0) {
      ReverseBytes(&diskSignature, 4);
      ReverseBytes(&nulls, 2);
      ReverseBytes(&MBRSignature, 2);
      for (i = 0; i < 4; i++) {
         ReverseBytes(&partitions[i].firstLBA, 4);
         ReverseBytes(&partitions[i].lengthLBA, 4);
      } // for
   } // if

   // Copy MBR data to a 512-byte data structure for writing, to
   // work around a FreeBSD limitation....
   for (i = 0; i < 440; i++)
      tempMBR.code[i] = code[i];
   tempMBR.diskSignature = diskSignature;
   tempMBR.nulls = nulls;
   tempMBR.MBRSignature = MBRSignature;
   for (i = 0; i < 4; i++) {
      tempMBR.partitions[i].status = partitions[i].status;
      tempMBR.partitions[i].partitionType = partitions[i].partitionType;
      tempMBR.partitions[i].firstLBA = partitions[i].firstLBA;
      tempMBR.partitions[i].lengthLBA = partitions[i].lengthLBA;
      for (j = 0; j < 3; j++) {
         tempMBR.partitions[i].firstSector[j] = partitions[i].firstSector[j];
         tempMBR.partitions[i].lastSector[j] = partitions[i].lastSector[j];
      } // for j...
   } // for i...

   // Now write that data structure...
   lseek64(fd, 0, SEEK_SET);
   write(fd, &tempMBR, 512);

/*   write(fd, code, 440);
   write(fd, &diskSignature, 4);
   write(fd, &nulls, 2);
   write(fd, partitions, 64);
   write(fd, &MBRSignature, 2); */

   // Reverse the byte order back, if necessary
   if (IsLittleEndian() == 0) {
      ReverseBytes(&diskSignature, 4);
      ReverseBytes(&nulls, 2);
      ReverseBytes(&MBRSignature, 2);
      for (i = 0; i < 4; i++) {
         ReverseBytes(&partitions[i].firstLBA, 4);
         ReverseBytes(&partitions[i].lengthLBA, 4);
      } // for
   }// if
} // MBRData::WriteMBRData(int fd)

// This is a recursive function to read all the logical partitions, following the
// logical partition linked list from the disk and storing the basic data in the
// logicals[] array. Returns last index to logicals[] uses, or -1 if there was a
// problem
int MBRData::ReadLogicalPart(int fd, uint32_t extendedStart,
                             uint32_t diskOffset, int partNum) {
   struct EBRRecord ebr;
   off_t offset;

   if ((partNum < NUM_LOGICALS) && (partNum >= 0)) {
      offset = (off_t) (extendedStart + diskOffset) * blockSize;
      if (lseek64(fd, offset, SEEK_SET) == (off_t) -1) { // seek to EBR record
         fprintf(stderr, "Unable to seek to %lu! Aborting!\n", (unsigned long) offset);
         partNum = -1;
      }
      if (read(fd, &ebr, 512) != 512) { // Load the data....
         fprintf(stderr, "Error seeking to or reading logical partition data from %lu!\nAborting!\n",
                 (unsigned long) offset);
         partNum = -1;
      } else if (IsLittleEndian() != 1) { // Reverse byte ordering of some data....
         ReverseBytes(&ebr.MBRSignature, 2);
         ReverseBytes(&ebr.partitions[0].firstLBA, 4);
         ReverseBytes(&ebr.partitions[0].lengthLBA, 4);
         ReverseBytes(&ebr.partitions[1].firstLBA, 4);
         ReverseBytes(&ebr.partitions[1].lengthLBA, 4);
      } // if/else/if

      if (ebr.MBRSignature != MBR_SIGNATURE) {
         partNum = -1;
         fprintf(stderr, "MBR signature in logical partition invalid; read 0x%04X, but should be 0x%04X\n",
                (unsigned int) ebr.MBRSignature, (unsigned int) MBR_SIGNATURE);
      } // if

      // Copy over the basic data....
      logicals[partNum].status = ebr.partitions[0].status;
      logicals[partNum].firstLBA = ebr.partitions[0].firstLBA + diskOffset + extendedStart;
      logicals[partNum].lengthLBA = ebr.partitions[0].lengthLBA;
      logicals[partNum].partitionType = ebr.partitions[0].partitionType;

      // Find the next partition (if there is one) and recurse....
      if ((ebr.partitions[1].firstLBA != UINT32_C(0)) && (partNum >= 0) &&
          (partNum < (NUM_LOGICALS - 1))) {
         partNum = ReadLogicalPart(fd, extendedStart, ebr.partitions[1].firstLBA,
                                   partNum + 1);
         } else {
            partNum++;
      } // if another partition
   } // Not enough space for all the logicals (or previous error encountered)
   return (partNum);
} // MBRData::ReadLogicalPart()

// Show the MBR data to the user....
void MBRData::DisplayMBRData(void) {
   int i;
   char tempStr[255];
   char bootCode;

   printf("MBR disk identifier: 0x%08X\n", (unsigned int) diskSignature);
   printf("MBR partitions:\n");
   printf("Number\t Boot\t Start (sector)\t Length (sectors)\tType\n");
   for (i = 0; i < 4; i++) {
      if (partitions[i].lengthLBA != 0) {
         if (partitions[i].status && 0x80) // it's bootable
	    bootCode = '*';
         else
            bootCode = ' ';
         printf("%4d\t   %c\t%13lu\t%15lu \t0x%02X\n", i + 1, bootCode,
                (unsigned long) partitions[i].firstLBA,
                (unsigned long) partitions[i].lengthLBA, partitions[i].partitionType);
      } // if
   } // for

   // Now display logical partition data....
   for (i = 0; i < NUM_LOGICALS; i++) {
      if (logicals[i].lengthLBA != 0) {
         printf("%4d\t%13lu\t%15lu \t0x%02X\n", i + 5, (unsigned long) logicals[i].firstLBA,
                (unsigned long) logicals[i].lengthLBA, logicals[i].partitionType);
      } // if
   } // for
   printf("\nDisk size is %lu sectors (%s)\n", (unsigned long) diskSize,
          BytesToSI(diskSize * (uint64_t) blockSize, tempStr));
} // MBRData::DisplayMBRData()

// Create a protective MBR. Clears the boot loader area if clearBoot > 0.
void MBRData::MakeProtectiveMBR(int clearBoot) {
   int i;

   // Initialize variables
   nulls = 0;
   MBRSignature = MBR_SIGNATURE;

   if (clearBoot > 0) {
      for (i = 0; i < 440; i++)
         code[i] = (uint8_t) 0;
   } // if

   partitions[0].status = UINT8_C(0); // Flag the protective part. as unbootable

   // Write CHS data. This maxes out the use of the disk, as much as
   // possible -- even to the point of exceeding the capacity of sub-8GB
   // disks. The EFI spec says to use 0xffffff as the ending value,
   // although normal MBR disks max out at 0xfeffff. FWIW, both GNU Parted
   // and Apple's Disk Utility use 0xfeffff, and the latter puts that
   // value in for the FIRST sector, too!
   partitions[0].firstSector[0] = UINT8_C(0);
   partitions[0].firstSector[1] = UINT8_C(1);
   partitions[0].firstSector[2] = UINT8_C(0);
   partitions[0].lastSector[0] = UINT8_C(255);
   partitions[0].lastSector[1] = UINT8_C(255);
   partitions[0].lastSector[2] = UINT8_C(255);

   partitions[0].partitionType = UINT8_C(0xEE);
   partitions[0].firstLBA = UINT32_C(1);
   if (diskSize < UINT32_MAX) { // If the disk is under 2TiB
      partitions[0].lengthLBA = diskSize - 1;
   } else { // disk is too big to represent, so fake it...
      partitions[0].lengthLBA = UINT32_MAX;
   } // if/else

   // Zero out three unused primary partitions...
   for (i = 1; i < 4; i++) {
      partitions[i].status = UINT8_C(0);
      partitions[i].firstSector[0] = UINT8_C(0);
      partitions[i].firstSector[1] = UINT8_C(0);
      partitions[i].firstSector[2] = UINT8_C(0);
      partitions[i].partitionType = UINT8_C(0);
      partitions[i].lastSector[0] = UINT8_C(0);
      partitions[i].lastSector[1] = UINT8_C(0);
      partitions[i].lastSector[2] = UINT8_C(0);
      partitions[i].firstLBA = UINT32_C(0);
      partitions[i].lengthLBA = UINT32_C(0);
   } // for

   // Zero out all the logical partitions. Not necessary for data
   // integrity on write, but eliminates stray entries if user wants
   // to view the MBR after converting the disk
   for (i = 0; i < NUM_LOGICALS; i++) {
      logicals[i].status = UINT8_C(0);
      logicals[i].firstSector[0] = UINT8_C(0);
      logicals[i].firstSector[1] = UINT8_C(0);
      logicals[i].firstSector[2] = UINT8_C(0);
      logicals[i].partitionType = UINT8_C(0);
      logicals[i].lastSector[0] = UINT8_C(0);
      logicals[i].lastSector[1] = UINT8_C(0);
      logicals[i].lastSector[2] = UINT8_C(0);
      logicals[i].firstLBA = UINT32_C(0);
      logicals[i].lengthLBA = UINT32_C(0);
   } // for

   state = gpt;
} // MBRData::MakeProtectiveMBR()

// Create a partition that fills the most available space. Returns
// 1 if partition was created, 0 otherwise. Intended for use in
// creating hybrid MBRs.
int MBRData::MakeBiggestPart(int i, int type) {
   uint32_t start = UINT32_C(1); // starting point for each search
   uint32_t firstBlock; // first block in a segment
   uint32_t lastBlock; // last block in a segment
   uint32_t segmentSize; // size of segment in blocks
   uint32_t selectedSegment = UINT32_C(0); // location of largest segment
   uint32_t selectedSize = UINT32_C(0); // size of largest segment in blocks
   int found = 0;

   do {
      firstBlock = FindFirstAvailable(start);
      if (firstBlock != UINT32_C(0)) { // something's free...
         lastBlock = FindLastInFree(firstBlock);
         segmentSize = lastBlock - firstBlock + UINT32_C(1);
	 if (segmentSize > selectedSize) {
            selectedSize = segmentSize;
	    selectedSegment = firstBlock;
	 } // if
	 start = lastBlock + 1;
      } // if
   } while (firstBlock != 0);
   if ((selectedSize > UINT32_C(0)) && ((uint64_t) selectedSize < diskSize)) {
      found = 1;
      MakePart(i, selectedSegment, selectedSize, type, 0);
   } else {
      found = 0;
   } // if/else
   return found;
} // MBRData::MakeBiggestPart(int i)

// Delete partition #i
void MBRData::DeletePartition(int i) {
   int j;

   partitions[i].firstLBA = UINT32_C(0);
   partitions[i].lengthLBA = UINT32_C(0);
   partitions[i].status = UINT8_C(0);
   partitions[i].partitionType = UINT8_C(0);
   for (j = 0; j < 3; j++) {
      partitions[i].firstSector[j] = UINT8_C(0);
      partitions[i].lastSector[j] = UINT8_C(0);
   } // for j (CHS data blanking)
} // MBRData::DeletePartition()

// Delete a partition if one exists at the specified location.
// Returns 1 if a partition was deleted, 0 otherwise....
// Used to help keep GPT & hybrid MBR partitions in sync....
int MBRData::DeleteByLocation(uint64_t start64, uint64_t length64) {
   uint32_t start32, length32;
   int i, j, deleted = 0;

   if ((state == hybrid) && (start64 < UINT32_MAX) && (length64 < UINT32_MAX)) {
      start32 = (uint32_t) start64;
      length32 = (uint32_t) length64;
      for (i = 0; i < 4; i++) {
         if ((partitions[i].firstLBA == start32) && (partitions[i].lengthLBA = length32) &&
             (partitions[i].partitionType != 0xEE)) {
            DeletePartition(i);
            OptimizeEESize();
            deleted = 1;
         } // if (match found)
      } // for i (partition scan)
   } // if (hybrid & GPT partition < 2TiB)
   return deleted;
} // MBRData::DeleteByLocation()

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
      } // if partition is 0xEE
      if (typeFlag == 0) { // No non-hybrid partitions found
         MakeProtectiveMBR(); // ensure it's a fully compliant hybrid MBR.
      } // if
   } // for partition loop
} // MBRData::OptimizeEESize()

// Return a pointer to a primary or logical partition, or NULL if
// the partition is out of range....
struct MBRRecord* MBRData::GetPartition(int i) {
   MBRRecord* thePart = NULL;

   if ((i >= 0) && (i < 4)) { // primary partition
      thePart = &partitions[i];
   } // if
   if ((i >= 4) && (i < (NUM_LOGICALS + 4))) {
      thePart = &logicals[i - 4];
   } // if
   return thePart;
} // GetPartition()

// Displays the state, as a word, on stdout. Used for debugging & to
// tell the user about the MBR state when the program launches....
void MBRData::ShowState(void) {
   switch (state) {
      case invalid:
         printf("  MBR: not present\n");
         break;
      case gpt:
         printf("  MBR: protective\n");
         break;
      case hybrid:
         printf("  MBR: hybrid\n");
         break;
      case mbr:
         printf("  MBR: MBR only\n");
         break;
      default:
         printf("\a  MBR: unknown -- bug!\n");
         break;
   } // switch
} // MBRData::ShowState()

// Create a primary partition of the specified number, starting LBA,
// and length. This function does *NO* error checking, so it's possible
// to seriously screw up a partition table using this function! It's
// intended as a way to create a hybrid MBR, which is a pretty funky
// setup to begin with....
void MBRData::MakePart(int num, uint32_t start, uint32_t length, int type,
                       int bootable) {

   partitions[num].status = (uint8_t) bootable * (uint8_t) 0x80;
   partitions[num].firstSector[0] = UINT8_C(0);
   partitions[num].firstSector[1] = UINT8_C(0);
   partitions[num].firstSector[2] = UINT8_C(0);
   partitions[num].partitionType = (uint8_t) type;
   partitions[num].lastSector[0] = UINT8_C(0);
   partitions[num].lastSector[1] = UINT8_C(0);
   partitions[num].lastSector[2] = UINT8_C(0);
   partitions[num].firstLBA = start;
   partitions[num].lengthLBA = length;
} // MakePart()

// Finds the first free space on the disk from start onward; returns 0
// if none available....
uint32_t MBRData::FindFirstAvailable(uint32_t start) {
   uint32_t first;
   uint32_t i;
   int firstMoved = 0;

   first = start;

   // ...now search through all partitions; if first is within an
   // existing partition, move it to the next sector after that
   // partition and repeat. If first was moved, set firstMoved
   // flag; repeat until firstMoved is not set, so as to catch
   // cases where partitions are out of sequential order....
   do {
      firstMoved = 0;
      for (i = 0; i < 4; i++) {
         // Check if it's in the existing partition
         if ((first >= partitions[i].firstLBA) &&
             (first < (partitions[i].firstLBA + partitions[i].lengthLBA))) {
            first = partitions[i].firstLBA + partitions[i].lengthLBA;
            firstMoved = 1;
          } // if
      } // for
   } while (firstMoved == 1);
   if (first >= diskSize)
      first = 0;
   return (first);
} // MBRData::FindFirstAvailable()

// Finds the last free sector on the disk from start forward.
uint32_t MBRData::FindLastInFree(uint32_t start) {
   uint32_t nearestStart;
   uint32_t i;

   if (diskSize <= UINT32_MAX)
      nearestStart = diskSize - 1;
   else
      nearestStart = UINT32_MAX - 1;
   for (i = 0; i < 4; i++) {
      if ((nearestStart > partitions[i].firstLBA) &&
          (partitions[i].firstLBA > start)) {
         nearestStart = partitions[i].firstLBA - 1;
      } // if
   } // for
   return (nearestStart);
} // MBRData::FindLastInFree()

// Finds the first free sector on the disk from start backward.
uint32_t MBRData::FindFirstInFree(uint32_t start) {
   uint32_t bestLastLBA, thisLastLBA;
   int i;

   bestLastLBA = 1;
   for (i = 0; i < 4; i++) {
      thisLastLBA = partitions[i].firstLBA + partitions[i].lengthLBA;
      if (thisLastLBA > 0) thisLastLBA--;
      if ((thisLastLBA > bestLastLBA) && (thisLastLBA < start)) {
         bestLastLBA = thisLastLBA + 1;
      } // if
   } // for
   return (bestLastLBA);
} // MBRData::FindFirstInFree()

// Returns 1 if the specified sector is unallocated, 0 if it's
// allocated.
int MBRData::IsFree(uint32_t sector) {
   int i, isFree = 1;
   uint32_t first, last;

   for (i = 0; i < 4; i++) {
      first = partitions[i].firstLBA;
      last = first + partitions[i].lengthLBA;
      if (last > 0) last--;
      if ((first <= sector) && (last >= sector))
         isFree = 0;
   } // for
   return isFree;
} // MBRData::IsFree()

uint8_t MBRData::GetStatus(int i) {
   MBRRecord* thePart;
   uint8_t retval;

   thePart = GetPartition(i);
   if (thePart != NULL)
      retval = thePart->status;
   else
      retval = UINT8_C(0);
   return retval;
} // MBRData::GetStatus()

uint8_t MBRData::GetType(int i) {
   MBRRecord* thePart;
   uint8_t retval;

   thePart = GetPartition(i);
   if (thePart != NULL)
      retval = thePart->partitionType;
   else
      retval = UINT8_C(0);
   return retval;
} // MBRData::GetType()

uint32_t MBRData::GetFirstSector(int i) {
   MBRRecord* thePart;
   uint32_t retval;

   thePart = GetPartition(i);
   if (thePart != NULL) {
      retval = thePart->firstLBA;
   } else
      retval = UINT32_C(0);
   return retval;
} // MBRData::GetFirstSector()

uint32_t MBRData::GetLength(int i) {
   MBRRecord* thePart;
   uint32_t retval;

   thePart = GetPartition(i);
   if (thePart != NULL) {
      retval = thePart->lengthLBA;
   } else
      retval = UINT32_C(0);
   return retval;
} // MBRData::GetLength()

// Return the MBR data as a GPT partition....
GPTPart MBRData::AsGPT(int i) {
   MBRRecord* origPart;
   GPTPart newPart;
   uint8_t origType;
   uint64_t firstSector, lastSector;
   char tempStr[NAME_SIZE];

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
         newPart.SetUniqueGUID(1);
         newPart.SetAttributes(0);
         newPart.SetName((unsigned char*) newPart.GetNameType(tempStr));
      } // if
   } // if
   return newPart;
} // MBRData::AsGPT()
