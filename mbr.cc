/* mbr.cc -- Functions for loading, saving, and manipulating legacy MBR partition
   data. */

/* By Rod Smith, January to February, 2009 */

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
void MBRData::ReadMBRData(int fd) {
   int allOK = 1, i;
   int err;

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

   read(fd, code, 440);
   read(fd, &diskSignature, 4);
   read(fd, &nulls, 2);
   read(fd, partitions, 64);
   read(fd, &MBRSignature, 2);
   if (MBRSignature != MBR_SIGNATURE) {
      allOK = 0;
      state = invalid;
      fprintf(stderr, "MBR signature invalid; read 0x%04X, but should be 0x%04X\n",
              (unsigned int) MBRSignature, (unsigned int) MBR_SIGNATURE);
   } /* if */

   // Find disk size
   diskSize = disksize(fd, &err);

   // Find block size
   if ((blockSize = GetBlockSize(fd)) == -1) {
      blockSize = SECTOR_SIZE;
      printf("Unable to determine sector size; assuming %lu bytes!\n",
             (unsigned long) SECTOR_SIZE);
   } // if

   // Load logical partition data, if any is found....
   if (allOK) {
      for (i = 0; i < 4; i++) {
         if ((partitions[i].partitionType == 0x05) || (partitions[i].partitionType == 0x0f)
             || (partitions[i].partitionType == 0x85)) {
            // Found it, so call a recursive algorithm to load everything from them....
            allOK = ReadLogicalPart(fd, partitions[i].firstLBA, UINT32_C(0), 0);
         } // if
      } // for
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
         } /* if */
      } /* for */
   } /* if */

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
   write(fd, code, 440);
   write(fd, &diskSignature, 4);
   write(fd, &nulls, 2);
   write(fd, partitions, 64);
   write(fd, &MBRSignature, 2);
} // MBRData::WriteMBRData(int fd)

// This is a recursive function to read all the logical partitions, following the
// logical partition linked list from the disk and storing the basic data in 
int MBRData::ReadLogicalPart(int fd, uint32_t extendedStart,
                             uint32_t diskOffset, int partNum) {
   int allOK = 1;
   struct EBRRecord ebr;
   off_t offset;

   offset = (off_t) (extendedStart + diskOffset) * blockSize;
   if (lseek64(fd, offset, SEEK_SET) == (off_t) -1) { // seek to EBR record
      fprintf(stderr, "Unable to seek to %lu! Aborting!\n", (unsigned long) offset);
      allOK = 0;
   }
   if (read(fd, &ebr, 512) != 512) { // Load the data....
      fprintf(stderr, "Error seeking to or reading logical partition data from %lu!\nAborting!\n",
              (unsigned long) offset);
      allOK = 0;
   }
   if (ebr.MBRSignature != MBR_SIGNATURE) {
      allOK = 0;
      printf("MBR signature in logical partition invalid; read 0x%04X, but should be 0x%04X\n",
	     (unsigned int) ebr.MBRSignature, (unsigned int) MBR_SIGNATURE);
   } /* if */

   // Copy over the basic data....
   logicals[partNum].status = ebr.partitions[0].status;
   logicals[partNum].firstLBA = ebr.partitions[0].firstLBA + diskOffset + extendedStart;
   logicals[partNum].lengthLBA = ebr.partitions[0].lengthLBA;
   logicals[partNum].partitionType = ebr.partitions[0].partitionType;

   // Find the next partition (if there is one) and recurse....
   if ((ebr.partitions[1].firstLBA != UINT32_C(0)) && allOK) {
      allOK = ReadLogicalPart(fd, extendedStart, ebr.partitions[1].firstLBA,
                              partNum + 1);
   } // if
   return (allOK);
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

// Create a protective MBR
void MBRData::MakeProtectiveMBR(void) {
   int i;

   // Initialize variables
   nulls = 0;
   MBRSignature = MBR_SIGNATURE;

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

// Displays the state, as a word, on stdout. Used for debugging
void MBRData::ShowState(void) {
   switch (state) {
      case invalid:
         printf("invalid");
         break;
      case gpt:
         printf("gpt");
         break;
      case hybrid:
         printf("hybrid");
         break;
      case mbr:
         printf("mbr");
         break;
      default:
         printf("unknown -- bug!");
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
} // MBRData::FindLastInFree

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
