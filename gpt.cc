/* gpt.cc -- Functions for loading, saving, and manipulating legacy MBR and GPT partition
   data. */

/* By Rod Smith, initial coding January to February, 2009 */

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
#include <iostream>
#include "crc32.h"
#include "gpt.h"
#include "bsd.h"
#include "support.h"
#include "parttypes.h"
#include "attributes.h"
#include "diskio.h"

using namespace std;

/****************************************
 *                                      *
 * GPTData class and related structures *
 *                                      *
 ****************************************/

// Default constructor
GPTData::GPTData(void) {
   blockSize = SECTOR_SIZE; // set a default
   diskSize = 0;
   partitions = NULL;
   state = gpt_valid;
   device = "";
   justLooking = 0;
   mainCrcOk = 0;
   secondCrcOk = 0;
   mainPartsCrcOk = 0;
   secondPartsCrcOk = 0;
   apmFound = 0;
   bsdFound = 0;
   sectorAlignment = 8; // Align partitions on 4096-byte boundaries by default
   beQuiet = 0;
   whichWasUsed = use_new;
   srand((unsigned int) time(NULL));
   mainHeader.numParts = 0;
   SetGPTSize(NUM_GPT_ENTRIES);
} // GPTData default constructor

// The following constructor loads GPT data from a device file
GPTData::GPTData(string filename) {
   blockSize = SECTOR_SIZE; // set a default
   diskSize = 0;
   partitions = NULL;
   state = gpt_invalid;
   device = "";
   justLooking = 0;
   mainCrcOk = 0;
   secondCrcOk = 0;
   mainPartsCrcOk = 0;
   secondPartsCrcOk = 0;
   apmFound = 0;
   bsdFound = 0;
   sectorAlignment = 8; // Align partitions on 4096-byte boundaries by default
   beQuiet = 0;
   whichWasUsed = use_new;
   srand((unsigned int) time(NULL));
   mainHeader.numParts = 0;
   if (!LoadPartitions(filename))
      exit(2);
} // GPTData(string filename) constructor

// Destructor
GPTData::~GPTData(void) {
   free(partitions);
} // GPTData destructor

/*********************************************************************
 *                                                                   *
 * Begin functions that verify data, or that adjust the verification *
 * information (compute CRCs, rebuild headers)                       *
 *                                                                   *
 *********************************************************************/

// Perform detailed verification, reporting on any problems found, but
// do *NOT* recover from these problems. Returns the total number of
// problems identified.
int GPTData::Verify(void) {
   int problems = 0;
   uint32_t i, numSegments;
   uint64_t totalFree, largestSegment;
   char siTotal[255], siLargest[255];

   // First, check for CRC errors in the GPT data....
   if (!mainCrcOk) {
      problems++;
      cout << "\nProblem: The CRC for the main GPT header is invalid. The main GPT header may\n"
           << "be corrupt. Consider loading the backup GPT header to rebuild the main GPT\n"
           << "header ('b' on the recovery & transformation menu). This report may be a false\n"
           << "alarm if you've already corrected other problems.\n";
   } // if
   if (!mainPartsCrcOk) {
      problems++;
      cout << "\nProblem: The CRC for the main partition table is invalid. This table may be\n"
           << "corrupt. Consider loading the backup partition table ('c' on the recovery &\n"
           << "transformation menu). This report may be a false alarm if you've already\n"
           << "corrected other problems.\n";
   } // if
   if (!secondCrcOk) {
      problems++;
      cout << "\nProblem: The CRC for the backup GPT header is invalid. The backup GPT header\n"
           << "may be corrupt. Consider using the main GPT header to rebuild the backup GPT\n"
           << "header ('d' on the recovery & transformation menu). This report may be a false\n"
           << "alarm if you've already corrected other problems.\n";
   } // if
   if (!secondPartsCrcOk) {
      problems++;
      cout << "\nCaution: The CRC for the backup partition table is invalid. This table may\n"
           << "be corrupt. This program will automatically create a new backup partition\n"
           << "table when you save your partitions.\n";
   } // if

   // Now check that the main and backup headers both point to themselves....
   if (mainHeader.currentLBA != 1) {
      problems++;
      cout << "\nProblem: The main header's self-pointer doesn't point to itself. This problem\n"
           << "is being automatically corrected, but it may be a symptom of more serious\n"
           << "problems. Think carefully before saving changes with 'w' or using this disk.\n";
      mainHeader.currentLBA = 1;
   } // if
   if (secondHeader.currentLBA != (diskSize - UINT64_C(1))) {
      problems++;
      cout << "\nProblem: The secondary header's self-pointer indicates that it doesn't reside\n"
           << "at the end of the disk. If you've added a disk to a RAID array, use the 'e'\n"
           << "option on the experts' menu to adjust the secondary header's and partition\n"
           << "table's locations.\n";
   } // if

   // Now check that critical main and backup GPT entries match each other
   if (mainHeader.currentLBA != secondHeader.backupLBA) {
      problems++;
      cout << "\nProblem: main GPT header's current LBA pointer (" << mainHeader.currentLBA
           << ") doesn't\nmatch the backup GPT header's alternate LBA pointer("
           << secondHeader.backupLBA << ").\n";
   } // if
   if (mainHeader.backupLBA != secondHeader.currentLBA) {
      problems++;
      cout << "\nProblem: main GPT header's backup LBA pointer (" << mainHeader.backupLBA
           << ") doesn't\nmatch the backup GPT header's current LBA pointer ("
           << secondHeader.currentLBA << ").\n"
           << "The 'e' option on the experts' menu may fix this problem.\n";
   } // if
   if (mainHeader.firstUsableLBA != secondHeader.firstUsableLBA) {
      problems++;
      cout << "\nProblem: main GPT header's first usable LBA pointer (" << mainHeader.firstUsableLBA
           << ") doesn't\nmatch the backup GPT header's first usable LBA pointer ("
           << secondHeader.firstUsableLBA << ")\n";
   } // if
   if (mainHeader.lastUsableLBA != secondHeader.lastUsableLBA) {
      problems++;
      cout << "\nProblem: main GPT header's last usable LBA pointer (" << mainHeader.lastUsableLBA
           << ") doesn't\nmatch the backup GPT header's last usable LBA pointer ("
           << secondHeader.lastUsableLBA << ")\n"
           << "The 'e' option on the experts' menu can probably fix this problem.\n";
   } // if
   if ((mainHeader.diskGUID != secondHeader.diskGUID)) {
      problems++;
      cout << "\nProblem: main header's disk GUID (" << mainHeader.diskGUID.AsString()
           << ") doesn't\nmatch the backup GPT header's disk GUID ("
           << secondHeader.diskGUID.AsString() << ")\n"
           << "You should use the 'b' or 'd' option on the recovery & transformation menu to\n"
           << "select one or the other header.\n";
   } // if
   if (mainHeader.numParts != secondHeader.numParts) {
      problems++;
      cout << "\nProblem: main GPT header's number of partitions (" << mainHeader.numParts
           << ") doesn't\nmatch the backup GPT header's number of partitions ("
           << secondHeader.numParts << ")\n"
           << "Resizing the partition table ('s' on the experts' menu) may help.\n";
   } // if
   if (mainHeader.sizeOfPartitionEntries != secondHeader.sizeOfPartitionEntries) {
      problems++;
      cout << "\nProblem: main GPT header's size of partition entries ("
           << mainHeader.sizeOfPartitionEntries << ") doesn't\n"
           << "match the backup GPT header's size of partition entries ("
           << secondHeader.sizeOfPartitionEntries << ")\n"
           << "You should use the 'b' or 'd' option on the recovery & transformation menu to\n"
           << "select one or the other header.\n";
   } // if

   // Now check for a few other miscellaneous problems...
   // Check that the disk size will hold the data...
   if (mainHeader.backupLBA > diskSize) {
      problems++;
      cout << "\nProblem: Disk is too small to hold all the data!\n"
           << "(Disk size is " << diskSize << " sectors, needs to be "
           << mainHeader.backupLBA << " sectors.)\n"
           << "The 'e' option on the experts' menu may fix this problem.\n";
   } // if

   // Check for overlapping partitions....
   problems += FindOverlaps();

   // Check for mismatched MBR and GPT partitions...
   problems += FindHybridMismatches();

   // Verify that partitions don't run into GPT data areas....
   problems += CheckGPTSize();

   // Check that partitions are aligned on proper boundaries (for WD Advanced
   // Format and similar disks)....
   for (i = 0; i < mainHeader.numParts; i++) {
      if ((partitions[i].GetFirstLBA() % sectorAlignment) != 0) {
         cout << "\nCaution: Partition " << i + 1 << " doesn't begin on a "
              << sectorAlignment << "-sector boundary. This may\nresult "
              << "in degraded performance on some modern (2009 and later) hard disks.\n";
      } // if
   } // for

   // Now compute available space, but only if no problems found, since
   // problems could affect the results
   if (problems == 0) {
      totalFree = FindFreeBlocks(&numSegments, &largestSegment);
      strcpy(siTotal, BytesToSI(totalFree * (uint64_t) blockSize).c_str());
      strcpy(siLargest, BytesToSI(largestSegment * (uint64_t) blockSize).c_str());
      cout << "No problems found. " << totalFree << " free sectors ("
           << BytesToSI(totalFree * (uint64_t) blockSize) << ") available in "
           << numSegments << "\nsegments, the largest of which is "
           << largestSegment << " (" << BytesToSI(largestSegment * (uint64_t) blockSize)
           << ") in size\n";
   } else {
      cout << "\nIdentified " << problems << " problems!\n";
   } // if/else

   return (problems);
} // GPTData::Verify()

// Checks to see if the GPT tables overrun existing partitions; if they
// do, issues a warning but takes no action. Returns number of problems
// detected (0 if OK, 1 to 2 if problems).
int GPTData::CheckGPTSize(void) {
   uint64_t overlap, firstUsedBlock, lastUsedBlock;
   uint32_t i;
   int numProbs = 0;

   // first, locate the first & last used blocks
   firstUsedBlock = UINT64_MAX;
   lastUsedBlock = 0;
   for (i = 0; i < mainHeader.numParts; i++) {
      if ((partitions[i].GetFirstLBA() < firstUsedBlock) &&
           (partitions[i].GetFirstLBA() != 0))
         firstUsedBlock = partitions[i].GetFirstLBA();
      if (partitions[i].GetLastLBA() > lastUsedBlock)
         lastUsedBlock = partitions[i].GetLastLBA();
   } // for

   // If the disk size is 0 (the default), then it means that various
   // variables aren't yet set, so the below tests will be useless;
   // therefore we should skip everything
   if (diskSize != 0) {
      if (mainHeader.firstUsableLBA > firstUsedBlock) {
         overlap = mainHeader.firstUsableLBA - firstUsedBlock;
         cout << "Warning! Main partition table overlaps the first partition by "
              << overlap << " blocks!\n";
         if (firstUsedBlock > 2) {
            cout << "Try reducing the partition table size by " << overlap * 4
                 << " entries.\n(Use the 's' item on the experts' menu.)\n";
         } else {
            cout << "You will need to delete this partition or resize it in another utility.\n";
         } // if/else
         numProbs++;
      } // Problem at start of disk
      if (mainHeader.lastUsableLBA < lastUsedBlock) {
         overlap = lastUsedBlock - mainHeader.lastUsableLBA;
         cout << "Warning! Secondary partition table overlaps the last partition by "
              << overlap << " blocks!\n";
         if (lastUsedBlock > (diskSize - 2)) {
            cout << "You will need to delete this partition or resize it in another utility.\n";
         } else {
            cout << "Try reducing the partition table size by " << overlap * 4
                 << " entries.\n(Use the 's' item on the experts' menu.)\n";
         } // if/else
         numProbs++;
      } // Problem at end of disk
   } // if (diskSize != 0)
   return numProbs;
} // GPTData::CheckGPTSize()

// Check the validity of the GPT header. Returns 1 if the main header
// is valid, 2 if the backup header is valid, 3 if both are valid, and
// 0 if neither is valid. Note that this function just checks the GPT
// signature and revision numbers, not CRCs or other data.
int GPTData::CheckHeaderValidity(void) {
   int valid = 3;

   cout.setf(ios::uppercase);
   cout.fill('0');

   // Note: failed GPT signature checks produce no error message because
   // a message is displayed in the ReversePartitionBytes() function
   if (mainHeader.signature != GPT_SIGNATURE) {
      valid -= 1;
   } else if ((mainHeader.revision != 0x00010000) && valid) {
      valid -= 1;
      cout << "Unsupported GPT version in main header; read 0x";
      cout.width(8);
      cout << hex << mainHeader.revision << ", should be\n0x";
      cout.width(8);
      cout << UINT32_C(0x00010000) << dec << "\n";
   } // if/else/if

   if (secondHeader.signature != GPT_SIGNATURE) {
      valid -= 2;
   } else if ((secondHeader.revision != 0x00010000) && valid) {
      valid -= 2;
      cout << "Unsupported GPT version in backup header; read 0x";
      cout.width(8);
      cout << hex << secondHeader.revision << ", should be\n0x";
      cout.width(8);
      cout << UINT32_C(0x00010000) << dec << "\n";
   } // if/else/if

   // If MBR bad, check for an Apple disk signature
   if ((protectiveMBR.GetValidity() == invalid) &&
        (((mainHeader.signature << 32) == APM_SIGNATURE1) ||
        (mainHeader.signature << 32) == APM_SIGNATURE2)) {
      apmFound = 1; // Will display warning message later
   } // if
   cout.fill(' ');

   return valid;
} // GPTData::CheckHeaderValidity()

// Check the header CRC to see if it's OK...
// Note: Must be called BEFORE byte-order reversal on big-endian
// systems!
int GPTData::CheckHeaderCRC(struct GPTHeader* header) {
   uint32_t oldCRC, newCRC, hSize;

   // Back up old header CRC and then blank it, since it must be 0 for
   // computation to be valid
   oldCRC = header->headerCRC;
   header->headerCRC = UINT32_C(0);
   hSize = header->headerSize;

   // If big-endian system, reverse byte order
   if (IsLittleEndian() == 0) {
      ReverseBytes(&oldCRC, 4);
   } // if

   // Initialize CRC functions...
   chksum_crc32gentab();

   // Compute CRC, restore original, and return result of comparison
   newCRC = chksum_crc32((unsigned char*) header, HEADER_SIZE);
   header->headerCRC = oldCRC;
   return (oldCRC == newCRC);
} // GPTData::CheckHeaderCRC()

// Recompute all the CRCs. Must be called before saving if any changes have
// been made. Must be called on platform-ordered data (this function reverses
// byte order and then undoes that reversal.)
void GPTData::RecomputeCRCs(void) {
   uint32_t crc, hSize, trueNumParts;
   int littleEndian = 1;

   // Initialize CRC functions...
   chksum_crc32gentab();

   // Save some key data from header before reversing byte order....
   trueNumParts = mainHeader.numParts;
   hSize = mainHeader.headerSize;

   if ((littleEndian = IsLittleEndian()) == 0) {
      ReversePartitionBytes();
      ReverseHeaderBytes(&mainHeader);
      ReverseHeaderBytes(&secondHeader);
   } // if
/*   if ((littleEndian = IsLittleEndian()) == 0) {
      ReverseBytes(&trueNumParts, 4);
      ReverseBytes(&hSize, 4);
   } // if */

   // Compute CRC of partition tables & store in main and secondary headers
   crc = chksum_crc32((unsigned char*) partitions, trueNumParts * GPT_SIZE);
   mainHeader.partitionEntriesCRC = crc;
   secondHeader.partitionEntriesCRC = crc;
   if (littleEndian == 0) {
      ReverseBytes(&mainHeader.partitionEntriesCRC, 4);
      ReverseBytes(&secondHeader.partitionEntriesCRC, 4);
   } // if

   // Zero out GPT tables' own CRCs (required for correct computation)
   mainHeader.headerCRC = 0;
   secondHeader.headerCRC = 0;

   // Compute & store CRCs of main & secondary headers...
   crc = chksum_crc32((unsigned char*) &mainHeader, hSize);
   if (littleEndian == 0)
      ReverseBytes(&crc, 4);
   mainHeader.headerCRC = crc;
   crc = chksum_crc32((unsigned char*) &secondHeader, hSize);
   if (littleEndian == 0)
      ReverseBytes(&crc, 4);
   secondHeader.headerCRC = crc;

   if ((littleEndian = IsLittleEndian()) == 0) {
      ReverseHeaderBytes(&mainHeader);
      ReverseHeaderBytes(&secondHeader);
      ReversePartitionBytes();
   } // if
} // GPTData::RecomputeCRCs()

// Rebuild the main GPT header, using the secondary header as a model.
// Typically called when the main header has been found to be corrupt.
void GPTData::RebuildMainHeader(void) {
   int i;

   mainHeader.signature = GPT_SIGNATURE;
   mainHeader.revision = secondHeader.revision;
   mainHeader.headerSize = secondHeader.headerSize;
   mainHeader.headerCRC = UINT32_C(0);
   mainHeader.reserved = secondHeader.reserved;
   mainHeader.currentLBA = secondHeader.backupLBA;
   mainHeader.backupLBA = secondHeader.currentLBA;
   mainHeader.firstUsableLBA = secondHeader.firstUsableLBA;
   mainHeader.lastUsableLBA = secondHeader.lastUsableLBA;
   mainHeader.diskGUID = secondHeader.diskGUID;
   mainHeader.partitionEntriesLBA = UINT64_C(2);
   mainHeader.numParts = secondHeader.numParts;
   mainHeader.sizeOfPartitionEntries = secondHeader.sizeOfPartitionEntries;
   mainHeader.partitionEntriesCRC = secondHeader.partitionEntriesCRC;
   for (i = 0 ; i < GPT_RESERVED; i++)
      mainHeader.reserved2[i] = secondHeader.reserved2[i];
   mainCrcOk = secondCrcOk;
} // GPTData::RebuildMainHeader()

// Rebuild the secondary GPT header, using the main header as a model.
void GPTData::RebuildSecondHeader(void) {
   int i;

   secondHeader.signature = GPT_SIGNATURE;
   secondHeader.revision = mainHeader.revision;
   secondHeader.headerSize = mainHeader.headerSize;
   secondHeader.headerCRC = UINT32_C(0);
   secondHeader.reserved = mainHeader.reserved;
   secondHeader.currentLBA = mainHeader.backupLBA;
   secondHeader.backupLBA = mainHeader.currentLBA;
   secondHeader.firstUsableLBA = mainHeader.firstUsableLBA;
   secondHeader.lastUsableLBA = mainHeader.lastUsableLBA;
   secondHeader.diskGUID = mainHeader.diskGUID;
   secondHeader.partitionEntriesLBA = secondHeader.lastUsableLBA + UINT64_C(1);
   secondHeader.numParts = mainHeader.numParts;
   secondHeader.sizeOfPartitionEntries = mainHeader.sizeOfPartitionEntries;
   secondHeader.partitionEntriesCRC = mainHeader.partitionEntriesCRC;
   for (i = 0 ; i < GPT_RESERVED; i++)
      secondHeader.reserved2[i] = mainHeader.reserved2[i];
   secondCrcOk = mainCrcOk;
} // GPTData::RebuildSecondHeader()

// Search for hybrid MBR entries that have no corresponding GPT partition.
// Returns number of such mismatches found
int GPTData::FindHybridMismatches(void) {
   int i, found, numFound = 0;
   uint32_t j;
   uint64_t mbrFirst, mbrLast;

   for (i = 0; i < 4; i++) {
      if ((protectiveMBR.GetType(i) != 0xEE) && (protectiveMBR.GetType(i) != 0x00)) {
         j = 0;
         found = 0;
         do {
            mbrFirst = (uint64_t) protectiveMBR.GetFirstSector(i);
            mbrLast = mbrFirst + (uint64_t) protectiveMBR.GetLength(i) - UINT64_C(1);
            if ((partitions[j].GetFirstLBA() == mbrFirst) &&
                (partitions[j].GetLastLBA() == mbrLast))
               found = 1;
            j++;
         } while ((!found) && (j < mainHeader.numParts));
         if (!found) {
            numFound++;
            cout << "\nWarning! Mismatched GPT and MBR partition! MBR partition "
                 << i + 1 << ", of type 0x";
            cout.fill('0');
            cout.setf(ios::uppercase);
            cout.width(2);
            cout << hex << (int) protectiveMBR.GetType(i) << ",\n"
                 << "has no corresponding GPT partition! You may continue, but this condition\n"
                 << "might cause data loss in the future!\a\n" << dec;
            cout.fill(' ');
         } // if
      } // if
   } // for
   return numFound;
} // GPTData::FindHybridMismatches

// Find overlapping partitions and warn user about them. Returns number of
// overlapping partitions.
int GPTData::FindOverlaps(void) {
   int problems = 0;
   uint32_t i, j;

   for (i = 1; i < mainHeader.numParts; i++) {
      for (j = 0; j < i; j++) {
         if (partitions[i].DoTheyOverlap(partitions[j])) {
            problems++;
            cout << "\nProblem: partitions " << i + 1 << " and " << j + 1 << " overlap:\n";
            cout << "  Partition " << i + 1 << ": " << partitions[i].GetFirstLBA()
                 << " to " << partitions[i].GetLastLBA() << "\n";
            cout << "  Partition " << j + 1 << ": " << partitions[j].GetFirstLBA()
                 << " to " << partitions[j].GetLastLBA() << "\n";
         } // if
      } // for j...
   } // for i...
   return problems;
} // GPTData::FindOverlaps()

/******************************************************************
 *                                                                *
 * Begin functions that load data from disk or save data to disk. *
 *                                                                *
 ******************************************************************/

// Scan for partition data. This function loads the MBR data (regular MBR or
// protective MBR) and loads BSD disklabel data (which is probably invalid).
// It also looks for APM data, forces a load of GPT data, and summarizes
// the results.
void GPTData::PartitionScan(void) {
   BSDData bsdDisklabel;

   // Read the MBR & check for BSD disklabel
   protectiveMBR.ReadMBRData(&myDisk);
   bsdDisklabel.ReadBSDData(&myDisk, 0, diskSize - 1);

   // Load the GPT data, whether or not it's valid
   ForceLoadGPTData();

   if (!beQuiet) {
      cout << "Partition table scan:\n";
      protectiveMBR.ShowState();
      bsdDisklabel.ShowState();
      ShowAPMState(); // Show whether there's an Apple Partition Map present
      ShowGPTState(); // Show GPT status
      cout << "\n";
   } // if

   if (apmFound) {
      cout << "\n*******************************************************************\n"
           << "This disk appears to contain an Apple-format (APM) partition table!\n";
      if (!justLooking) {
         cout << "It will be destroyed if you continue!\n";
      } // if
      cout << "*******************************************************************\n\n\a";
   } // if
} // GPTData::PartitionScan()

// Read GPT data from a disk.
int GPTData::LoadPartitions(const string & deviceFilename) {
   int err, allOK = 1;
   uint32_t i;
   uint64_t firstBlock, lastBlock;
   BSDData bsdDisklabel;
   MBRValidity mbrState;

   // First, do a test to see if writing will be possible later....
   err = myDisk.OpenForWrite(deviceFilename);
   if ((err == 0) && (!justLooking)) {
      cout << "\aNOTE: Write test failed with error number " << errno
           << ". It will be impossible to save\nchanges to this disk's partition table!\n";
#ifdef __FreeBSD__
      cout << "You may be able to enable writes by exiting this program, typing\n"
           << "'sysctl kern.geom.debugflags=16' at a shell prompt, and re-running this\n"
           << "program.\n";
#endif
      cout << "\n";
   } // if
   myDisk.Close();

   if (myDisk.OpenForRead(deviceFilename)) {
      // store disk information....
      diskSize = myDisk.DiskSize(&err);
      blockSize = (uint32_t) myDisk.GetBlockSize();
      sectorAlignment = myDisk.FindAlignment();
      device = deviceFilename;
      PartitionScan(); // Check for partition types, load GPT, & print summary

      whichWasUsed = UseWhichPartitions();
      switch (whichWasUsed) {
         case use_mbr:
            XFormPartitions();
            break;
         case use_bsd:
            bsdDisklabel.ReadBSDData(&myDisk, 0, diskSize - 1);
//            bsdDisklabel.DisplayBSDData();
            ClearGPTData();
            protectiveMBR.MakeProtectiveMBR(1); // clear boot area (option 1)
            XFormDisklabel(&bsdDisklabel, 0);
            break;
         case use_gpt:
            mbrState = protectiveMBR.GetValidity();
            if ((mbrState == invalid) || (mbrState == mbr))
               protectiveMBR.MakeProtectiveMBR();
            break;
         case use_new:
            ClearGPTData();
            protectiveMBR.MakeProtectiveMBR();
            break;
         case use_abort:
            allOK = 0;
            cerr << "Aborting because of invalid partition data!\n";
            break;
      } // switch

      // Now find the first and last sectors used by partitions...
      if (allOK) {
         firstBlock = mainHeader.backupLBA; // start high
         lastBlock = 0; // start low
         for (i = 0; i < mainHeader.numParts; i++) {
            if ((partitions[i].GetFirstLBA() < firstBlock) &&
                 (partitions[i].GetFirstLBA() > 0))
               firstBlock = partitions[i].GetFirstLBA();
            if (partitions[i].GetLastLBA() > lastBlock)
               lastBlock = partitions[i].GetLastLBA();
         } // for
         CheckGPTSize();
      } // if
   } else {
      allOK = 0;
      cerr << "Problem opening " << deviceFilename << " for reading! Error is "
           << errno << "\n";
      if (errno == EACCES) { // User is probably not running as root
         cerr << "You must run this program as root or use sudo!\n";
      } // if
   } // if/else
   return (allOK);
} // GPTData::LoadPartitions()

// Loads the GPT, as much as possible. Returns 1 if this seems to have
// succeeded, 0 if there are obvious problems....
int GPTData::ForceLoadGPTData(void) {
   int allOK = 1, validHeaders;
   uint64_t seekTo;
   uint8_t* storage;
   uint32_t newCRC, sizeOfParts;

   // Seek to and read the main GPT header
   if (myDisk.Seek(1)) {
      if (myDisk.Read(&mainHeader, 512) != 512) { // read main GPT header
         cerr << "Warning! Error " << errno << " reading main GPT header!\n";
      } // if read not OK
   } else allOK = 0; // if/else seek OK
   mainCrcOk = CheckHeaderCRC(&mainHeader);
   if (IsLittleEndian() == 0) // big-endian system; adjust header byte order....
      ReverseHeaderBytes(&mainHeader);

   // Load backup header, check its CRC, and store the results of the
   // check for future reference. Load backup header using pointer in main
   // header if possible; but if main header has a CRC error, or if it
   // points to beyond the end of the disk, load the last sector of the
   // disk instead.
   if (mainCrcOk) {
      if (mainHeader.backupLBA < diskSize) {
         seekTo = mainHeader.backupLBA;
      } else {
         seekTo = diskSize - UINT64_C(1);
         cout << "Warning! Disk size is smaller than the main header indicates! Loading\n"
              << "secondary header from the last sector of the disk! You should use 'v' to\n"
              << "verify disk integrity, and perhaps options on the experts' menu to repair\n"
              << "the disk.\n";
      } // else
   } else {
      seekTo = diskSize - UINT64_C(1);
   } // if/else (mainCrcOk)

   if (myDisk.Seek(seekTo)) {
      if (myDisk.Read(&secondHeader, 512) != 512) { // read secondary GPT header
         cerr << "Warning! Error " << errno << " reading secondary GPT header!\n";
      } // if
      secondCrcOk = CheckHeaderCRC(&secondHeader);
      if (IsLittleEndian() == 0) // big-endian system; adjust header byte order....
         ReverseHeaderBytes(&secondHeader);
   } else {
      allOK = 0;
      state = gpt_invalid;
      cerr << "Unable to seek to secondary GPT header at sector "
           << (diskSize - (UINT64_C(1))) << "!\n";
   } // if/else lseek

   // Return valid headers code: 0 = both headers bad; 1 = main header
   // good, backup bad; 2 = backup header good, main header bad;
   // 3 = both headers good. Note these codes refer to valid GPT
   // signatures and version numbers; more subtle problems will elude
   // this check!
   validHeaders = CheckHeaderValidity();

   // Read partitions (from primary array)
   if (validHeaders > 0) { // if at least one header is OK....
      // GPT appears to be valid....
      state = gpt_valid;

      // We're calling the GPT valid, but there's a possibility that one
      // of the two headers is corrupt. If so, use the one that seems to
      // be in better shape to regenerate the bad one
      if (validHeaders == 1) { // valid main header, invalid backup header
         cerr << "\aCaution: invalid backup GPT header, but valid main header; regenerating\n"
              << "backup header from main header.\n\n";
         RebuildSecondHeader();
         state = gpt_corrupt;
         secondCrcOk = mainCrcOk; // Since regenerated, use CRC validity of main
      } else if (validHeaders == 2) { // valid backup header, invalid main header
         cerr << "\aCaution: invalid main GPT header, but valid backup; regenerating main header\n"
              << "from backup!\n\n";
         RebuildMainHeader();
         state = gpt_corrupt;
         mainCrcOk = secondCrcOk; // Since copied, use CRC validity of backup
      } // if/else/if

      // Figure out which partition table to load....
      // Load the main partition table, since either its header's CRC is OK or the
      // backup header's CRC is not OK....
      if (mainCrcOk || !secondCrcOk) {
         if (LoadMainTable() == 0)
            allOK = 0;
      } else { // bad main header CRC and backup header CRC is OK
         state = gpt_corrupt;
         if (LoadSecondTableAsMain()) {
            cerr << "\aWarning: Invalid CRC on main header data; loaded backup partition table.\n";
         } else { // backup table bad, bad main header CRC, but try main table in desperation....
            if (LoadMainTable() == 0) {
               allOK = 0;
               cerr << "\a\aWarning! Unable to load either main or backup partition table!\n";
            } // if
         } // if/else (LoadSecondTableAsMain())
      } // if/else (load partition table)

      // Load backup partition table into temporary storage to check
      // its CRC and store the results, then discard this temporary
      // storage, since we don't use it in any but recovery operations
      seekTo = secondHeader.partitionEntriesLBA;
      if ((myDisk.Seek(seekTo)) && (secondCrcOk)) {
         sizeOfParts = secondHeader.numParts * secondHeader.sizeOfPartitionEntries;
         storage = (uint8_t*) malloc(sizeOfParts);
         if (myDisk.Read(storage, sizeOfParts) != (int) sizeOfParts) {
            cerr << "Warning! Error " << errno << " reading backup partition table!\n";
         } // if
         newCRC = chksum_crc32((unsigned char*) storage,  sizeOfParts);
         free(storage);
         secondPartsCrcOk = (newCRC == secondHeader.partitionEntriesCRC);
      } // if

      // Problem with main partition table; if backup is OK, use it instead....
      if (secondPartsCrcOk && secondCrcOk && !mainPartsCrcOk) {
         state = gpt_corrupt;
         allOK = allOK && LoadSecondTableAsMain();
         cerr << "\aWarning! Main partition table CRC mismatch! Loaded backup "
              << "partition table\ninstead of main partition table!\n\n";
      } // if

      // Check for valid CRCs and warn if there are problems
      if ((mainCrcOk == 0) || (secondCrcOk == 0) || (mainPartsCrcOk == 0) ||
           (secondPartsCrcOk == 0)) {
         cerr << "Warning! One or more CRCs don't match. You should repair the disk!\n\n";
         state = gpt_corrupt;
      } // if
   } else {
      state = gpt_invalid;
   } // if/else
   return allOK;
} // GPTData::ForceLoadGPTData()

// Loads the partition table pointed to by the main GPT header. The
// main GPT header in memory MUST be valid for this call to do anything
// sensible!
// Returns 1 on success, 0 on failure. CRC errors do NOT count as failure.
int GPTData::LoadMainTable(void) {
   int retval = 1;
   uint32_t newCRC, sizeOfParts;

   if (myDisk.OpenForRead(device)) {
      // Set internal data structures for number of partitions on the disk
      SetGPTSize(mainHeader.numParts);

      // Load main partition table, and record whether its CRC
      // matches the stored value
      if (!myDisk.Seek(mainHeader.partitionEntriesLBA))
         retval = 0;
      sizeOfParts = mainHeader.numParts * mainHeader.sizeOfPartitionEntries;
      if (myDisk.Read(partitions, sizeOfParts) != (int) sizeOfParts) {
         cerr << "Warning! Error " << errno << " when loading the main partition table!\n";
         retval = 0;
      } // if
      newCRC = chksum_crc32((unsigned char*) partitions, sizeOfParts);
      mainPartsCrcOk = (newCRC == mainHeader.partitionEntriesCRC);
      if (IsLittleEndian() == 0)
         ReversePartitionBytes();
   } else retval = 0; // if open for read....
   return retval;
} // GPTData::LoadMainTable()

// Load the second (backup) partition table as the primary partition
// table. Used in repair functions, and when starting up if the main
// partition table is damaged.
// Returns 1 on success, 0 on failure. CRC errors do NOT count as failure.
int GPTData::LoadSecondTableAsMain(void) {
   uint64_t seekTo;
   uint32_t sizeOfParts, newCRC;
   int retval = 1;

   if (myDisk.OpenForRead(device)) {
      seekTo = secondHeader.partitionEntriesLBA;
      retval = myDisk.Seek(seekTo);
      if (retval == 1) {
         SetGPTSize(secondHeader.numParts);
         sizeOfParts = secondHeader.numParts * secondHeader.sizeOfPartitionEntries;
         if (myDisk.Read(partitions, sizeOfParts) != (int) sizeOfParts) {
            cerr << "Warning! Read error " << errno << "! Misbehavior now likely!\n";
            retval = 0;
         } // if
         newCRC = chksum_crc32((unsigned char*) partitions, sizeOfParts);
         secondPartsCrcOk = (newCRC == secondHeader.partitionEntriesCRC);
         mainPartsCrcOk = secondPartsCrcOk;
         if (IsLittleEndian() == 0)
            ReversePartitionBytes();
         if (!secondPartsCrcOk) {
            cout << "Caution! After loading backup partitions, the CRC still doesn't check out!\n";
         } // if
      } else {
         cerr << "Error! Couldn't seek to backup partition table!\n";
      } // if/else
   } else {
      cerr << "Error! Couldn't open device " << device
           << " when recovering backup partition table!\n";
      retval = 0;
   } // if/else
   return retval;
} // GPTData::LoadSecondTableAsMain()

// Writes GPT (and protective MBR) to disk. Returns 1 on successful
// write, 0 if there was a problem.
int GPTData::SaveGPTData(int quiet) {
   int allOK = 1, littleEndian;
   char answer;
   uint64_t secondTable;
   uint32_t numParts;
   uint64_t offset;

   littleEndian = IsLittleEndian();

   if (device == "") {
      cerr << "Device not defined.\n";
   } // if

   // First do some final sanity checks....

   // This test should only fail on read-only disks....
   if (justLooking) {
      cout << "The justLooking flag is set. This probably means you can't write to the disk.\n";
      allOK = 0;
   } // if

   // Is there enough space to hold the GPT headers and partition tables,
   // given the partition sizes?
   if (CheckGPTSize() > 0) {
      allOK = 0;
   } // if

   // Check that disk is really big enough to handle this...
   if (mainHeader.backupLBA > diskSize) {
      cerr << "Error! Disk is too small! The 'e' option on the experts' menu might fix the\n"
           << "problem (or it might not). Aborting!\n(Disk size is "
           << diskSize << " sectors, needs to be " << mainHeader.backupLBA << " sectors.)\n";
      allOK = 0;
   } // if
   // Check that second header is properly placed. Warn and ask if this should
   // be corrected if the test fails....
   if ((mainHeader.backupLBA < (diskSize - UINT64_C(1))) && (quiet == 0)) {
      cout << "Warning! Secondary header is placed too early on the disk! Do you want to\n"
           << "correct this problem? ";
      if (GetYN() == 'Y') {
         MoveSecondHeaderToEnd();
         cout << "Have moved second header and partition table to correct location.\n";
      } else {
         cout << "Have not corrected the problem. Strange problems may occur in the future!\n";
      } // if correction requested
   } // if

   // Check for overlapping partitions....
   if (FindOverlaps() > 0) {
      allOK = 0;
      cerr << "Aborting write operation!\n";
   } // if

   // Check for mismatched MBR and GPT data, but let it pass if found
   // (function displays warning message)
   FindHybridMismatches();

   // Pull out some data that's needed before doing byte-order reversal on
   // big-endian systems....
   numParts = mainHeader.numParts;
   secondTable = secondHeader.partitionEntriesLBA;
/*   if (IsLittleEndian() == 0) {
      // Reverse partition bytes first, since that function requires non-reversed
      // data from the main header....
      ReversePartitionBytes();
      ReverseHeaderBytes(&mainHeader);
      ReverseHeaderBytes(&secondHeader);
   } // if */
   RecomputeCRCs();
/*   ReverseHeaderBytes(&mainHeader);
   ReverseHeaderBytes(&secondHeader);
   ReversePartitionBytes(); */

   if ((allOK) && (!quiet)) {
      cout << "\nFinal checks complete. About to write GPT data. THIS WILL OVERWRITE EXISTING\n"
           << "PARTITIONS!!\n\nDo you want to proceed, possibly destroying your data? ";
      answer = GetYN();
      if (answer == 'Y') {
         cout << "OK; writing new GUID partition table (GPT).\n";
      } else {
         allOK = 0;
      } // if/else
   } // if

   // Do it!
   if (allOK) {
      // First, write the protective MBR...
      allOK = protectiveMBR.WriteMBRData(&myDisk);

      if (allOK && myDisk.OpenForWrite(device)) {
         // Now write the main GPT header...
         if (myDisk.Seek(1) == 1) {
            if (!littleEndian)
               ReverseHeaderBytes(&mainHeader);
            if (myDisk.Write(&mainHeader, 512) != 512)
               allOK = 0;
            if (!littleEndian)
               ReverseHeaderBytes(&mainHeader);
         } else allOK = 0; // if (myDisk.Seek()...)

         // Now write the main partition tables...
         if (allOK) {
	    offset = mainHeader.partitionEntriesLBA;
	    if (myDisk.Seek(offset)) {
               if (!littleEndian)
                  ReversePartitionBytes();
               if (myDisk.Write(partitions, GPT_SIZE * numParts) == -1)
                  allOK = 0;
               if (!littleEndian)
                  ReversePartitionBytes();
            } else allOK = 0; // if (myDisk.Seek()...)
         } // if (allOK)

         // Now seek to near the end to write the secondary GPT....
         if (allOK) {
            offset = (uint64_t) secondTable;
            if (myDisk.Seek(offset) != 1) {
               allOK = 0;
               cerr << "Unable to seek to end of disk! Perhaps the 'e' option on the experts' menu\n"
                    << "will resolve this problem.\n";
            } // if
         } // if

         // Now write the secondary partition tables....
         if (allOK) {
            if (!littleEndian)
               ReversePartitionBytes();
            if (myDisk.Write(partitions, GPT_SIZE * numParts) == -1)
               allOK = 0;
            if (!littleEndian)
               ReversePartitionBytes();
         } // if (allOK)

         // Now write the secondary GPT header...
         if (allOK) {
	    offset = mainHeader.backupLBA;
            if (!littleEndian)
               ReverseHeaderBytes(&secondHeader);
            if (myDisk.Seek(offset)) {
               if (myDisk.Write(&secondHeader, 512) == -1)
                  allOK = 0;
	    } else allOK = 0; // if (myDisk.Seek()...)
            if (!littleEndian)
               ReverseHeaderBytes(&secondHeader);
         } // if (allOK)

         // re-read the partition table
         if (allOK) {
            myDisk.DiskSync();
         } // if

         if (allOK) { // writes completed OK
            cout << "The operation has completed successfully.\n";
         } else {
            cerr << "Warning! An error was reported when writing the partition table! This error\n"
                 << "MIGHT be harmless, but you may have trashed the disk! Use parted and, if\n"
                 << "necessary, restore your original partition table.\n";
         } // if/else
         myDisk.Close();
      } else {
         cerr << "Unable to open device " << device << " for writing! Errno is "
              << errno << "! Aborting write!\n";
         allOK = 0;
      } // if/else
   } else {
      cout << "Aborting write of new partition table.\n";
   } // if

/*   if (IsLittleEndian() == 0) {
      // Reverse (normalize) header bytes first, since ReversePartitionBytes()
      // requires non-reversed data in mainHeader...
      ReverseHeaderBytes(&mainHeader);
      ReverseHeaderBytes(&secondHeader);
      ReversePartitionBytes();
   } // if */

   return (allOK);
} // GPTData::SaveGPTData()

// Save GPT data to a backup file. This function does much less error
// checking than SaveGPTData(). It can therefore preserve many types of
// corruption for later analysis; however, it preserves only the MBR,
// the main GPT header, the backup GPT header, and the main partition
// table; it discards the backup partition table, since it should be
// identical to the main partition table on healthy disks.
int GPTData::SaveGPTBackup(const string & filename) {
   int allOK = 1;
   uint32_t numParts;
   DiskIO backupFile;

   if (backupFile.OpenForWrite(filename)) {
      // Recomputing the CRCs is likely to alter them, which could be bad
      // if the intent is to save a potentially bad GPT for later analysis;
      // but if we don't do this, we get bogus errors when we load the
      // backup. I'm favoring misses over false alarms....
      RecomputeCRCs();

      // Reverse the byte order, if necessary....
      numParts = mainHeader.numParts;
      if (IsLittleEndian() == 0) {
         ReversePartitionBytes();
         ReverseHeaderBytes(&mainHeader);
         ReverseHeaderBytes(&secondHeader);
      } // if

      // Now write the protective MBR...
      protectiveMBR.WriteMBRData(&backupFile);

      // Now write the main GPT header...
      if (allOK)
         // MBR write closed disk, so re-open and seek to end....
         backupFile.OpenForWrite();
         backupFile.Seek(1);
         if (backupFile.Write(&mainHeader, 512) == -1)
            allOK = 0;

      // Now write the secondary GPT header...
      if (allOK)
         if (backupFile.Write(&secondHeader, 512) == -1)
            allOK = 0;

      // Now write the main partition tables...
      if (allOK) {
         if (backupFile.Write(partitions, GPT_SIZE * numParts) == -1)
            allOK = 0;
      } // if

      if (allOK) { // writes completed OK
         cout << "The operation has completed successfully.\n";
      } else {
         cerr << "Warning! An error was reported when writing the backup file.\n"
              << "It may not be usable!\n";
      } // if/else
      backupFile.Close();

      // Now reverse the byte-order reversal, if necessary....
      if (IsLittleEndian() == 0) {
         ReverseHeaderBytes(&mainHeader);
         ReverseHeaderBytes(&secondHeader);
         ReversePartitionBytes();
      } // if
   } else {
      cerr << "Unable to open file " << filename << " for writing! Aborting!\n";
      allOK = 0;
   } // if/else
   return allOK;
} // GPTData::SaveGPTBackup()

// Load GPT data from a backup file created by SaveGPTBackup(). This function
// does minimal error checking. It returns 1 if it completed successfully,
// 0 if there was a problem. In the latter case, it creates a new empty
// set of partitions.
int GPTData::LoadGPTBackup(const string & filename) {
   int allOK = 1, val;
   uint32_t numParts, sizeOfEntries, sizeOfParts, newCRC;
   int littleEndian = 1;
   DiskIO backupFile;

   if (backupFile.OpenForRead(filename)) {
      if (IsLittleEndian() == 0)
         littleEndian = 0;

      // Let the MBRData class load the saved MBR...
      protectiveMBR.ReadMBRData(&backupFile, 0); // 0 = don't check block size

      // Load the main GPT header, check its vaility, and set the GPT
      // size based on the data
      if (backupFile.Read(&mainHeader, 512) != 512) {
         cerr << "Warning! Read error " << errno << "; strange behavior now likely!\n";
      } // if
      mainCrcOk = CheckHeaderCRC(&mainHeader);

      // Reverse byte order, if necessary
      if (littleEndian == 0) {
         ReverseHeaderBytes(&mainHeader);
      } // if

      // Load the backup GPT header in much the same way as the main
      // GPT header....
      if (backupFile.Read(&secondHeader, 512) != 512) {
         cerr << "Warning! Read error " << errno << "; strange behavior now likely!\n";
      } // if
      secondCrcOk = CheckHeaderCRC(&secondHeader);

      // Reverse byte order, if necessary
      if (littleEndian == 0) {
         ReverseHeaderBytes(&secondHeader);
      } // if

      // Return valid headers code: 0 = both headers bad; 1 = main header
      // good, backup bad; 2 = backup header good, main header bad;
      // 3 = both headers good. Note these codes refer to valid GPT
      // signatures and version numbers; more subtle problems will elude
      // this check!
      if ((val = CheckHeaderValidity()) > 0) {
         if (val == 2) { // only backup header seems to be good
            numParts = secondHeader.numParts;
            sizeOfEntries = secondHeader.sizeOfPartitionEntries;
         } else { // main header is OK
            numParts = mainHeader.numParts;
            sizeOfEntries = mainHeader.sizeOfPartitionEntries;
         } // if/else

         SetGPTSize(numParts);

         // If current disk size doesn't match that of backup....
         if (secondHeader.currentLBA != diskSize - UINT64_C(1)) {
            cout << "Warning! Current disk size doesn't match that of the backup!\n"
                 << "Adjusting sizes to match, but subsequent problems are possible!\n";
            MoveSecondHeaderToEnd();
         } // if

         // Load main partition table, and record whether its CRC
         // matches the stored value
         sizeOfParts = numParts * sizeOfEntries;
         if (backupFile.Read(partitions, sizeOfParts) != (int) sizeOfParts) {
            cerr << "Warning! Read error " << errno << "; strange behavior now likely!\n";
         } // if

         newCRC = chksum_crc32((unsigned char*) partitions, sizeOfParts);
         mainPartsCrcOk = (newCRC == mainHeader.partitionEntriesCRC);
         secondPartsCrcOk = (newCRC == secondHeader.partitionEntriesCRC);
         // Reverse byte order, if necessary
         if (littleEndian == 0) {
            ReversePartitionBytes();
         } // if

      } else {
         allOK = 0;
      } // if/else
   } else {
      allOK = 0;
      cerr << "Unable to open file " << filename << " for reading! Aborting!\n";
   } // if/else

   // Something went badly wrong, so blank out partitions
   if (allOK == 0) {
      ClearGPTData();
      protectiveMBR.MakeProtectiveMBR();
   } // if
   return allOK;
} // GPTData::LoadGPTBackup()

// Tell user whether Apple Partition Map (APM) was discovered....
void GPTData::ShowAPMState(void) {
   if (apmFound)
      cout << "  APM: present\n";
   else
      cout << "  APM: not present\n";
} // GPTData::ShowAPMState()

// Tell user about the state of the GPT data....
void GPTData::ShowGPTState(void) {
   switch (state) {
      case gpt_invalid:
         cout << "  GPT: not present\n";
         break;
      case gpt_valid:
         cout << "  GPT: present\n";
         break;
      case gpt_corrupt:
         cout << "  GPT: damaged\n";
         break;
      default:
         cout << "\a  GPT: unknown -- bug!\n";
         break;
   } // switch
} // GPTData::ShowGPTState()

// Display the basic GPT data
void GPTData::DisplayGPTData(void) {
   uint32_t i;
   uint64_t temp, totalFree;

   cout << "Disk " << device << ": " << diskSize << " sectors, "
        << BytesToSI(diskSize * blockSize) << "\n";
   cout << "Logical sector size: " << blockSize << " bytes\n";
   cout << "Disk identifier (GUID): " << mainHeader.diskGUID.AsString() << "\n";
   cout << "Partition table holds up to " << mainHeader.numParts << " entries\n";
   cout << "First usable sector is " << mainHeader.firstUsableLBA
        << ", last usable sector is " << mainHeader.lastUsableLBA << "\n";
   totalFree = FindFreeBlocks(&i, &temp);
   cout << "Total free space is " << totalFree << " sectors ("
        << BytesToSI(totalFree * (uint64_t) blockSize) << ")\n";
   cout << "\nNumber  Start (sector)    End (sector)  Size       Code  Name\n";
   for (i = 0; i < mainHeader.numParts; i++) {
      partitions[i].ShowSummary(i, blockSize);
   } // for
} // GPTData::DisplayGPTData()

// Get partition number from user and then call ShowPartDetails(partNum)
// to show its detailed information
void GPTData::ShowDetails(void) {
   int partNum;
   uint32_t low, high;

   if (GetPartRange(&low, &high) > 0) {
      partNum = GetPartNum();
      ShowPartDetails(partNum);
   } else {
      cout << "No partitions\n";
   } // if/else
} // GPTData::ShowDetails()

// Show detailed information on the specified partition
void GPTData::ShowPartDetails(uint32_t partNum) {
   if (partitions[partNum].GetFirstLBA() != 0) {
      partitions[partNum].ShowDetails(blockSize);
   } else {
      cout << "Partition #" << partNum + 1 << " does not exist.";
   } // if
} // GPTData::ShowPartDetails()

/*********************************************************************
 *                                                                   *
 * Begin functions that obtain information from the users, and often *
 * do something with that information (call other functions)         *
 *                                                                   *
 *********************************************************************/

// Prompts user for partition number and returns the result.
uint32_t GPTData::GetPartNum(void) {
   uint32_t partNum;
   uint32_t low, high;
   char prompt[255];

   if (GetPartRange(&low, &high) > 0) {
      sprintf(prompt, "Partition number (%d-%d): ", low + 1, high + 1);
      partNum = GetNumber(low + 1, high + 1, low, prompt);
   } else partNum = 1;
   return (partNum - 1);
} // GPTData::GetPartNum()

// What it says: Resize the partition table. (Default is 128 entries.)
void GPTData::ResizePartitionTable(void) {
   int newSize;
   char prompt[255];
   uint32_t curLow, curHigh;

   cout << "Current partition table size is " << mainHeader.numParts << ".\n";
   GetPartRange(&curLow, &curHigh);
   curHigh++; // since GetPartRange() returns numbers starting from 0...
   // There's no point in having fewer than four partitions....
   if (curHigh < 4)
      curHigh = 4;
   sprintf(prompt, "Enter new size (%d up, default %d): ", (int) curHigh,
           (int) NUM_GPT_ENTRIES);
   newSize = GetNumber(4, 65535, 128, prompt);
   if (newSize < 128) {
      cout << "Caution: The partition table size should officially be 16KB or larger,\n"
           << "which works out to 128 entries. In practice, smaller tables seem to\n"
           << "work with most OSes, but this practice is risky. I'm proceeding with\n"
           << "the resize, but you may want to reconsider this action and undo it.\n\n";
   } // if
   SetGPTSize(newSize);
} // GPTData::ResizePartitionTable()

// Interactively create a partition
void GPTData::CreatePartition(void) {
   uint64_t firstBlock, firstInLargest, lastBlock, sector;
   uint32_t firstFreePart = 0;
   char prompt[255];
   int partNum;

   // Find first free partition...
   while (partitions[firstFreePart].GetFirstLBA() != 0) {
      firstFreePart++;
   } // while

   if (((firstBlock = FindFirstAvailable()) != 0) &&
         (firstFreePart < mainHeader.numParts)) {
      lastBlock = FindLastAvailable(firstBlock);
      firstInLargest = FindFirstInLargest();

      // Get partition number....
      do {
         sprintf(prompt, "Partition number (%d-%d, default %d): ", firstFreePart + 1,
                 mainHeader.numParts, firstFreePart + 1);
         partNum = GetNumber(firstFreePart + 1, mainHeader.numParts,
                             firstFreePart + 1, prompt) - 1;
         if (partitions[partNum].GetFirstLBA() != 0)
            cout << "partition " << partNum + 1 << " is in use.\n";
      } while (partitions[partNum].GetFirstLBA() != 0);

      // Get first block for new partition...
      sprintf(prompt, "First sector (%llu-%llu, default = %llu) or {+-}size{KMGT}: ",
              (unsigned long long) firstBlock, (unsigned long long) lastBlock,
              (unsigned long long) firstInLargest);
      do {
         sector = GetSectorNum(firstBlock, lastBlock, firstInLargest, prompt);
      } while (IsFree(sector) == 0);
      Align(&sector); // Align sector to correct multiple
      firstBlock = sector;

      // Get last block for new partitions...
      lastBlock = FindLastInFree(firstBlock);
      sprintf(prompt, "Last sector (%llu-%llu, default = %llu) or {+-}size{KMGT}: ",
              (unsigned long long) firstBlock, (unsigned long long) lastBlock,
              (unsigned long long) lastBlock);
      do {
         sector = GetSectorNum(firstBlock, lastBlock, lastBlock, prompt);
      } while (IsFree(sector) == 0);
      lastBlock = sector;

      firstFreePart = CreatePartition(partNum, firstBlock, lastBlock);
      partitions[partNum].ChangeType();
      partitions[partNum].SetDefaultDescription();
   } else {
      cout << "No free sectors available\n";
   } // if/else
} // GPTData::CreatePartition()

// Interactively delete a partition (duh!)
void GPTData::DeletePartition(void) {
   int partNum;
   uint32_t low, high;
   char prompt[255];

   if (GetPartRange(&low, &high) > 0) {
      sprintf(prompt, "Partition number (%d-%d): ", low + 1, high + 1);
      partNum = GetNumber(low + 1, high + 1, low, prompt);
      DeletePartition(partNum - 1);
   } else {
      cout << "No partitions\n";
   } // if/else
} // GPTData::DeletePartition()

// Prompt user for a partition number, then change its type code
// using ChangeGPTType(struct GPTPartition*) function.
void GPTData::ChangePartType(void) {
   int partNum;
   uint32_t low, high;

   if (GetPartRange(&low, &high) > 0) {
      partNum = GetPartNum();
      partitions[partNum].ChangeType();
   } else {
      cout << "No partitions\n";
   } // if/else
} // GPTData::ChangePartType()

// Partition attributes seem to be rarely used, but I want a way to
// adjust them for completeness....
void GPTData::SetAttributes(uint32_t partNum) {
   Attributes theAttr;

   theAttr.SetAttributes(partitions[partNum].GetAttributes());
   theAttr.DisplayAttributes();
   theAttr.ChangeAttributes();
   partitions[partNum].SetAttributes(theAttr.GetAttributes());
} // GPTData::SetAttributes()

// This function destroys the on-disk GPT structures. Returns 1 if the
// user confirms destruction, 0 if the user aborts.
// If prompt == 0, don't ask user about proceeding and do NOT wipe out
// MBR. (Set prompt == 0 when doing a GPT-to-MBR conversion.)
// If prompt == -1, don't ask user about proceeding and DO wipe out
// MBR.
int GPTData::DestroyGPT(int prompt) {
   int i, sum, tableSize;
   uint8_t blankSector[512], goOn = 'Y', blank = 'N';
   uint8_t* emptyTable;

   for (i = 0; i < 512; i++) {
      blankSector[i] = 0;
   } // for

   if (((apmFound) || (bsdFound)) && (prompt > 0)) {
      cout << "WARNING: APM or BSD disklabel structures detected! This operation could\n"
           << "damage any APM or BSD partitions on this disk!\n";
   } // if APM or BSD
   if (prompt > 0) {
      cout << "\a\aAbout to wipe out GPT on " << device << ". Proceed? ";
      goOn = GetYN();
   } // if
   if (goOn == 'Y') {
      if (myDisk.OpenForWrite(device)) {
         myDisk.Seek(mainHeader.currentLBA); // seek to GPT header
         if (myDisk.Write(blankSector, 512) != 512) { // blank it out
            cerr << "Warning! GPT main header not overwritten! Error is " << errno << "\n";
         } // if
         myDisk.Seek(mainHeader.partitionEntriesLBA); // seek to partition table
         tableSize = mainHeader.numParts * mainHeader.sizeOfPartitionEntries;
         emptyTable = (uint8_t*) malloc(tableSize);
         for (i = 0; i < tableSize; i++)
            emptyTable[i] = 0;
         sum = myDisk.Write(emptyTable, tableSize);
         if (sum != tableSize)
            cerr << "Warning! GPT main partition table not overwritten! Error is " << errno << "\n";
         myDisk.Seek(secondHeader.partitionEntriesLBA); // seek to partition table
         sum = myDisk.Write(emptyTable, tableSize);
         if (sum != tableSize)
            cerr << "Warning! GPT backup partition table not overwritten! Error is " << errno << "\n";
         myDisk.Seek(secondHeader.currentLBA); // seek to GPT header
         if (myDisk.Write(blankSector, 512) != 512) { // blank it out
            cerr << "Warning! GPT backup header not overwritten! Error is " << errno << "\n";
         } // if
         if (prompt > 0) {
            cout << "Blank out MBR? ";
            blank = GetYN();
         } // if
         // Note on below: Touch the MBR only if the user wants it completely
         // blanked out. Version 0.4.2 deleted the 0xEE partition and re-wrote
         // the MBR, but this could wipe out a valid MBR that the program
         // had subsequently discarded (say, if it conflicted with older GPT
         // structures).
         if ((blank == 'Y') || (prompt < 0)) {
            myDisk.Seek(0);
            if (myDisk.Write(blankSector, 512) != 512) { // blank it out
               cerr << "Warning! MBR not overwritten! Error is " << errno << "!\n";
            } // if
         } else {
            cout << "MBR is unchanged. You may need to delete an EFI GPT (0xEE) partition\n"
                 << "with fdisk or another tool.\n";
         } // if/else
         myDisk.DiskSync();
         myDisk.Close();
         cout << "GPT data structures destroyed! You may now partition the disk using fdisk or\n"
              << "other utilities. Program will now terminate.\n";
      } else {
         cerr << "Problem opening " << device << " for writing! Program will now terminate.\n";
      } // if/else (fd != -1)
   } // if (goOn == 'Y')
   return (goOn == 'Y');
} // GPTData::DestroyGPT()

/**************************************************************************
 *                                                                        *
 * Partition table transformation functions (MBR or BSD disklabel to GPT) *
 * (some of these functions may require user interaction)                 *
 *                                                                        *
 **************************************************************************/

// Examines the MBR & GPT data, and perhaps asks the user questions, to
// determine which set of data to use: the MBR (use_mbr), the GPT (use_gpt),
// or create a new set of partitions (use_new)
WhichToUse GPTData::UseWhichPartitions(void) {
   WhichToUse which = use_new;
   MBRValidity mbrState;
   int answer;

   mbrState = protectiveMBR.GetValidity();

   if ((state == gpt_invalid) && ((mbrState == mbr) || (mbrState == hybrid))) {
      cout << "\n***************************************************************\n"
           << "Found invalid GPT and valid MBR; converting MBR to GPT format.\n";
      if (!justLooking) {
         cout << "\aTHIS OPERATON IS POTENTIALLY DESTRUCTIVE! Exit by typing 'q' if\n"
              << "you don't want to convert your MBR partitions to GPT format!\n";
      } // if
      cout << "***************************************************************\n\n";
      which = use_mbr;
   } // if

   if ((state == gpt_invalid) && bsdFound) {
      cout << "\n**********************************************************************\n"
           << "Found invalid GPT and valid BSD disklabel; converting BSD disklabel\n"
           << "to GPT format.";
      if ((!justLooking) && (!beQuiet)) {
      cout << "\a THIS OPERATON IS POTENTIALLY DESTRUCTIVE! Your first\n"
           << "BSD partition will likely be unusable. Exit by typing 'q' if you don't\n"
           << "want to convert your BSD partitions to GPT format!";
      } // if
      cout << "\n**********************************************************************\n\n";
      which = use_bsd;
   } // if

   if ((state == gpt_valid) && (mbrState == gpt)) {
      which = use_gpt;
      if (!beQuiet)
         cout << "Found valid GPT with protective MBR; using GPT.\n";
   } // if
   if ((state == gpt_valid) && (mbrState == hybrid)) {
      which = use_gpt;
      if (!beQuiet)
         cout << "Found valid GPT with hybrid MBR; using GPT.\n";
   } // if
   if ((state == gpt_valid) && (mbrState == invalid)) {
      cout << "\aFound valid GPT with corrupt MBR; using GPT and will write new\n"
           << "protective MBR on save.\n";
      which = use_gpt;
   } // if
   if ((state == gpt_valid) && (mbrState == mbr)) {
      if (!beQuiet) {
         cout << "Found valid MBR and GPT. Which do you want to use?\n";
         answer = GetNumber(1, 3, 2, " 1 - MBR\n 2 - GPT\n 3 - Create blank GPT\n\nYour answer: ");
         if (answer == 1) {
            which = use_mbr;
         } else if (answer == 2) {
            which = use_gpt;
            cout << "Using GPT and creating fresh protective MBR.\n";
         } else which = use_new;
      } else which = use_abort;
   } // if

   // Nasty decisions here -- GPT is present, but corrupt (bad CRCs or other
   // problems)
   if (state == gpt_corrupt) {
      if (beQuiet) {
         which = use_abort;
      } else {
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
         } else { // corrupt GPT, MBR indicates it's a GPT disk....
            cout << "\a\a****************************************************************************\n"
                 << "Caution: Found protective or hybrid MBR and corrupt GPT. Using GPT, but disk\n"
                 << "verification and recovery are STRONGLY recommended.\n"
                 << "****************************************************************************\n";
            which = use_gpt;
         } // if/else/else
      } // else (beQuiet)
   } // if (corrupt GPT)

   if (which == use_new)
      cout << "Creating new GPT entries.\n";

   return which;
} // UseWhichPartitions()

// Convert MBR partition table into GPT form
int GPTData::XFormPartitions(void) {
   int i, numToConvert;
   uint8_t origType;

   // Clear out old data & prepare basics....
   ClearGPTData();

   // Convert the smaller of the # of GPT or MBR partitions
   if (mainHeader.numParts > (MAX_MBR_PARTS))
      numToConvert = MAX_MBR_PARTS;
   else
      numToConvert = mainHeader.numParts;

   for (i = 0; i < numToConvert; i++) {
      origType = protectiveMBR.GetType(i);
      // don't waste CPU time trying to convert extended, hybrid protective, or
      // null (non-existent) partitions
      if ((origType != 0x05) && (origType != 0x0f) && (origType != 0x85) &&
          (origType != 0x00) && (origType != 0xEE))
         partitions[i] = protectiveMBR.AsGPT(i);
   } // for

   // Convert MBR into protective MBR
   protectiveMBR.MakeProtectiveMBR();

   // Record that all original CRCs were OK so as not to raise flags
   // when doing a disk verification
   mainCrcOk = secondCrcOk = mainPartsCrcOk = secondPartsCrcOk = 1;

   return (1);
} // GPTData::XFormPartitions()

// Transforms BSD disklabel on the specified partition (numbered from 0).
// If an invalid partition number is given, the program prompts for one.
// (Default for i is -1; called without an option, it therefore prompts.)
// Returns the number of new partitions created.
int GPTData::XFormDisklabel(int i) {
   uint32_t low, high, partNum, startPart;
   uint16_t hexCode;
   int goOn = 1, numDone = 0;
   BSDData disklabel;

   if (GetPartRange(&low, &high) != 0) {
      if ((i < (int) low) || (i > (int) high))
         partNum = GetPartNum();
      else
         partNum = (uint32_t) i;

      // Find the partition after the last used one
      startPart = high + 1;

      // Now see if the specified partition has a BSD type code....
      hexCode = partitions[partNum].GetHexType();
      if ((hexCode != 0xa500) && (hexCode != 0xa900)) {
         cout << "Specified partition doesn't have a disklabel partition type "
              << "code.\nContinue anyway? ";
         goOn = (GetYN() == 'Y');
      } // if

      // If all is OK, read the disklabel and convert it.
      if (goOn) {
         goOn = disklabel.ReadBSDData(&myDisk, partitions[partNum].GetFirstLBA(),
                                      partitions[partNum].GetLastLBA());
         if ((goOn) && (disklabel.IsDisklabel())) {
            numDone = XFormDisklabel(&disklabel, startPart);
            if (numDone == 1)
               cout << "Converted " << numDone << " BSD partition.\n";
            else
               cout << "Converted " << numDone << " BSD partitions.\n";
         } else {
            cout << "Unable to convert partitions! Unrecognized BSD disklabel.\n";
         } // if/else
      } // if
      if (numDone > 0) { // converted partitions; delete carrier
         partitions[partNum].BlankPartition();
      } // if
   } else {
      cout << "No partitions\n";
   } // if/else
   return numDone;
} // GPTData::XFormDisklable(int i)

// Transform the partitions on an already-loaded BSD disklabel...
int GPTData::XFormDisklabel(BSDData* disklabel, uint32_t startPart) {
   int i, numDone = 0;

   if ((disklabel->IsDisklabel()) && (startPart >= 0) &&
       (startPart < mainHeader.numParts)) {
      for (i = 0; i < disklabel->GetNumParts(); i++) {
         partitions[i + startPart] = disklabel->AsGPT(i);
         if (partitions[i + startPart].GetFirstLBA() != UINT64_C(0))
            numDone++;
      } // for
   } // if

   // Record that all original CRCs were OK so as not to raise flags
   // when doing a disk verification
   mainCrcOk = secondCrcOk = mainPartsCrcOk = secondPartsCrcOk = 1;

   return numDone;
} // GPTData::XFormDisklabel(BSDData* disklabel)

// Add one GPT partition to MBR. Used by XFormToMBR() and MakeHybrid()
// functions. Returns 1 if operation was successful.
int GPTData::OnePartToMBR(uint32_t gptPart, int mbrPart) {
   int allOK = 1, typeCode, bootable;
   uint64_t length;
   char line[255];
   char* junk;

   cout.setf(ios::uppercase);
   cout.fill('0');

   if ((mbrPart < 0) || (mbrPart > 3)) {
      cout << "MBR partition " << mbrPart + 1 << " is out of range; omitting it.\n";
      allOK = 0;
   } // if
   if (gptPart >= mainHeader.numParts) {
      cout << "GPT partition " << gptPart + 1 << " is out of range; omitting it.\n";
      allOK = 0;
   } // if
   if (allOK && (partitions[gptPart].GetLastLBA() == UINT64_C(0))) {
      cout << "GPT partition " << gptPart + 1 << " is undefined; omitting it.\n";
      allOK = 0;
   } // if
   if (allOK && (partitions[gptPart].GetFirstLBA() <= UINT32_MAX) &&
       (partitions[gptPart].GetLengthLBA() <= UINT32_MAX)) {
      if (partitions[gptPart].GetLastLBA() > UINT32_MAX) {
         cout << "Caution: Partition end point past 32-bit pointer boundary;"
              << " some OSes may\nreact strangely.\n";
      } // if partition ends past 32-bit (usually 2TiB) boundary
      do {
         cout << "Enter an MBR hex code (default " << hex;
         cout.width(2);
         cout << partitions[gptPart].GetHexType() / 0x0100 << "): ";
         junk = fgets(line, 255, stdin);
         if (line[0] == '\n')
            typeCode = partitions[gptPart].GetHexType() / 256;
         else
            sscanf(line, "%x", &typeCode);
      } while ((typeCode <= 0) || (typeCode > 255));
      cout << "Set the bootable flag? ";
      bootable = (GetYN() == 'Y');
      length = partitions[gptPart].GetLengthLBA();
      protectiveMBR.MakePart(mbrPart, (uint32_t) partitions[gptPart].GetFirstLBA(),
                             (uint32_t) length, typeCode, bootable);
   } else { // partition out of range
      cout << "Partition " << gptPart + 1 << " begins beyond the 32-bit pointer limit of MBR "
           << "partitions, or is\n too big; omitting it.\n";
      allOK = 0;
   } // if/else
   cout.fill(' ');
   return allOK;
} // GPTData::OnePartToMBR()

// Convert the GPT to MBR form. This function is necessarily limited; it
// handles at most four partitions and creates layouts that ignore CHS
// geometries. Returns the number of converted partitions; if this value
// is over 0, the calling function should call DestroyGPT() to destroy
// the GPT data, and then exit.
int GPTData::XFormToMBR(void) {
   char line[255];
   char* junk;
   int j, numParts, numConverted = 0;
   uint32_t i, partNums[4];

   // Get the numbers of up to four partitions to add to the
   // hybrid MBR....
   numParts = CountParts();
   cout << "Counted " << numParts << " partitions.\n";

   // Prepare the MBR for conversion (empty it of existing partitions).
   protectiveMBR.EmptyMBR(0);
   protectiveMBR.SetDiskSize(diskSize);

   if (numParts > 4) { // Over four partitions; engage in triage
      cout << "Type from one to four GPT partition numbers, separated by spaces, to be\n"
           << "used in the MBR, in sequence: ";
      junk = fgets(line, 255, stdin);
      numParts = sscanf(line, "%d %d %d %d", &partNums[0], &partNums[1],
                        &partNums[2], &partNums[3]);
   } else { // Four or fewer partitions; convert them all
      i = j = 0;
      while ((j < numParts) && (i < mainHeader.numParts)) {
         if (partitions[i].GetFirstLBA() > 0) { // if GPT part. is defined
            partNums[j++] = ++i; // flag it for conversion
         } else i++;
      } // while
   } // if/else

   for (i = 0; i < (uint32_t) numParts; i++) {
      j = partNums[i] - 1;
      cout << "\nCreating entry for partition #" << j + 1 << "\n";
      numConverted += OnePartToMBR(j, i);
   } // for
   cout << "MBR writing returned " << protectiveMBR.WriteMBRData(&myDisk) << "\n";
   return numConverted;
} // GPTData::XFormToMBR()

// Create a hybrid MBR -- an ugly, funky thing that helps GPT work with
// OSes that don't understand GPT.
void GPTData::MakeHybrid(void) {
   uint32_t partNums[3];
   char line[255];
   char* junk;
   int numParts, numConverted = 0, i, j, typeCode, mbrNum;
   char fillItUp = 'M'; // fill extra partition entries? (Yes/No/Maybe)
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
      // Blank out the protective MBR, but leave the boot loader code
      // alone....
      protectiveMBR.EmptyMBR(0);
      protectiveMBR.SetDiskSize(diskSize);
      cout << "Place EFI GPT (0xEE) partition first in MBR (good for GRUB)? ";
      eeFirst = GetYN();
   } // if

   for (i = 0; i < numParts; i++) {
      j = partNums[i] - 1;
      cout << "\nCreating entry for partition #" << j + 1 << "\n";
      if (eeFirst == 'Y')
         mbrNum = i + 1;
      else
         mbrNum = i;
      numConverted += OnePartToMBR(j, mbrNum);
   } // for

   if ((numParts > 0) && (numConverted > 0)) { // User opted to create a hybrid MBR....
      // Create EFI protective partition that covers the start of the disk.
      // If this location (covering the main GPT data structures) is omitted,
      // Linux won't find any partitions on the disk. Note that this is
      // NUMBERED AFTER the hybrid partitions, contrary to what the
      // gptsync utility does. This is because Windows seems to choke on
      // disks with a 0xEE partition in the first slot and subsequent
      // additional partitions, unless it boots from the disk.
      if (eeFirst == 'Y')
         mbrNum = 0;
      else
         mbrNum = numParts;
      protectiveMBR.MakePart(mbrNum, 1, protectiveMBR.FindLastInFree(1), 0xEE);
      protectiveMBR.SetHybrid();

      // ... and for good measure, if there are any partition spaces left,
      // optionally create another protective EFI partition to cover as much
      // space as possible....
      for (i = 0; i < 4; i++) {
         if (protectiveMBR.GetType(i) == 0x00) { // unused entry....
            if (fillItUp == 'M') {
               cout << "\nUnused partition space(s) found. Use one to protect more partitions? ";
               fillItUp = GetYN();
               typeCode = 0x00; // use this to flag a need to get type code
            } // if
            if (fillItUp == 'Y') {
               while ((typeCode <= 0) || (typeCode > 255)) {
                  cout << "Enter an MBR hex code (EE is EFI GPT, but may confuse MacOS): ";
                  // Comment on above: Mac OS treats disks with more than one
                  // 0xEE MBR partition as MBR disks, not as GPT disks.
                  junk = fgets(line, 255, stdin);
                  sscanf(line, "%x", &typeCode);
                  if (line[0] == '\n')
                     typeCode = 0;
               } // while
               protectiveMBR.MakeBiggestPart(i, typeCode); // make a partition
            } // if (fillItUp == 'Y')
         } // if unused entry
      } // for (i = 0; i < 4; i++)
   } // if (numParts > 0)
} // GPTData::MakeHybrid()

/**********************************************************************
 *                                                                    *
 * Functions that adjust GPT data structures WITHOUT user interaction *
 * (they may display information for the user's benefit, though)      *
 *                                                                    *
 **********************************************************************/

// Resizes GPT to specified number of entries. Creates a new table if
// necessary, copies data if it already exists. Returns 1 if all goes
// well, 0 if an error is encountered.
int GPTData::SetGPTSize(uint32_t numEntries) {
   struct GPTPart* newParts;
   struct GPTPart* trash;
   uint32_t i, high, copyNum;
   int allOK = 1;

   // First, adjust numEntries upward, if necessary, to get a number
   // that fills the allocated sectors
   i = blockSize / GPT_SIZE;
   if ((numEntries % i) != 0) {
      cout << "Adjusting GPT size from " << numEntries << " to ";
      numEntries = ((numEntries / i) + 1) * i;
      cout << numEntries << " to fill the sector\n";
   } // if

   // Do the work only if the # of partitions is changing. Along with being
   // efficient, this prevents mucking the with location of the secondary
   // partition table, which causes problems when loading data from a RAID
   // array that's been expanded because this function is called when loading
   // data.
   if ((numEntries != mainHeader.numParts) || (numEntries != secondHeader.numParts)
       || (partitions == NULL)) {
      newParts = (GPTPart*) calloc(numEntries, sizeof (GPTPart));
      if (newParts != NULL) {
         if (partitions != NULL) { // existing partitions; copy them over
            GetPartRange(&i, &high);
            if (numEntries < (high + 1)) { // Highest entry too high for new #
               cout << "The highest-numbered partition is " << high + 1
                    << ", which is greater than the requested\n"
                    << "partition table size of " << numEntries
                    << "; cannot resize. Perhaps sorting will help.\n";
               allOK = 0;
            } else { // go ahead with copy
               if (numEntries < mainHeader.numParts)
                  copyNum = numEntries;
               else
                  copyNum = mainHeader.numParts;
               for (i = 0; i < copyNum; i++) {
                  newParts[i] = partitions[i];
               } // for
               trash = partitions;
               partitions = newParts;
               free(trash);
            } // if
         } else { // No existing partition table; just create it
            partitions = newParts;
         } // if/else existing partitions
         mainHeader.numParts = numEntries;
         secondHeader.numParts = numEntries;
         mainHeader.firstUsableLBA = ((numEntries * GPT_SIZE) / blockSize) + 2 ;
         secondHeader.firstUsableLBA = mainHeader.firstUsableLBA;
         MoveSecondHeaderToEnd();
         if (diskSize > 0)
            CheckGPTSize();
      } else { // Bad memory allocation
         cerr << "Error allocating memory for partition table!\n";
         allOK = 0;
      } // if/else
   } // if/else
   return (allOK);
} // GPTData::SetGPTSize()

// Blank the partition array
void GPTData::BlankPartitions(void) {
   uint32_t i;

   for (i = 0; i < mainHeader.numParts; i++) {
      partitions[i].BlankPartition();
   } // for
} // GPTData::BlankPartitions()

// Delete a partition by number. Returns 1 if successful,
// 0 if there was a problem. Returns 1 if partition was in
// range, 0 if it was out of range.
int GPTData::DeletePartition(uint32_t partNum) {
   uint64_t startSector, length;
   uint32_t low, high, numParts, retval = 1;;

   numParts = GetPartRange(&low, &high);
   if ((numParts > 0) && (partNum >= low) && (partNum <= high)) {
      // In case there's a protective MBR, look for & delete matching
      // MBR partition....
      startSector = partitions[partNum].GetFirstLBA();
      length = partitions[partNum].GetLengthLBA();
      protectiveMBR.DeleteByLocation(startSector, length);

      // Now delete the GPT partition
      partitions[partNum].BlankPartition();
   } else {
      cerr << "Partition number " << partNum + 1 << " out of range!\n";
      retval = 0;
   } // if/else
   return retval;
} // GPTData::DeletePartition(uint32_t partNum)

// Non-interactively create a partition. Note that this function is overloaded
// with another of the same name but different parameters; that one prompts
// the user for data. This one returns 1 if the operation was successful, 0
// if a problem was discovered.
uint32_t GPTData::CreatePartition(uint32_t partNum, uint64_t startSector, uint64_t endSector) {
   int retval = 1; // assume there'll be no problems

   if (IsFreePartNum(partNum)) {
      Align(&startSector); // Align sector to correct multiple
      if (IsFree(startSector) && (startSector <= endSector)) {
         if (FindLastInFree(startSector) >= endSector) {
            partitions[partNum].SetFirstLBA(startSector);
            partitions[partNum].SetLastLBA(endSector);
            partitions[partNum].SetType(0x0700);
            partitions[partNum].RandomizeUniqueGUID();
         } else retval = 0; // if free space until endSector
      } else retval = 0; // if startSector is free
   } else retval = 0; // if legal partition number
   return retval;
} // GPTData::CreatePartition(partNum, startSector, endSector)

// Sort the GPT entries, eliminating gaps and making for a logical
// ordering. Relies on QuickSortGPT() for the bulk of the work
void GPTData::SortGPT(void) {
   GPTPart temp;
   uint32_t i, numFound, firstPart, lastPart;

   // First, find the last partition with data, so as not to
   // spend needless time sorting empty entries....
   numFound = GetPartRange(&firstPart, &lastPart);

   // Now swap empties with the last partitions, to simplify the logic
   // in the Quicksort function....
   i = 0;
   while (i < lastPart) {
      if (partitions[i].GetFirstLBA() == 0) {
         temp = partitions[i];
         partitions[i] = partitions[lastPart];
         partitions[lastPart] = temp;
         do {
            lastPart--;
         } while ((lastPart > 0) && (partitions[lastPart].GetFirstLBA() == 0));
      } // if
      i++;
   } // while

   // If there are more empties than partitions in the range from 0 to lastPart,
   // the above leaves lastPart set too high, so we've got to adjust it to
   // prevent empties from migrating to the top of the list....
   GetPartRange(&firstPart, &lastPart);

   // Now call the recursive quick sort routine to do the real work....
   QuickSortGPT(partitions, 0, lastPart);
} // GPTData::SortGPT()

// Set up data structures for entirely new set of partitions on the
// specified device. Returns 1 if OK, 0 if there were problems.
// Note that this function does NOT clear the protectiveMBR data
// structure, since it may hold the original MBR partitions if the
// program was launched on an MBR disk, and those may need to be
// converted to GPT format.
int GPTData::ClearGPTData(void) {
   int goOn = 1, i;

   // Set up the partition table....
   if (partitions != NULL)
      free(partitions);
   partitions = NULL;
   SetGPTSize(NUM_GPT_ENTRIES);

   // Now initialize a bunch of stuff that's static....
   mainHeader.signature = GPT_SIGNATURE;
   mainHeader.revision = 0x00010000;
   mainHeader.headerSize = HEADER_SIZE;
   mainHeader.reserved = 0;
   mainHeader.currentLBA = UINT64_C(1);
   mainHeader.partitionEntriesLBA = (uint64_t) 2;
   mainHeader.sizeOfPartitionEntries = GPT_SIZE;
   for (i = 0; i < GPT_RESERVED; i++) {
      mainHeader.reserved2[i] = '\0';
   } // for

   // Now some semi-static items (computed based on end of disk)
   mainHeader.backupLBA = diskSize - UINT64_C(1);
   mainHeader.lastUsableLBA = diskSize - mainHeader.firstUsableLBA;

   // Set a unique GUID for the disk, based on random numbers
   mainHeader.diskGUID.Randomize();

   // Copy main header to backup header
   RebuildSecondHeader();

   // Blank out the partitions array....
   BlankPartitions();

   // Flag all CRCs as being OK....
   mainCrcOk = 1;
   secondCrcOk = 1;
   mainPartsCrcOk = 1;
   secondPartsCrcOk = 1;

   return (goOn);
} // GPTData::ClearGPTData()

// Set the location of the second GPT header data to the end of the disk.
// Used internally and called by the 'e' option on the recovery &
// transformation menu, to help users of RAID arrays who add disk space
// to their arrays.
void GPTData::MoveSecondHeaderToEnd() {
   mainHeader.backupLBA = secondHeader.currentLBA = diskSize - UINT64_C(1);
   mainHeader.lastUsableLBA = secondHeader.lastUsableLBA = diskSize - mainHeader.firstUsableLBA;
   secondHeader.partitionEntriesLBA = secondHeader.lastUsableLBA + UINT64_C(1);
} // GPTData::FixSecondHeaderLocation()

int GPTData::SetName(uint32_t partNum, const string & theName) {
   int retval = 1;

   if (!IsFreePartNum(partNum)) {
      partitions[partNum].SetName(theName);
   } else retval = 0;

   return retval;
} // GPTData::SetName

// Set the disk GUID to the specified value. Note that the header CRCs must
// be recomputed after calling this function.
void GPTData::SetDiskGUID(GUIDData newGUID) {
   mainHeader.diskGUID = newGUID;
   secondHeader.diskGUID = newGUID;
} // SetDiskGUID()

// Set the unique GUID of the specified partition. Returns 1 on
// successful completion, 0 if there were problems (invalid
// partition number).
int GPTData::SetPartitionGUID(uint32_t pn, GUIDData theGUID) {
   int retval = 0;

   if (pn < mainHeader.numParts) {
      if (partitions[pn].GetFirstLBA() != UINT64_C(0)) {
         partitions[pn].SetUniqueGUID(theGUID);
         retval = 1;
      } // if
   } // if
   return retval;
} // GPTData::SetPartitionGUID()

// Change partition type code non-interactively. Returns 1 if
// successful, 0 if not....
int GPTData::ChangePartType(uint32_t partNum, uint16_t hexCode) {
   int retval = 1;

   if (!IsFreePartNum(partNum)) {
      partitions[partNum].SetType(hexCode);
   } else retval = 0;
   return retval;
} // GPTData::ChangePartType()

// Adjust sector number so that it falls on a sector boundary that's a
// multiple of sectorAlignment. This is done to improve the performance
// of Western Digital Advanced Format disks and disks with similar
// technology from other companies, which use 4096-byte sectors
// internally although they translate to 512-byte sectors for the
// benefit of the OS. If partitions aren't properly aligned on these
// disks, some filesystem data structures can span multiple physical
// sectors, degrading performance. This function should be called
// only on the FIRST sector of the partition, not the last!
// This function returns 1 if the alignment was altered, 0 if it
// was unchanged.
int GPTData::Align(uint64_t* sector) {
   int retval = 0, sectorOK = 0;
   uint64_t earlier, later, testSector, original;

   if ((*sector % sectorAlignment) != 0) {
      original = *sector;
      retval = 1;
      earlier = (*sector / sectorAlignment) * sectorAlignment;
      later = earlier + (uint64_t) sectorAlignment;

      // Check to see that every sector between the earlier one and the
      // requested one is clear, and that it's not too early....
      if (earlier >= mainHeader.firstUsableLBA) {
         sectorOK = 1;
         testSector = earlier;
         do {
            sectorOK = IsFree(testSector++);
         } while ((sectorOK == 1) && (testSector < *sector));
         if (sectorOK == 1) {
            *sector = earlier;
         } // if
      } // if firstUsableLBA check

      // If couldn't move the sector earlier, try to move it later instead....
      if ((sectorOK != 1) && (later <= mainHeader.lastUsableLBA)) {
         sectorOK = 1;
         testSector = later;
         do {
            sectorOK = IsFree(testSector--);
         } while ((sectorOK == 1) && (testSector > *sector));
         if (sectorOK == 1) {
            *sector = later;
         } // if
      } // if

      // If sector was changed successfully, inform the user of this fact.
      // Otherwise, notify the user that it couldn't be done....
      if (sectorOK == 1) {
         cout << "Information: Moved requested sector from " << original << " to "
              << *sector << " for\nalignment purposes.\n";
         if (!beQuiet)
            cout << "Use 'l' on the experts' menu to adjust alignment\n";
      } else {
         cout << "Information: Sector not aligned on " << sectorAlignment
              << "-sector boundary and could not be moved.\n"
              << "If you're using a Western Digital Advanced Format or similar disk with\n"
              << "underlying 4096-byte sectors, performance may suffer.\n";
         retval = 0;
      } // if/else
   } // if
   return retval;
} // GPTData::Align()

/********************************************************
 *                                                      *
 * Functions that return data about GPT data structures *
 * (most of these are inline in gpt.h)                  *
 *                                                      *
 ********************************************************/

// Find the low and high used partition numbers (numbered from 0).
// Return value is the number of partitions found. Note that the
// *low and *high values are both set to 0 when no partitions
// are found, as well as when a single partition in the first
// position exists. Thus, the return value is the only way to
// tell when no partitions exist.
int GPTData::GetPartRange(uint32_t *low, uint32_t *high) {
   uint32_t i;
   int numFound = 0;

   *low = mainHeader.numParts + 1; // code for "not found"
   *high = 0;
   if (mainHeader.numParts > 0) { // only try if partition table exists...
      for (i = 0; i < mainHeader.numParts; i++) {
         if (partitions[i].GetFirstLBA() != UINT64_C(0)) { // it exists
            *high = i; // since we're counting up, set the high value
	    // Set the low value only if it's not yet found...
            if (*low == (mainHeader.numParts + 1)) *low = i;
            numFound++;
         } // if
      } // for
   } // if

   // Above will leave *low pointing to its "not found" value if no partitions
   // are defined, so reset to 0 if this is the case....
   if (*low == (mainHeader.numParts + 1))
      *low = 0;
   return numFound;
} // GPTData::GetPartRange()

// Returns the number of defined partitions.
uint32_t GPTData::CountParts(void) {
   uint32_t i, counted = 0;

   for (i = 0; i < mainHeader.numParts; i++) {
      if (partitions[i].GetFirstLBA() > 0)
         counted++;
   } // for
   return counted;
} // GPTData::CountParts()

/****************************************************
 *                                                  *
 * Functions that return data about disk free space *
 *                                                  *
 ****************************************************/

// Find the first available block after the starting point; returns 0 if
// there are no available blocks left
uint64_t GPTData::FindFirstAvailable(uint64_t start) {
   uint64_t first;
   uint32_t i;
   int firstMoved = 0;

   // Begin from the specified starting point or from the first usable
   // LBA, whichever is greater...
   if (start < mainHeader.firstUsableLBA)
      first = mainHeader.firstUsableLBA;
   else
      first = start;

   // ...now search through all partitions; if first is within an
   // existing partition, move it to the next sector after that
   // partition and repeat. If first was moved, set firstMoved
   // flag; repeat until firstMoved is not set, so as to catch
   // cases where partitions are out of sequential order....
   do {
      firstMoved = 0;
      for (i = 0; i < mainHeader.numParts; i++) {
         if ((first >= partitions[i].GetFirstLBA()) &&
              (first <= partitions[i].GetLastLBA())) { // in existing part.
            first = partitions[i].GetLastLBA() + 1;
            firstMoved = 1;
              } // if
      } // for
   } while (firstMoved == 1);
   if (first > mainHeader.lastUsableLBA)
      first = 0;
   return (first);
} // GPTData::FindFirstAvailable()

// Finds the first available sector in the largest block of unallocated
// space on the disk. Returns 0 if there are no available blocks left
uint64_t GPTData::FindFirstInLargest(void) {
   uint64_t start, firstBlock, lastBlock, segmentSize, selectedSize = 0, selectedSegment = 0;

   start = 0;
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
   return selectedSegment;
} // GPTData::FindFirstInLargest()

// Find the last available block on the disk at or after the start
// block. Returns 0 if there are no available partitions after
// (or including) start.
uint64_t GPTData::FindLastAvailable(uint64_t start) {
   uint64_t last;
   uint32_t i;
   int lastMoved = 0;

   // Start by assuming the last usable LBA is available....
   last = mainHeader.lastUsableLBA;

   // ...now, similar to algorithm in FindFirstAvailable(), search
   // through all partitions, moving last when it's in an existing
   // partition. Set the lastMoved flag so we repeat to catch cases
   // where partitions are out of logical order.
   do {
      lastMoved = 0;
      for (i = 0; i < mainHeader.numParts; i++) {
         if ((last >= partitions[i].GetFirstLBA()) &&
              (last <= partitions[i].GetLastLBA())) { // in existing part.
            last = partitions[i].GetFirstLBA() - 1;
            lastMoved = 1;
              } // if
      } // for
   } while (lastMoved == 1);
   if (last < mainHeader.firstUsableLBA)
      last = 0;
   return (last);
} // GPTData::FindLastAvailable()

// Find the last available block in the free space pointed to by start.
uint64_t GPTData::FindLastInFree(uint64_t start) {
   uint64_t nearestStart;
   uint32_t i;

   nearestStart = mainHeader.lastUsableLBA;
   for (i = 0; i < mainHeader.numParts; i++) {
      if ((nearestStart > partitions[i].GetFirstLBA()) &&
           (partitions[i].GetFirstLBA() > start)) {
         nearestStart = partitions[i].GetFirstLBA() - 1;
           } // if
   } // for
   return (nearestStart);
} // GPTData::FindLastInFree()

// Finds the total number of free blocks, the number of segments in which
// they reside, and the size of the largest of those segments
uint64_t GPTData::FindFreeBlocks(uint32_t *numSegments, uint64_t *largestSegment) {
   uint64_t start = UINT64_C(0); // starting point for each search
   uint64_t totalFound = UINT64_C(0); // running total
   uint64_t firstBlock; // first block in a segment
   uint64_t lastBlock; // last block in a segment
   uint64_t segmentSize; // size of segment in blocks
   uint32_t num = 0;

   *largestSegment = UINT64_C(0);
   do {
      firstBlock = FindFirstAvailable(start);
      if (firstBlock != UINT64_C(0)) { // something's free...
         lastBlock = FindLastInFree(firstBlock);
         segmentSize = lastBlock - firstBlock + UINT64_C(1);
         if (segmentSize > *largestSegment) {
            *largestSegment = segmentSize;
         } // if
         totalFound += segmentSize;
         num++;
         start = lastBlock + 1;
      } // if
   } while (firstBlock != 0);
   *numSegments = num;
   return totalFound;
} // GPTData::FindFreeBlocks()

// Returns 1 if sector is unallocated, 0 if it's allocated to a partition
int GPTData::IsFree(uint64_t sector) {
   int isFree = 1;
   uint32_t i;

   for (i = 0; i < mainHeader.numParts; i++) {
      if ((sector >= partitions[i].GetFirstLBA()) &&
           (sector <= partitions[i].GetLastLBA())) {
         isFree = 0;
           } // if
   } // for
   if ((sector < mainHeader.firstUsableLBA) ||
        (sector > mainHeader.lastUsableLBA)) {
      isFree = 0;
        } // if
        return (isFree);
} // GPTData::IsFree()

// Returns 1 if partNum is unused.
int GPTData::IsFreePartNum(uint32_t partNum) {
   int retval = 1;

   if ((partNum >= 0) && (partNum < mainHeader.numParts)) {
      if ((partitions[partNum].GetFirstLBA() != UINT64_C(0)) ||
          (partitions[partNum].GetLastLBA() != UINT64_C(0))) {
         retval = 0;
      } // if partition is in use
   } else retval = 0;

   return retval;
} // GPTData::IsFreePartNum()

/********************************
 *                              *
 * Endianness support functions *
 *                              *
 ********************************/

void GPTData::ReverseHeaderBytes(struct GPTHeader* header) {
   ReverseBytes(&header->signature, 8);
   ReverseBytes(&header->revision, 4);
   ReverseBytes(&header->headerSize, 4);
   ReverseBytes(&header->headerCRC, 4);
   ReverseBytes(&header->reserved, 4);
   ReverseBytes(&header->currentLBA, 8);
   ReverseBytes(&header->backupLBA, 8);
   ReverseBytes(&header->firstUsableLBA, 8);
   ReverseBytes(&header->lastUsableLBA, 8);
   ReverseBytes(&header->partitionEntriesLBA, 8);
   ReverseBytes(&header->numParts, 4);
   ReverseBytes(&header->sizeOfPartitionEntries, 4);
   ReverseBytes(&header->partitionEntriesCRC, 4);
   ReverseBytes(&header->reserved2, GPT_RESERVED);
//   header->diskGUID.ReverseGUIDBytes();
} // GPTData::ReverseHeaderBytes()

// IMPORTANT NOTE: This function requires non-reversed mainHeader
// structure!
void GPTData::ReversePartitionBytes() {
   uint32_t i;

   // Check GPT signature on big-endian systems; this will mismatch
   // if the function is called out of order. Unfortunately, it'll also
   // mismatch if there's data corruption.
   if ((mainHeader.signature != GPT_SIGNATURE) && (IsLittleEndian() == 0)) {
      cerr << "GPT signature mismatch in GPTData::ReversePartitionBytes(). This indicates\n"
           << "data corruption or a misplaced call to this function.\n";
   } // if signature mismatch....
   for (i = 0; i < mainHeader.numParts; i++) {
      partitions[i].ReversePartBytes();
   } // for
} // GPTData::ReversePartitionBytes()

/******************************************
 *                                        *
 * Additional non-class support functions *
 *                                        *
 ******************************************/

// Check to be sure that data type sizes are correct. The basic types (uint*_t) should
// never fail these tests, but the struct types may fail depending on compile options.
// Specifically, the -fpack-struct option to gcc may be required to ensure proper structure
// sizes.
int SizesOK(void) {
   int allOK = 1;

   if (sizeof(uint8_t) != 1) {
      cerr << "uint8_t is " << sizeof(uint8_t) << " bytes, should be 1 byte; aborting!\n";
      allOK = 0;
   } // if
   if (sizeof(uint16_t) != 2) {
      cerr << "uint16_t is " << sizeof(uint16_t) << " bytes, should be 2 bytes; aborting!\n";
      allOK = 0;
   } // if
   if (sizeof(uint32_t) != 4) {
      cerr << "uint32_t is " << sizeof(uint32_t) << " bytes, should be 4 bytes; aborting!\n";
      allOK = 0;
   } // if
   if (sizeof(uint64_t) != 8) {
      cerr << "uint64_t is " << sizeof(uint64_t) << " bytes, should be 8 bytes; aborting!\n";
      allOK = 0;
   } // if
   if (sizeof(struct MBRRecord) != 16) {
      cerr << "MBRRecord is " << sizeof(MBRRecord) << " bytes, should be 16 bytes; aborting!\n";
      allOK = 0;
   } // if
   if (sizeof(struct TempMBR) != 512) {
      cerr << "TempMBR is " <<  sizeof(TempMBR) << " bytes, should be 512 bytes; aborting!\n";
      allOK = 0;
   } // if
   if (sizeof(struct GPTHeader) != 512) {
      cerr << "GPTHeader is " << sizeof(GPTHeader) << " bytes, should be 512 bytes; aborting!\n";
      allOK = 0;
   } // if
   if (sizeof(GPTPart) != 128) {
      cerr << "GPTPart is " << sizeof(GPTPart) << " bytes, should be 128 bytes; aborting!\n";
      allOK = 0;
   } // if
   if (sizeof(GUIDData) != 16) {
      cerr << "GUIDData is " << sizeof(GUIDData) << " bytes, should be 16 bytes; aborting!\n";
      allOK = 0;
   } // if
   if (sizeof(PartType) != 16) {
      cerr << "PartType is " << sizeof(GUIDData) << " bytes, should be 16 bytes; aborting!\n";
      allOK = 0;
   } // if
   // Determine endianness; warn user if running on big-endian (PowerPC, etc.) hardware
   if (IsLittleEndian() == 0) {
      cerr << "\aRunning on big-endian hardware. Big-endian support is new and poorly"
            " tested!\n";
   } // if
   return (allOK);
} // SizesOK()

