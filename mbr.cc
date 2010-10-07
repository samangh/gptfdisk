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
#include "partnotes.h"
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
   device = "";
   state = invalid;
   srand((unsigned int) time(NULL));
   numHeads = MAX_HEADS;
   numSecspTrack = MAX_SECSPERTRACK;
   myDisk = NULL;
   canDeleteMyDisk = 0;
   EmptyMBR();
} // MBRData default constructor

MBRData::MBRData(string filename) {
   blockSize = SECTOR_SIZE;
   diskSize = 0;
   device = filename;
   state = invalid;
   numHeads = MAX_HEADS;
   numSecspTrack = MAX_SECSPERTRACK;
   myDisk = NULL;
   canDeleteMyDisk = 0;

   srand((unsigned int) time(NULL));
   // Try to read the specified partition table, but if it fails....
   if (!ReadMBRData(filename)) {
      EmptyMBR();
      device = "";
   } // if
} // MBRData(string filename) constructor

// Free space used by myDisk only if that's OK -- sometimes it will be
// copied from an outside source, in which case that source should handle
// it!
MBRData::~MBRData(void) {
   if (canDeleteMyDisk)
      delete myDisk;
} // MBRData destructor

/**********************
 *                    *
 * Disk I/O functions *
 *                    *
 **********************/

// Read data from MBR. Returns 1 if read was successful (even if the
// data isn't a valid MBR), 0 if the read failed.
int MBRData::ReadMBRData(const string & deviceFilename) {
   int allOK = 1;

   if (myDisk == NULL) {
      myDisk = new DiskIO;
      canDeleteMyDisk = 1;
   } // if
   if (myDisk->OpenForRead(deviceFilename)) {
      allOK = ReadMBRData(myDisk);
   } else {
      allOK = 0;
   } // if

   if (allOK)
      device = deviceFilename;

   return allOK;
} // MBRData::ReadMBRData(const string & deviceFilename)

// Read data from MBR. If checkBlockSize == 1 (the default), the block
// size is checked; otherwise it's set to the default (512 bytes).
// Note that any extended partition(s) present will be explicitly stored
// in the partitions[] array, along with their contained partitions; the
// extended container partition(s) should be ignored by other functions.
int MBRData::ReadMBRData(DiskIO * theDisk, int checkBlockSize) {
   int allOK = 1, i, j, logicalNum;
   int err = 1;
   TempMBR tempMBR;

   if ((myDisk != NULL) && (canDeleteMyDisk)) {
      delete myDisk;
      canDeleteMyDisk = 0;
   } // if

   myDisk = theDisk;

   // Empty existing MBR data, including the logical partitions...
   EmptyMBR(0);

   if (myDisk->Seek(0))
     if (myDisk->Read(&tempMBR, 512))
        err = 0;
   if (err) {
      cerr << "Problem reading disk in MBRData::ReadMBRData()!\n";
   } else {
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
         } // for j... (reading parts of CHS geometry)
      } // for i... (reading all four partitions)
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
      diskSize = myDisk->DiskSize(&err);

      // Find block size
      if (checkBlockSize) {
         blockSize = myDisk->GetBlockSize();
      } // if (checkBlockSize)

      // Load logical partition data, if any is found....
      if (allOK) {
         for (i = 0; i < 4; i++) {
            if ((partitions[i].partitionType == 0x05) || (partitions[i].partitionType == 0x0f)
               || (partitions[i].partitionType == 0x85)) {
               // Found it, so call a recursive algorithm to load everything from them....
               logicalNum = ReadLogicalPart(partitions[i].firstLBA, UINT32_C(0), 4);
               if ((logicalNum < 0) || (logicalNum >= MAX_MBR_PARTS)) {
                  allOK = 0;
                  cerr << "Error reading logical partitions! List may be truncated!\n";
               } // if maxLogicals valid
            } // if primary partition is extended
         } // for primary partition loop
         if (allOK) { // Loaded logicals OK
            state = mbr;
         } else {
            state = invalid;
         } // if
      } // if

      // Check to see if it's in GPT format....
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
      } // if (hybrid detection code)
   } // no initial error
   return allOK;
} // MBRData::ReadMBRData(DiskIO * theDisk, int checkBlockSize)

// This is a recursive function to read all the logical partitions, following the
// logical partition linked list from the disk and storing the basic data in the
// partitions[] array. Returns last index to partitions[] used, or -1 if there was
// a problem.
// Parameters:
// extendedStart = LBA of the start of the extended partition
// diskOffset = LBA offset WITHIN the extended partition of the one to be read
// partNum = location in partitions[] array to store retrieved data
int MBRData::ReadLogicalPart(uint32_t extendedStart, uint32_t diskOffset, int partNum) {
   struct TempMBR ebr;
   uint64_t offset;

   // Check for a valid partition number. Note that partitions MAY be read into
   // the area normally used by primary partitions, although the only calling
   // function as of GPT fdisk version 0.5.0 doesn't do so.
   if ((partNum < MAX_MBR_PARTS) && (partNum >= 0)) {
      offset = (uint64_t) (extendedStart + diskOffset);
      if (myDisk->Seek(offset) == 0) { // seek to EBR record
         cerr << "Unable to seek to " << offset << "! Aborting!\n";
         partNum = -1;
      }
      if (myDisk->Read(&ebr, 512) != 512) { // Load the data....
         cerr << "Error seeking to or reading logical partition data from " << offset
              << "!\nAborting!\n";
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
         cerr << "MBR signature in logical partition invalid; read 0x";
         cerr.fill('0');
         cerr.width(4);
         cerr.setf(ios::uppercase);
         cerr << hex << ebr.MBRSignature << ", but should be 0x";
         cerr.width(4);
         cerr << MBR_SIGNATURE << dec << "\n";
         cerr.fill(' ');
      } // if

      // Copy over the basic data....
      partitions[partNum].status = ebr.partitions[0].status;
      partitions[partNum].firstLBA = ebr.partitions[0].firstLBA + diskOffset + extendedStart;
      partitions[partNum].lengthLBA = ebr.partitions[0].lengthLBA;
      partitions[partNum].partitionType = ebr.partitions[0].partitionType;

      // Find the next partition (if there is one) and recurse....
      if ((ebr.partitions[1].firstLBA != UINT32_C(0)) && (partNum >= 4) &&
          (partNum < (MAX_MBR_PARTS - 1))) {
         partNum = ReadLogicalPart(extendedStart, ebr.partitions[1].firstLBA,
                                   partNum + 1);
      } else {
         partNum++;
      } // if another partition
   } // Not enough space for all the logicals (or previous error encountered)
   return (partNum);
} // MBRData::ReadLogicalPart()

// Write the MBR data to the default defined device. This writes both the
// MBR itself and any defined logical partitions, provided there's an
// MBR extended partition.
int MBRData::WriteMBRData(void) {
   int allOK = 1;

   if (myDisk != NULL) {
      if (myDisk->OpenForWrite() != 0) {
         allOK = WriteMBRData(myDisk);
      } else {
         allOK = 0;
      } // if/else
      myDisk->Close();
   } else allOK = 0;
   return allOK;
} // MBRData::WriteMBRData(void)

// Save the MBR data to a file. This writes both the
// MBR itself and any defined logical partitions, provided there's an
// MBR extended partition.
int MBRData::WriteMBRData(DiskIO *theDisk) {
   int i, j, partNum, allOK, moreLogicals = 0;
   uint32_t extFirstLBA = 0;
   uint64_t writeEbrTo; // 64-bit because we support extended in 2-4TiB range
   TempMBR tempMBR;

   // First write the main MBR data structure....
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
      if (partitions[i].partitionType == 0x0f) {
         extFirstLBA = partitions[i].firstLBA;
         moreLogicals = 1;
      } // if
   } // for i...
   allOK = WriteMBRData(tempMBR, theDisk, 0);

   // Set up tempMBR with some constant data for logical partitions...
   tempMBR.diskSignature = 0;
   for (i = 2; i < 4; i++) {
      tempMBR.partitions[i].firstLBA = tempMBR.partitions[i].lengthLBA = 0;
      tempMBR.partitions[i].partitionType = 0x00;
      for (j = 0; j < 3; j++) {
         tempMBR.partitions[i].firstSector[j] = 0;
         tempMBR.partitions[i].lastSector[j] = 0;
      } // for j
   } // for i

   partNum = 4;
   writeEbrTo = (uint64_t) extFirstLBA;
   // If extended partition is present, write logicals...
   while (allOK && moreLogicals && (partNum < MAX_MBR_PARTS)) {
      tempMBR.partitions[0] = partitions[partNum];
      tempMBR.partitions[0].firstLBA = 1; // partition starts on sector after EBR
      // tempMBR.partitions[1] points to next EBR or terminates EBR linked list...
      if ((partNum < MAX_MBR_PARTS - 1) && (partitions[partNum + 1].firstLBA > 0)) {
         tempMBR.partitions[1].partitionType = 0x0f;
         tempMBR.partitions[1].firstLBA = partitions[partNum + 1].firstLBA - 1;
         tempMBR.partitions[1].lengthLBA = partitions[partNum + 1].lengthLBA + 1;
         LBAtoCHS((uint64_t) tempMBR.partitions[1].firstLBA,
                  (uint8_t *) &tempMBR.partitions[1].firstSector);
         LBAtoCHS(tempMBR.partitions[1].lengthLBA - extFirstLBA,
                  (uint8_t *) &tempMBR.partitions[1].lastSector);
      } else {
         tempMBR.partitions[1].partitionType = 0x00;
         tempMBR.partitions[1].firstLBA = 0;
         tempMBR.partitions[1].lengthLBA = 0;
         moreLogicals = 0;
      } // if/else
      allOK = WriteMBRData(tempMBR, theDisk, writeEbrTo);
      partNum++;
      writeEbrTo = (uint64_t) tempMBR.partitions[1].firstLBA + (uint64_t) extFirstLBA;
   } // while
   return allOK;
} // MBRData::WriteMBRData(DiskIO *theDisk)

int MBRData::WriteMBRData(const string & deviceFilename) {
   device = deviceFilename;
   return WriteMBRData();
} // MBRData::WriteMBRData(const string & deviceFilename)

// Write a single MBR record to the specified sector. Used by the like-named
// function to write both the MBR and multiple EBR (for logical partition)
// records.
// Returns 1 on success, 0 on failure
int MBRData::WriteMBRData(struct TempMBR & mbr, DiskIO *theDisk, uint64_t sector) {
   int i, allOK;

   // Reverse the byte order, if necessary
   if (IsLittleEndian() == 0) {
      ReverseBytes(&mbr.diskSignature, 4);
      ReverseBytes(&mbr.nulls, 2);
      ReverseBytes(&mbr.MBRSignature, 2);
      for (i = 0; i < 4; i++) {
         ReverseBytes(&mbr.partitions[i].firstLBA, 4);
         ReverseBytes(&mbr.partitions[i].lengthLBA, 4);
      } // for
   } // if

   // Now write the data structure...
   allOK = theDisk->OpenForWrite();
   if (allOK && theDisk->Seek(sector)) {
      if (theDisk->Write(&mbr, 512) != 512) {
         allOK = 0;
         cerr << "Error " << errno << " when saving MBR!\n";
      } // if
   } else {
      allOK = 0;
      cerr << "Error " << errno << " when seeking to MBR to write it!\n";
   } // if/else
   theDisk->Close();

   // Reverse the byte order back, if necessary
   if (IsLittleEndian() == 0) {
      ReverseBytes(&mbr.diskSignature, 4);
      ReverseBytes(&mbr.nulls, 2);
      ReverseBytes(&mbr.MBRSignature, 2);
      for (i = 0; i < 4; i++) {
         ReverseBytes(&mbr.partitions[i].firstLBA, 4);
         ReverseBytes(&mbr.partitions[i].lengthLBA, 4);
      } // for
   }// if
   return allOK;
} // MBRData::WriteMBRData(uint64_t sector)

/********************************************
 *                                          *
 * Functions that display data for the user *
 *                                          *
 ********************************************/

// Show the MBR data to the user, up to the specified maximum number
// of partitions....
void MBRData::DisplayMBRData(int maxParts) {
   int i;
   char bootCode;

   if (maxParts > MAX_MBR_PARTS)
      maxParts = MAX_MBR_PARTS;
   cout << "MBR disk identifier: 0x";
   cout.width(8);
   cout.fill('0');
   cout.setf(ios::uppercase);
   cout << hex << diskSignature << dec << "\n";
   cout << "MBR partitions:\n";
   cout << "Number\t Boot\t Start (sector)\t Length (sectors)\tType\n";
   for (i = 0; i < maxParts; i++) {
      if (partitions[i].lengthLBA != 0) {
         if (partitions[i].status && 0x80) // it's bootable
            bootCode = '*';
         else
            bootCode = ' ';
         cout.fill(' ');
         cout.width(4);
         cout << i + 1 << "\t   " << bootCode << "\t";
         cout.width(13);
         cout << partitions[i].firstLBA << "\t";
         cout.width(15);
         cout << partitions[i].lengthLBA << " \t0x";
         cout.width(2);
         cout.fill('0');
         cout << hex << (int) partitions[i].partitionType << dec << "\n";
      } // if
      cout.fill(' ');
   } // for
   cout << "\nDisk size is " << diskSize << " sectors ("
        << BytesToSI(diskSize, blockSize) << ")\n";
} // MBRData::DisplayMBRData()

// Displays the state, as a word, on stdout. Used for debugging & to
// tell the user about the MBR state when the program launches....
void MBRData::ShowState(void) {
   switch (state) {
      case invalid:
         cout << "  MBR: not present\n";
         break;
      case gpt:
         cout << "  MBR: protective\n";
         break;
      case hybrid:
         cout << "  MBR: hybrid\n";
         break;
      case mbr:
         cout << "  MBR: MBR only\n";
         break;
      default:
         cout << "\a  MBR: unknown -- bug!\n";
         break;
   } // switch
} // MBRData::ShowState()

/*********************************************************************
 *                                                                   *
 * Functions that set or get disk metadata (CHS geometry, disk size, *
 * etc.)                                                             *
 *                                                                   *
 *********************************************************************/

// Sets the CHS geometry. CHS geometry is used by LBAtoCHS() function.
// Note that this only sets the heads and sectors; the number of
// cylinders is determined by these values and the disk size.
void MBRData::SetCHSGeom(uint32_t h, uint32_t s) {
   if ((h <= MAX_HEADS) && (s <= MAX_SECSPERTRACK)) {
      numHeads = h;
      numSecspTrack = s;
   } else {
      cout << "Warning! Attempt to set invalid CHS geometry!\n";
   } // if/else
} // MBRData::SetCHSGeom()

// Converts 64-bit LBA value to MBR-style CHS value. Returns 1 if conversion
// was within the range that can be expressed by CHS (including 0, for an
// empty partition), 0 if the value is outside that range, and -1 if chs is
// invalid.
int MBRData::LBAtoCHS(uint64_t lba, uint8_t * chs) {
   uint64_t cylinder, head, sector; // all numbered from 0
   uint64_t remainder;
   int retval = 1;
   int done = 0;

   if (chs != NULL) {
      // Special case: In case of 0 LBA value, zero out CHS values....
      if (lba == 0) {
         chs[0] = chs[1] = chs[2] = UINT8_C(0);
         done = 1;
      } // if
      // If LBA value is too large for CHS, max out CHS values....
      if ((!done) && (lba >= (numHeads * numSecspTrack * MAX_CYLINDERS))) {
         chs[0] = 254;
         chs[1] = chs[2] = 255;
         done = 1;
         retval = 0;
      } // if
      // If neither of the above applies, compute CHS values....
      if (!done) {
         cylinder = lba / (uint64_t) (numHeads * numSecspTrack);
         remainder = lba - (cylinder * numHeads * numSecspTrack);
         head = remainder / numSecspTrack;
         remainder -= head * numSecspTrack;
         sector = remainder;
         if (head < numHeads)
            chs[0] = (uint8_t) head;
         else
            retval = 0;
         if (sector < numSecspTrack) {
            chs[1] = (uint8_t) ((sector + 1) + (cylinder >> 8) * 64);
            chs[2] = (uint8_t) (cylinder & UINT64_C(0xFF));
         } else {
            retval = 0;
         } // if/else
      } // if value is expressible and non-0
   } else { // Invalid (NULL) chs pointer
      retval = -1;
   } // if CHS pointer valid
   return (retval);
} // MBRData::LBAtoCHS()

// Look for problems -- overlapping partitions, etc.
int MBRData::Verify(void) {
   int i, j, theyOverlap, numProbs = 0, numEE = 0;
   uint32_t firstLBA1, firstLBA2, lastLBA1, lastLBA2;

   for (i = 0; i < MAX_MBR_PARTS; i++) {
      for (j = i + 1; j < MAX_MBR_PARTS; j++) {
         theyOverlap = 0;
         firstLBA1 = partitions[i].firstLBA;
         firstLBA2 = partitions[j].firstLBA;
         if ((firstLBA1 != 0) && (firstLBA2 != 0)) {
            lastLBA1 = partitions[i].firstLBA + partitions[i].lengthLBA - 1;
            lastLBA2 = partitions[j].firstLBA + partitions[j].lengthLBA - 1;
            if ((firstLBA1 < lastLBA2) && (lastLBA1 >= firstLBA2))
               theyOverlap = 1;
            if ((firstLBA2 < lastLBA1) && (lastLBA2 >= firstLBA1))
               theyOverlap = 1;
         } // if
         if (theyOverlap) {
            numProbs++;
            cout << "\nProblem: MBR partitions " << i + 1 << " and " << j + 1
                 << " overlap!\n";
         } // if
      } // for (j...)
      if (partitions[i].partitionType == 0xEE) {
         numEE++;
         if (partitions[i].firstLBA != 1)
            cout << "\nWarning: 0xEE partition doesn't start on sector 1. This can cause problems\n"
                 << "in some OSes.\n";
      } // if
   } // for (i...)
   if (numEE > 1)
      cout << "\nCaution: More than one 0xEE MBR partition found. This can cause problems\n"
           << "in some OSes.\n";

   return numProbs;
} // MBRData::Verify()

/*****************************************************
 *                                                   *
 * Functions to create, delete, or change partitions *
 *                                                   *
 *****************************************************/

// Empty all data. Meant mainly for calling by constructors, but it's also
// used by the hybrid MBR functions in the GPTData class.
void MBRData::EmptyMBR(int clearBootloader) {
   int i;

   // Zero out the boot loader section, the disk signature, and the
   // 2-byte nulls area only if requested to do so. (This is the
   // default.)
   if (clearBootloader == 1) {
      EmptyBootloader();
   } // if

   // Blank out the partitions
   for (i = 0; i < MAX_MBR_PARTS; i++) {
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
} // MBRData::EmptyMBR()

// Blank out the boot loader area. Done with the initial MBR-to-GPT
// conversion, since MBR boot loaders don't understand GPT, and so
// need to be replaced....
void MBRData::EmptyBootloader(void) {
   int i;

   for (i = 0; i < 440; i++)
      code[i] = 0;
   nulls = 0;
} // MBRData::EmptyBootloader

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

// Create a partition of the specified number, starting LBA, and
// length. This function does *NO* error checking, so it's possible
// to seriously screw up a partition table using this function!
// Note: This function should NOT be used to create the 0xEE partition
// in a conventional GPT configuration, since that partition has
// specific size requirements that this function won't handle. It may
// be used for creating the 0xEE partition(s) in a hybrid MBR, though,
// since those toss the rulebook away anyhow....
void MBRData::MakePart(int num, uint32_t start, uint32_t length, int type, int bootable) {
   if ((num >= 0) && (num < MAX_MBR_PARTS)) {
      partitions[num].firstSector[0] = UINT8_C(0);
      partitions[num].firstSector[1] = UINT8_C(0);
      partitions[num].firstSector[2] = UINT8_C(0);
      partitions[num].partitionType = (uint8_t) type;
      partitions[num].lastSector[0] = UINT8_C(0);
      partitions[num].lastSector[1] = UINT8_C(0);
      partitions[num].lastSector[2] = UINT8_C(0);
      partitions[num].firstLBA = start;
      partitions[num].lengthLBA = length;
      // If this is a "real" partition, set its CHS geometry
      if (length > 0) {
         LBAtoCHS((uint64_t) start, partitions[num].firstSector);
         LBAtoCHS((uint64_t) (start + length - 1), partitions[num].lastSector);
      } // if (length > 0)
      SetPartBootable(num, bootable);
   } // if valid partition number
} // MBRData::MakePart()

// Set the partition's type code.
// Returns 1 if successful, 0 if not (invalid partition number)
int MBRData::SetPartType(int num, int type) {
   int allOK = 1;

   if ((num >= 0) && (num < MAX_MBR_PARTS)) {
      if (partitions[num].lengthLBA != UINT32_C(0)) {
         partitions[num].partitionType = (uint8_t) type;
      } else allOK = 0;
   } else allOK = 0;
   return allOK;
} // MBRData::SetPartType()

// Set (or remove) the partition's bootable flag. Setting it is the
// default; pass 0 as bootable to remove the flag.
// Returns 1 if successful, 0 if not (invalid partition number)
int MBRData::SetPartBootable(int num, int bootable) {
   int allOK = 1;

   if ((num >= 0) && (num < MAX_MBR_PARTS)) {
      if (partitions[num].lengthLBA != UINT32_C(0)) {
         if (bootable == 0)
            partitions[num].status = UINT8_C(0);
         else
            partitions[num].status = UINT8_C(0x80);
      } else allOK = 0;
   } else allOK = 0;
   return allOK;
} // MBRData::SetPartBootable()

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
   } // for partition loop
   if (typeFlag == 0) { // No non-hybrid partitions found
      MakeProtectiveMBR(); // ensure it's a fully compliant hybrid MBR.
   } // if
} // MBRData::OptimizeEESize()

// Recomputes the CHS values for the specified partition and adjusts the value.
// Note that this will create a technically incorrect CHS value for EFI GPT (0xEE)
// protective partitions, but this is required by some buggy BIOSes, so I'm
// providing a function to do this deliberately at the user's command.
// This function does nothing if the partition's length is 0.
void MBRData::RecomputeCHS(int partNum) {
   uint64_t firstLBA, lengthLBA;

   firstLBA = (uint64_t) partitions[partNum].firstLBA;
   lengthLBA = (uint64_t) partitions[partNum].lengthLBA;

   if (lengthLBA > 0) {
      LBAtoCHS(firstLBA, partitions[partNum].firstSector);
      LBAtoCHS(firstLBA + lengthLBA - 1, partitions[partNum].lastSector);
   } // if
} // MBRData::RecomputeCHS()

// Creates an MBR extended partition holding logical partitions that
// correspond to the list of GPT partitions in theList. The extended
// partition is placed in position #4 (counting from 1) in the MBR.
// The logical partition data are copied to the partitions[] array in
// positions 4 and up (counting from 0). Neither the MBR nor the EBR
// entries are written to disk; that is left for the WriteMBRData()
// function.
// Returns number of converted partitions
int MBRData::CreateLogicals(PartNotes& notes) {
   uint64_t extEndLBA = 0, extStartLBA = UINT64_MAX;
   int i = 4, numLogicals = 0;
   struct PartInfo aPart;

   // Find bounds of the extended partition....
   notes.Rewind();
   while (notes.GetNextInfo(&aPart) >= 0) {
      if (aPart.type == LOGICAL) {
         if (extStartLBA > aPart.firstLBA)
            extStartLBA = aPart.firstLBA;
         if (extEndLBA < aPart.lastLBA)
            extEndLBA = aPart.lastLBA;
         numLogicals++;
      } // if
   } // while
   extStartLBA--;

   if ((extStartLBA < UINT32_MAX) && ((extEndLBA - extStartLBA + 1) < UINT32_MAX)) {
      notes.Rewind();
      i = 4;
      while ((notes.GetNextInfo(&aPart) >= 0) && (i < MAX_MBR_PARTS)) {
         if (aPart.type == LOGICAL) {
            partitions[i].partitionType = aPart.hexCode;
            partitions[i].firstLBA = (uint32_t) (aPart.firstLBA - extStartLBA);
            partitions[i].lengthLBA = (uint32_t) (aPart.lastLBA - aPart.firstLBA + 1);
            LBAtoCHS(UINT64_C(1), (uint8_t *) &partitions[i].firstSector);
            LBAtoCHS(partitions[i].lengthLBA, (uint8_t *) &partitions[i].lastSector);
            partitions[i].status = aPart.active * 0x80;
            i++;
         } // if
      } // while
      MakePart(3, (uint32_t) extStartLBA, (uint32_t) (extEndLBA - extStartLBA + 1), 0x0f, 0);
   } else {
      if (numLogicals > 0) {
         cerr << "Unable to create logical partitions; they exceed the 2 TiB limit!\n";
//         cout << "extStartLBA = " << extStartLBA << ", extEndLBA = " << extEndLBA << "\n";
      }
   } // if/else
   return (i - 4);
} // MBRData::CreateLogicals()

/****************************************
 *                                      *
 * Functions to find data on free space *
 *                                      *
 ****************************************/

// Finds the first free space on the disk from start onward; returns 0
// if none available....
uint32_t MBRData::FindFirstAvailable(uint32_t start) {
   uint32_t first;
   uint32_t i;
   int firstMoved;

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

   if ((diskSize <= UINT32_MAX) && (diskSize > 0))
      nearestStart = (uint32_t) diskSize - 1;
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
      if (thisLastLBA > 0)
         thisLastLBA--;
      if ((thisLastLBA > bestLastLBA) && (thisLastLBA < start))
         bestLastLBA = thisLastLBA + 1;
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
      // Note: Weird two-line thing to avoid subtracting 1 from a 0 value
      // for an unsigned int....
      last = first + partitions[i].lengthLBA;
      if (last > 0)
         last--;
      if ((first <= sector) && (last >= sector))
         isFree = 0;
   } // for
   return isFree;
} // MBRData::IsFree()

/******************************************************
 *                                                    *
 * Functions that extract data on specific partitions *
 *                                                    *
 ******************************************************/

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

/***********************
 *                     *
 * Protected functions *
 *                     *
 ***********************/

// Return a pointer to a primary or logical partition, or NULL if
// the partition is out of range....
struct MBRRecord* MBRData::GetPartition(int i) {
   MBRRecord* thePart = NULL;

   if ((i >= 0) && (i < MAX_MBR_PARTS))
      thePart = &partitions[i];
   return thePart;
} // GetPartition()
