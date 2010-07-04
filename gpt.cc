/* gpt.cc -- Functions for loading, saving, and manipulating legacy MBR and GPT partition
   data. */

/* By Rod Smith, initial coding January to February, 2009 */

/* This program is copyright (c) 2009 by Roderick W. Smith. It is distributed
  under the terms of the GNU GPL version 2, as detailed in the COPYING file. */

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
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
#include "partnotes.h"

using namespace std;

#ifdef __FreeBSD__
#define log2(x) (log(x) / M_LN2)
#endif // __FreeBSD__

#ifdef _MSC_VER
#define log2(x) (log((double) x) / log(2.0))
#endif // Microsoft Visual C++

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
   numParts = 0;
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
   numParts = 0;
   if (!LoadPartitions(filename))
      exit(2);
} // GPTData(string filename) constructor

// Destructor
GPTData::~GPTData(void) {
   delete[] partitions;
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

   // Check for insane partitions (start after end, hugely big, etc.)
   problems += FindInsanePartitions();

   // Check for mismatched MBR and GPT partitions...
   problems += FindHybridMismatches();

   // Verify that partitions don't run into GPT data areas....
   problems += CheckGPTSize();

   // Check that partitions are aligned on proper boundaries (for WD Advanced
   // Format and similar disks)....
   for (i = 0; i < numParts; i++) {
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
      cout << "No problems found. " << totalFree << " free sectors ("
           << BytesToSI(totalFree * (uint64_t) blockSize) << ") available in "
           << numSegments << "\nsegments, the largest of which is "
           << largestSegment << " (" << BytesToSI(largestSegment * (uint64_t) blockSize)
           << ") in size.\n";
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
   for (i = 0; i < numParts; i++) {
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
         cout << "\nWarning! Secondary partition table overlaps the last partition by\n"
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
// Note: Must be called with header in LITTLE-ENDIAN
// (x86, x86-64, etc.) byte order.
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
   uint32_t crc, hSize;
   int littleEndian = 1;

   // Initialize CRC functions...
   chksum_crc32gentab();

   // Save some key data from header before reversing byte order....
   hSize = mainHeader.headerSize;

   if ((littleEndian = IsLittleEndian()) == 0) {
      ReversePartitionBytes();
      ReverseHeaderBytes(&mainHeader);
      ReverseHeaderBytes(&secondHeader);
   } // if

   // Compute CRC of partition tables & store in main and secondary headers
   crc = chksum_crc32((unsigned char*) partitions, numParts * GPT_SIZE);
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
   SetGPTSize(mainHeader.numParts);
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
   SetGPTSize(secondHeader.numParts);
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
         } while ((!found) && (j < numParts));
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

   for (i = 1; i < numParts; i++) {
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

// Find partitions that are insane -- they start after they end or are too
// big for the disk. (The latter should duplicate detection of overlaps
// with GPT backup data structures, but better to err on the side of
// redundant tests than to miss something....)
int GPTData::FindInsanePartitions(void) {
   uint32_t i;
   int problems = 0;

   for (i = 0; i < numParts; i++) {
      if (partitions[i].GetFirstLBA() > partitions[i].GetLastLBA()) {
         problems++;
         cout << "\nProblem: partition " << i + 1 << " ends before it begins.\n";
      } // if
      if (partitions[i].GetLastLBA() >= diskSize) {
         problems++;
      cout << "\nProblem: partition " << i + 1<< " is too big for the disk.\n";
      } // if
   } // for
   return problems;
} // GPTData::FindInsanePartitions(void)


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
   BSDData bsdDisklabel;
   int err, allOK = 1;
   MBRValidity mbrState;

   if (myDisk.OpenForRead(deviceFilename)) {
      err = myDisk.OpenForWrite(deviceFilename);
      if ((err == 0) && (!justLooking)) {
         cout << "\aNOTE: Write test failed with error number " << errno
              << ". It will be impossible to save\nchanges to this disk's partition table!\n";
#if defined (__FreeBSD__) || defined (__FreeBSD_kernel__)
         cout << "You may be able to enable writes by exiting this program, typing\n"
              << "'sysctl kern.geom.debugflags=16' at a shell prompt, and re-running this\n"
              << "program.\n";
#endif
         cout << "\n";
      } // if
      myDisk.Close(); // Close and re-open read-only in case of bugs
   } else allOK = 0; // if

   if (allOK && myDisk.OpenForRead(deviceFilename)) {
      // store disk information....
      diskSize = myDisk.DiskSize(&err);
      blockSize = (uint32_t) myDisk.GetBlockSize();
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
            XFormDisklabel(&bsdDisklabel);
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

      if (allOK)
         CheckGPTSize();
      myDisk.Close();
      ComputeAlignment();
   } else {
      allOK = 0;
   } // if/else
   return (allOK);
} // GPTData::LoadPartitions()

// Loads the GPT, as much as possible. Returns 1 if this seems to have
// succeeded, 0 if there are obvious problems....
int GPTData::ForceLoadGPTData(void) {
   int allOK, validHeaders, loadedTable = 1;

   allOK = LoadHeader(&mainHeader, myDisk, 1, &mainCrcOk);

   if (mainCrcOk && (mainHeader.backupLBA < diskSize)) {
      allOK = LoadHeader(&secondHeader, myDisk, mainHeader.backupLBA, &secondCrcOk) && allOK;
   } else {
      allOK = LoadHeader(&secondHeader, myDisk, diskSize - UINT64_C(1), &secondCrcOk) && allOK;
      if (mainCrcOk && (mainHeader.backupLBA >= diskSize))
         cout << "Warning! Disk size is smaller than the main header indicates! Loading\n"
              << "secondary header from the last sector of the disk! You should use 'v' to\n"
              << "verify disk integrity, and perhaps options on the experts' menu to repair\n"
              << "the disk.\n";
   } // if/else
   if (!allOK)
      state = gpt_invalid;

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
            loadedTable = 2;
            cerr << "\aWarning: Invalid CRC on main header data; loaded backup partition table.\n";
         } else { // backup table bad, bad main header CRC, but try main table in desperation....
            if (LoadMainTable() == 0) {
               allOK = 0;
               loadedTable = 0;
               cerr << "\a\aWarning! Unable to load either main or backup partition table!\n";
            } // if
         } // if/else (LoadSecondTableAsMain())
      } // if/else (load partition table)

      if (loadedTable == 1)
         secondPartsCrcOk = CheckTable(&secondHeader);
      else if (loadedTable == 2)
         mainPartsCrcOk = CheckTable(&mainHeader);
      else
         mainPartsCrcOk = secondPartsCrcOk = 0;

      // Problem with main partition table; if backup is OK, use it instead....
      if (secondPartsCrcOk && secondCrcOk && !mainPartsCrcOk) {
         state = gpt_corrupt;
         allOK = allOK && LoadSecondTableAsMain();
         mainPartsCrcOk = 0; // LoadSecondTableAsMain() resets this, so re-flag as bad
         cerr << "\aWarning! Main partition table CRC mismatch! Loaded backup "
              << "partition table\ninstead of main partition table!\n\n";
      } // if */

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
   return LoadPartitionTable(mainHeader, myDisk);
} // GPTData::LoadMainTable()

// Load the second (backup) partition table as the primary partition
// table. Used in repair functions, and when starting up if the main
// partition table is damaged.
// Returns 1 on success, 0 on failure. CRC errors do NOT count as failure.
int GPTData::LoadSecondTableAsMain(void) {
   return LoadPartitionTable(secondHeader, myDisk);
} // GPTData::LoadSecondTableAsMain()

// Load a single GPT header (main or backup) from the specified disk device and
// sector. Applies byte-order corrections on big-endian platforms. Sets crcOk
// value appropriately.
// Returns 1 on success, 0 on failure. Note that CRC errors do NOT qualify as
// failure.
int GPTData::LoadHeader(struct GPTHeader *header, DiskIO & disk, uint64_t sector, int *crcOk) {
   int allOK = 1;
   GPTHeader tempHeader;

   disk.Seek(sector);
   if (disk.Read(&tempHeader, 512) != 512) {
      cerr << "Warning! Read error " << errno << "; strange behavior now likely!\n";
      allOK = 0;
   } // if
   *crcOk = CheckHeaderCRC(&tempHeader);

   // Reverse byte order, if necessary
   if (IsLittleEndian() == 0) {
      ReverseHeaderBytes(&tempHeader);
   } // if

   if (allOK && (numParts != tempHeader.numParts) && *crcOk) {
      allOK = SetGPTSize(tempHeader.numParts);
   }

   *header = tempHeader;
   return allOK;
} // GPTData::LoadHeader

// Load a partition table (either main or secondary) from the specified disk,
// using header as a reference for what to load. If sector != 0 (the default
// is 0), loads from the specified sector; otherwise loads from the sector
// indicated in header.
// Returns 1 on success, 0 on failure. CRC errors do NOT count as failure.
int GPTData::LoadPartitionTable(const struct GPTHeader & header, DiskIO & disk, uint64_t sector) {
   uint32_t sizeOfParts, newCRC;
   int retval;

   if (disk.OpenForRead()) {
      if (sector == 0) {
         retval = disk.Seek(header.partitionEntriesLBA);
      } else {
         retval = disk.Seek(sector);
      } // if/else
      if (retval == 1)
         retval = SetGPTSize(header.numParts);
      if (retval == 1) {
         sizeOfParts = header.numParts * header.sizeOfPartitionEntries;
         if (disk.Read(partitions, sizeOfParts) != (int) sizeOfParts) {
            cerr << "Warning! Read error " << errno << "! Misbehavior now likely!\n";
            retval = 0;
         } // if
         newCRC = chksum_crc32((unsigned char*) partitions, sizeOfParts);
         mainPartsCrcOk = secondPartsCrcOk = (newCRC == header.partitionEntriesCRC);
         if (IsLittleEndian() == 0)
            ReversePartitionBytes();
         if (!mainPartsCrcOk) {
            cout << "Caution! After loading partitions, the CRC doesn't check out!\n";
         } // if
      } else {
         cerr << "Error! Couldn't seek to partition table!\n";
      } // if/else
   } else {
      cerr << "Error! Couldn't open device " << device
           << " when reading partition table!\n";
      retval = 0;
   } // if/else
   return retval;
} // GPTData::LoadPartitionsTable()

// Check the partition table pointed to by header, but don't keep it
// around.
// Returns 1 if the CRC is OK, 0 if not or if there was a read error.
int GPTData::CheckTable(struct GPTHeader *header) {
   uint32_t sizeOfParts, newCRC;
   uint8_t *storage;
   int newCrcOk = 0;

   // Load partition table into temporary storage to check
   // its CRC and store the results, then discard this temporary
   // storage, since we don't use it in any but recovery operations
   if (myDisk.Seek(header->partitionEntriesLBA)) {
      sizeOfParts = header->numParts * header->sizeOfPartitionEntries;
      storage = new uint8_t[sizeOfParts];
      if (myDisk.Read(storage, sizeOfParts) != (int) sizeOfParts) {
         cerr << "Warning! Error " << errno << " reading partition table for CRC check!\n";
      } else {
         newCRC = chksum_crc32((unsigned char*) storage,  sizeOfParts);
         newCrcOk = (newCRC == header->partitionEntriesCRC);
      } // if/else
      delete[] storage;
   } // if
   return newCrcOk;
} // GPTData::CheckTable()

// Writes GPT (and protective MBR) to disk. Returns 1 on successful
// write, 0 if there was a problem.
int GPTData::SaveGPTData(int quiet) {
   int allOK = 1, littleEndian;
   char answer;

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

   // Check for overlapping or insane partitions....
   if ((FindOverlaps() > 0) || (FindInsanePartitions() > 0)) {
      allOK = 0;
      cerr << "Aborting write operation!\n";
   } // if

   // Check for mismatched MBR and GPT data, but let it pass if found
   // (function displays warning message)
   FindHybridMismatches();

   RecomputeCRCs();

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
      if (myDisk.OpenForWrite(device)) {
         // As per UEFI specs, write the secondary table and GPT first....
         allOK = SavePartitionTable(myDisk, secondHeader.partitionEntriesLBA);
         if (!allOK)
            cerr << "Unable to save backup partition table! Perhaps the 'e' option on the experts'\n"
                 << "menu will resolve this problem.\n";

         // Now write the secondary GPT header...
         allOK = allOK && SaveHeader(&secondHeader, myDisk, mainHeader.backupLBA);

         // Now write the main partition tables...
         allOK = allOK && SavePartitionTable(myDisk, mainHeader.partitionEntriesLBA);

         // Now write the main GPT header...
         allOK = allOK && SaveHeader(&mainHeader, myDisk, 1);

         // To top it off, write the protective MBR...
         allOK = allOK && protectiveMBR.WriteMBRData(&myDisk);

         // re-read the partition table
         if (allOK) {
            myDisk.DiskSync();
         } // if

         if (allOK) { // writes completed OK
            cout << "The operation has completed successfully.\n";
         } else {
            cerr << "Warning! An error was reported when writing the partition table! This error\n"
                 << "MIGHT be harmless, but you may have trashed the disk!\n";
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
   DiskIO backupFile;

   if (backupFile.OpenForWrite(filename)) {
      // Recomputing the CRCs is likely to alter them, which could be bad
      // if the intent is to save a potentially bad GPT for later analysis;
      // but if we don't do this, we get bogus errors when we load the
      // backup. I'm favoring misses over false alarms....
      RecomputeCRCs();

      protectiveMBR.WriteMBRData(&backupFile);

      if (allOK) {
         // MBR write closed disk, so re-open and seek to end....
         backupFile.OpenForWrite();
         allOK = SaveHeader(&mainHeader, backupFile, 1);
      } // if (allOK)

      if (allOK)
         allOK = SaveHeader(&secondHeader, backupFile, 2);

      if (allOK)
         allOK = SavePartitionTable(backupFile, 3);

      if (allOK) { // writes completed OK
         cout << "The operation has completed successfully.\n";
      } else {
         cerr << "Warning! An error was reported when writing the backup file.\n"
              << "It may not be usable!\n";
      } // if/else
      backupFile.Close();
   } else {
      cerr << "Unable to open file " << filename << " for writing! Aborting!\n";
      allOK = 0;
   } // if/else
   return allOK;
} // GPTData::SaveGPTBackup()

// Write a GPT header (main or backup) to the specified sector. Used by both
// the SaveGPTData() and SaveGPTBackup() functions.
// Should be passed an architecture-appropriate header (DO NOT call
// ReverseHeaderBytes() on the header before calling this function)
// Returns 1 on success, 0 on failure
int GPTData::SaveHeader(struct GPTHeader *header, DiskIO & disk, uint64_t sector) {
   int littleEndian, allOK = 1;

   littleEndian = IsLittleEndian();
   if (!littleEndian)
      ReverseHeaderBytes(header);
   if (disk.Seek(sector)) {
      if (disk.Write(header, 512) == -1)
         allOK = 0;
   } else allOK = 0; // if (disk.Seek()...)
   if (!littleEndian)
      ReverseHeaderBytes(header);
   return allOK;
} // GPTData::SaveHeader()

// Save the partitions to the specified sector. Used by both the SaveGPTData()
// and SaveGPTBackup() functions.
// Should be passed an architecture-appropriate header (DO NOT call
// ReverseHeaderBytes() on the header before calling this function)
// Returns 1 on success, 0 on failure
int GPTData::SavePartitionTable(DiskIO & disk, uint64_t sector) {
   int littleEndian, allOK = 1;

   littleEndian = IsLittleEndian();
   if (disk.Seek(sector)) {
      if (!littleEndian)
         ReversePartitionBytes();
      if (disk.Write(partitions, mainHeader.sizeOfPartitionEntries * numParts) == -1)
         allOK = 0;
      if (!littleEndian)
         ReversePartitionBytes();
   } else allOK = 0; // if (myDisk.Seek()...)
   return allOK;
} // GPTData::SavePartitionTable()

// Load GPT data from a backup file created by SaveGPTBackup(). This function
// does minimal error checking. It returns 1 if it completed successfully,
// 0 if there was a problem. In the latter case, it creates a new empty
// set of partitions.
int GPTData::LoadGPTBackup(const string & filename) {
   int allOK = 1, val, err;
   uint32_t sizeOfEntries;
   int littleEndian = 1, shortBackup = 0;
   DiskIO backupFile;

   if (backupFile.OpenForRead(filename)) {
      if (IsLittleEndian() == 0)
         littleEndian = 0;

      // Let the MBRData class load the saved MBR...
      protectiveMBR.ReadMBRData(&backupFile, 0); // 0 = don't check block size

      LoadHeader(&mainHeader, backupFile, 1, &mainCrcOk);

      // Check backup file size and rebuild second header if file is right
      // size to be direct dd copy of MBR, main header, and main partition
      // table; if other size, treat it like a GPT fdisk-generated backup
      // file
      shortBackup = ((backupFile.DiskSize(&err) * backupFile.GetBlockSize()) ==
                     (mainHeader.numParts * mainHeader.sizeOfPartitionEntries) + 1024);
      if (shortBackup) {
         RebuildSecondHeader();
         secondCrcOk = mainCrcOk;
      } else {
         LoadHeader(&secondHeader, backupFile, 2, &secondCrcOk);
      } // if/else

      // Return valid headers code: 0 = both headers bad; 1 = main header
      // good, backup bad; 2 = backup header good, main header bad;
      // 3 = both headers good. Note these codes refer to valid GPT
      // signatures and version numbers; more subtle problems will elude
      // this check!
      if ((val = CheckHeaderValidity()) > 0) {
         if (val == 2) { // only backup header seems to be good
            SetGPTSize(secondHeader.numParts);
//            numParts = secondHeader.numParts;
            sizeOfEntries = secondHeader.sizeOfPartitionEntries;
         } else { // main header is OK
            SetGPTSize(mainHeader.numParts);
//            numParts = mainHeader.numParts;
            sizeOfEntries = mainHeader.sizeOfPartitionEntries;
         } // if/else

//         SetGPTSize(numParts);

         if (secondHeader.currentLBA != diskSize - UINT64_C(1)) {
            cout << "Warning! Current disk size doesn't match that of the backup!\n"
                 << "Adjusting sizes to match, but subsequent problems are possible!\n";
            MoveSecondHeaderToEnd();
         } // if

         if (!LoadPartitionTable(mainHeader, backupFile, (uint64_t) (3 - shortBackup)))
            cerr << "Warning! Read error " << errno
                 << " loading partition table; strange behavior now likely!\n";
      } else {
         allOK = 0;
      } // if/else
      // Something went badly wrong, so blank out partitions
      if (allOK == 0) {
         cerr << "Improper backup file! Clearing all partition data!\n";
         ClearGPTData();
         protectiveMBR.MakeProtectiveMBR();
      } // if
   } else {
      allOK = 0;
      cerr << "Unable to open file " << filename << " for reading! Aborting!\n";
   } // if/else

   return allOK;
} // GPTData::LoadGPTBackup()

int GPTData::SaveMBR(void) {
   return protectiveMBR.WriteMBRData(&myDisk);
} // GPTData::SaveMBR()

// This function destroys the on-disk GPT structures, but NOT the on-disk
// MBR.
// Returns 1 if the operation succeeds, 0 if not.
int GPTData::DestroyGPT(void) {
   int i, sum, tableSize, allOK = 1;
   uint8_t blankSector[512];
   uint8_t* emptyTable;

   for (i = 0; i < 512; i++) {
      blankSector[i] = 0;
   } // for

   if (myDisk.OpenForWrite()) {
      if (!myDisk.Seek(mainHeader.currentLBA))
         allOK = 0;
      if (myDisk.Write(blankSector, 512) != 512) { // blank it out
         cerr << "Warning! GPT main header not overwritten! Error is " << errno << "\n";
         allOK = 0;
      } // if
      if (!myDisk.Seek(mainHeader.partitionEntriesLBA))
         allOK = 0;
      tableSize = numParts * mainHeader.sizeOfPartitionEntries;
      emptyTable = new uint8_t[tableSize];
      for (i = 0; i < tableSize; i++)
         emptyTable[i] = 0;
      if (allOK) {
         sum = myDisk.Write(emptyTable, tableSize);
         if (sum != tableSize) {
            cerr << "Warning! GPT main partition table not overwritten! Error is " << errno << "\n";
            allOK = 0;
         } // if write failed
      } // if 
      if (!myDisk.Seek(secondHeader.partitionEntriesLBA))
         allOK = 0;
      if (allOK) {
         sum = myDisk.Write(emptyTable, tableSize);
         if (sum != tableSize) {
            cerr << "Warning! GPT backup partition table not overwritten! Error is "
                 << errno << "\n";
            allOK = 0;
         } // if wrong size written
      } // if
      if (!myDisk.Seek(secondHeader.currentLBA))
         allOK = 0;
      if (allOK) {
         if (myDisk.Write(blankSector, 512) != 512) { // blank it out
            cerr << "Warning! GPT backup header not overwritten! Error is " << errno << "\n";
            allOK = 0;
         } // if
      } // if
      myDisk.DiskSync();
      myDisk.Close();
      cout << "GPT data structures destroyed! You may now partition the disk using fdisk or\n"
           << "other utilities.\n";
      delete[] emptyTable;
   } else {
      cerr << "Problem opening " << device << " for writing! Program will now terminate.\n";
   } // if/else (fd != -1)
   return (allOK);
} // GPTDataTextUI::DestroyGPT()

// Wipe MBR data from the disk (zero it out completely)
// Returns 1 on success, 0 on failure.
int GPTData::DestroyMBR(void) {
   int allOK = 1, i;
   uint8_t blankSector[512];

   for (i = 0; i < 512; i++)
      blankSector[i] = 0;

   if (myDisk.OpenForWrite()) {
      if (myDisk.Seek(0)) {
         if (myDisk.Write(blankSector, 512) != 512)
            allOK = 0;
      } else allOK = 0;
   } else allOK = 0;
   if (!allOK)
      cerr << "Warning! MBR not overwritten! Error is " << errno << "!\n";
   return allOK;
} // GPTData::DestroyMBR(void)

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
   cout << "Partition table holds up to " << numParts << " entries\n";
   cout << "First usable sector is " << mainHeader.firstUsableLBA
        << ", last usable sector is " << mainHeader.lastUsableLBA << "\n";
   totalFree = FindFreeBlocks(&i, &temp);
   cout << "Partitions will be aligned on " << sectorAlignment << "-sector boundaries\n";
   cout << "Total free space is " << totalFree << " sectors ("
        << BytesToSI(totalFree * (uint64_t) blockSize) << ")\n";
   cout << "\nNumber  Start (sector)    End (sector)  Size       Code  Name\n";
   for (i = 0; i < numParts; i++) {
      partitions[i].ShowSummary(i, blockSize);
   } // for
} // GPTData::DisplayGPTData()

// Show detailed information on the specified partition
void GPTData::ShowPartDetails(uint32_t partNum) {
   if (partitions[partNum].GetFirstLBA() != 0) {
      partitions[partNum].ShowDetails(blockSize);
   } else {
      cout << "Partition #" << partNum + 1 << " does not exist.";
   } // if
} // GPTData::ShowPartDetails()

/**************************************************************************
 *                                                                        *
 * Partition table transformation functions (MBR or BSD disklabel to GPT) *
 * (some of these functions may require user interaction)                 *
 *                                                                        *
 **************************************************************************/

// Examines the MBR & GPT data to determine which set of data to use: the
// MBR (use_mbr), the GPT (use_gpt), the BSD disklabel (use_bsd), or create
// a new set of partitions (use_new). A return value of use_abort indicates
// that this function couldn't determine what to do. Overriding functions
// in derived classes may ask users questions in such cases.
WhichToUse GPTData::UseWhichPartitions(void) {
   WhichToUse which = use_new;
   MBRValidity mbrState;

   mbrState = protectiveMBR.GetValidity();

   if ((state == gpt_invalid) && ((mbrState == mbr) || (mbrState == hybrid))) {
      cout << "\n***************************************************************\n"
           << "Found invalid GPT and valid MBR; converting MBR to GPT format.\n";
      if (!justLooking) {
         cout << "\aTHIS OPERATION IS POTENTIALLY DESTRUCTIVE! Exit by typing 'q' if\n"
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
      cout << "\a THIS OPERATION IS POTENTIALLY DESTRUCTIVE! Your first\n"
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
      which = use_abort;
   } // if

   if (state == gpt_corrupt) {
      if (mbrState == gpt) {
         cout << "\a\a****************************************************************************\n"
              << "Caution: Found protective or hybrid MBR and corrupt GPT. Using GPT, but disk\n"
              << "verification and recovery are STRONGLY recommended.\n"
              << "****************************************************************************\n";
         which = use_gpt;
      } else {
         which = use_abort;
      } // if/else MBR says disk is GPT
   } // if GPT corrupt

   if (which == use_new)
      cout << "Creating new GPT entries.\n";

   return which;
} // UseWhichPartitions()

// Convert MBR partition table into GPT form.
void GPTData::XFormPartitions(void) {
   int i, numToConvert;
   uint8_t origType;

   // Clear out old data & prepare basics....
   ClearGPTData();

   // Convert the smaller of the # of GPT or MBR partitions
   if (numParts > MAX_MBR_PARTS)
      numToConvert = MAX_MBR_PARTS;
   else
      numToConvert = numParts;

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
} // GPTData::XFormPartitions()

// Transforms BSD disklabel on the specified partition (numbered from 0).
// If an invalid partition number is given, the program does nothing.
// Returns the number of new partitions created.
int GPTData::XFormDisklabel(uint32_t partNum) {
   uint32_t low, high;
   int goOn = 1, numDone = 0;
   BSDData disklabel;

   if (GetPartRange(&low, &high) == 0) {
      goOn = 0;
      cout << "No partitions!\n";
   } // if
   if (partNum > high) {
      goOn = 0;
      cout << "Specified partition is invalid!\n";
   } // if

   // If all is OK, read the disklabel and convert it.
   if (goOn) {
      goOn = disklabel.ReadBSDData(&myDisk, partitions[partNum].GetFirstLBA(),
                                   partitions[partNum].GetLastLBA());
      if ((goOn) && (disklabel.IsDisklabel())) {
         numDone = XFormDisklabel(&disklabel);
         if (numDone == 1)
            cout << "Converted 1 BSD partition.\n";
         else
            cout << "Converted " << numDone << " BSD partitions.\n";
      } else {
         cout << "Unable to convert partitions! Unrecognized BSD disklabel.\n";
      } // if/else
   } // if
   if (numDone > 0) { // converted partitions; delete carrier
      partitions[partNum].BlankPartition();
   } // if
   return numDone;
} // GPTData::XFormDisklabel(uint32_t i)

// Transform the partitions on an already-loaded BSD disklabel...
int GPTData::XFormDisklabel(BSDData* disklabel) {
   int i, partNum = 0, numDone = 0;

   if (disklabel->IsDisklabel()) {
      for (i = 0; i < disklabel->GetNumParts(); i++) {
         partNum = FindFirstFreePart();
         if (partNum >= 0) {
            partitions[partNum] = disklabel->AsGPT(i);
            if (partitions[partNum].IsUsed())
               numDone++;
         } // if
      } // for
      if (partNum == -1)
         cerr << "Warning! Too many partitions to convert!\n";
   } // if

   // Record that all original CRCs were OK so as not to raise flags
   // when doing a disk verification
   mainCrcOk = secondCrcOk = mainPartsCrcOk = secondPartsCrcOk = 1;

   return numDone;
} // GPTData::XFormDisklabel(BSDData* disklabel)

// Add one GPT partition to MBR. Used by PartsToMBR() functions. Created
// partition has the active/bootable flag UNset and uses the GPT fdisk
// type code divided by 0x0100 as the MBR type code.
// Returns 1 if operation was 100% successful, 0 if there were ANY
// problems.
int GPTData::OnePartToMBR(uint32_t gptPart, int mbrPart) {
   int allOK = 1;

   if ((mbrPart < 0) || (mbrPart > 3)) {
      cout << "MBR partition " << mbrPart + 1 << " is out of range; omitting it.\n";
      allOK = 0;
   } // if
   if (gptPart >= numParts) {
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
      } // if
      protectiveMBR.MakePart(mbrPart, (uint32_t) partitions[gptPart].GetFirstLBA(),
                             (uint32_t) partitions[gptPart].GetLengthLBA(),
                             partitions[gptPart].GetHexType() / 256, 0);
   } else { // partition out of range
      if (allOK) // Display only if "else" triggered by out-of-bounds condition
         cout << "Partition " << gptPart + 1 << " begins beyond the 32-bit pointer limit of MBR "
              << "partitions, or is\n too big; omitting it.\n";
      allOK = 0;
   } // if/else
   return allOK;
} // GPTData::OnePartToMBR()

// Convert partitions to MBR form (primary and logical) and return
// the number done. Partitions are specified in a PartNotes variable,
// which includes pointers to GPT partition numbers. A partition number
// of MBR_EFI_GPT means to place an EFI GPT protective partition in that
// location in the table, and MBR_EMPTY means not to create a partition
// in that table position. If the partition type entry for a partition
// is 0, a default entry is used, based on the GPT partition type code.
// Returns the number of partitions converted, NOT counting EFI GPT
// protective partitions or extended partitions.
int GPTData::PartsToMBR(PartNotes & notes) {
   int mbrNum = 0, numConverted = 0;
   struct PartInfo convInfo;

   protectiveMBR.EmptyMBR();
   protectiveMBR.SetDiskSize(diskSize);
   notes.Rewind();
   while (notes.GetNextInfo(&convInfo) >= 0) {
      if ((convInfo.gptPartNum >= 0) && (convInfo.type == PRIMARY)) {
         numConverted += OnePartToMBR((uint32_t) convInfo.gptPartNum, mbrNum);
         if (convInfo.hexCode != 0)
            protectiveMBR.SetPartType(mbrNum, convInfo.hexCode);
         if (convInfo.active)
            protectiveMBR.SetPartBootable(mbrNum);
         mbrNum++;
      } // if
      if (convInfo.gptPartNum == MBR_EFI_GPT)
         mbrNum++;
   } // for
   // Now go through and set sizes for MBR_EFI_GPT partitions....
   notes.Rewind();
   mbrNum = 0;
   while (notes.GetNextInfo(&convInfo) >= 0) {
      if ((convInfo.gptPartNum >= 0) && (convInfo.type == PRIMARY))
         mbrNum++;
      if (convInfo.gptPartNum == MBR_EFI_GPT) {
         if (protectiveMBR.FindFirstAvailable() == UINT32_C(1)) {
            protectiveMBR.MakePart(mbrNum, 1, protectiveMBR.FindLastInFree(1), convInfo.hexCode);
            protectiveMBR.SetHybrid();
         } else {
            protectiveMBR.MakeBiggestPart(mbrNum, convInfo.hexCode);
         } // if/else
         mbrNum++;
      } // if
   } // while
   // Now do logical partition(s)...
   protectiveMBR.SetDisk(&myDisk);
   numConverted += protectiveMBR.CreateLogicals(notes);
   return numConverted;
} // GPTData::PartsToMBR()


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
   GPTPart* newParts;
   GPTPart* trash;
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
   // efficient, this prevents mucking with the location of the secondary
   // partition table, which causes problems when loading data from a RAID
   // array that's been expanded because this function is called when loading
   // data.
   if (((numEntries != numParts) || (partitions == NULL)) && (numEntries > 0)) {
      newParts = new GPTPart [numEntries * sizeof (GPTPart)];
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
               if (numEntries < numParts)
                  copyNum = numEntries;
               else
                  copyNum = numParts;
               for (i = 0; i < copyNum; i++) {
                  newParts[i] = partitions[i];
               } // for
               trash = partitions;
               partitions = newParts;
               delete[] trash;
            } // if
         } else { // No existing partition table; just create it
            partitions = newParts;
         } // if/else existing partitions
         numParts = numEntries;
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
   mainHeader.numParts = numParts;
   secondHeader.numParts = numParts;
   return (allOK);
} // GPTData::SetGPTSize()

// Blank the partition array
void GPTData::BlankPartitions(void) {
   uint32_t i;

   for (i = 0; i < numParts; i++) {
      partitions[i].BlankPartition();
   } // for
} // GPTData::BlankPartitions()

// Delete a partition by number. Returns 1 if successful,
// 0 if there was a problem. Returns 1 if partition was in
// range, 0 if it was out of range.
int GPTData::DeletePartition(uint32_t partNum) {
   uint64_t startSector, length;
   uint32_t low, high, numUsedParts, retval = 1;;

   numUsedParts = GetPartRange(&low, &high);
   if ((numUsedParts > 0) && (partNum >= low) && (partNum <= high)) {
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

// Non-interactively create a partition.
// Returns 1 if the operation was successful, 0 if a problem was discovered.
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
   uint32_t i, numFound, firstPart, lastPart;

   // First, find the last partition with data, so as not to
   // spend needless time sorting empty entries....
   numFound = GetPartRange(&firstPart, &lastPart);

   // Now swap empties with the last partitions, to simplify the logic
   // in the Quicksort function....
   i = 0;
   while (i < lastPart) {
      if (partitions[i].GetFirstLBA() == 0) {
	     SwapPartitions(i, lastPart);
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
   QuickSortGPT(0, lastPart);
} // GPTData::SortGPT()

// Recursive quick sort algorithm for GPT partitions. Note that if there
// are any empties in the specified range, they'll be sorted to the
// start, resulting in a sorted set of partitions that begins with
// partition 2, 3, or higher.
void GPTData::QuickSortGPT(int start, int finish) {
   uint64_t starterValue; // starting location of median partition
   int left, right;

   left = start;
   right = finish;
   starterValue = partitions[(start + finish) / 2].GetFirstLBA();
   do {
      while (partitions[left].GetFirstLBA() < starterValue)
         left++;
      while (partitions[right].GetFirstLBA() > starterValue)
         right--;
      if (left <= right)
	     SwapPartitions(left++, right--);
   } while (left <= right);
   if (start < right) QuickSortGPT(start, right);
   if (finish > left) QuickSortGPT(left, finish);
} // GPTData::QuickSortGPT()

// Swap the contents of two partitions.
// Returns 1 if successful, 0 if either partition is out of range
// (that is, not a legal number; either or both can be empty).
// Note that if partNum1 = partNum2 and this number is in range,
// it will be considered successful.
int GPTData::SwapPartitions(uint32_t partNum1, uint32_t partNum2) {
   GPTPart temp;
   int allOK = 1;

   if ((partNum1 < numParts) && (partNum2 < numParts)) {
      if (partNum1 != partNum2) {
         temp = partitions[partNum1];
         partitions[partNum1] = partitions[partNum2];
         partitions[partNum2] = temp;
      } // if
   } else allOK = 0; // partition numbers are valid
   return allOK;
} // GPTData::SwapPartitions()

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
      delete[] partitions;
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
   sectorAlignment = DEFAULT_ALIGNMENT;

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

   if (pn < numParts) {
      if (partitions[pn].GetFirstLBA() != UINT64_C(0)) {
         partitions[pn].SetUniqueGUID(theGUID);
         retval = 1;
      } // if
   } // if
   return retval;
} // GPTData::SetPartitionGUID()

// Set new random GUIDs for the disk and all partitions. Intended to be used
// after disk cloning or similar operations that don't randomize the GUIDs.
void GPTData::RandomizeGUIDs(void) {
   uint32_t i;

   mainHeader.diskGUID.Randomize();
   secondHeader.diskGUID = mainHeader.diskGUID;
   for (i = 0; i < numParts; i++)
      if (partitions[i].IsUsed())
         partitions[i].RandomizeUniqueGUID();
} // GPTData::RandomizeGUIDs()

// Change partition type code non-interactively. Returns 1 if
// successful, 0 if not....
int GPTData::ChangePartType(uint32_t partNum, uint16_t hexCode) {
   int retval = 1;

   if (!IsFreePartNum(partNum)) {
      partitions[partNum].SetType(hexCode);
   } else retval = 0;
   return retval;
} // GPTData::ChangePartType()

// Recompute the CHS values of all the MBR partitions. Used to reset
// CHS values that some BIOSes require, despite the fact that the
// resulting CHS values violate the GPT standard.
void GPTData::RecomputeCHS(void) {
   int i;

   for (i = 0; i < 4; i++)
      protectiveMBR.RecomputeCHS(i);
} // GPTData::RecomputeCHS()

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
              << *sector << " in\norder to align on " << sectorAlignment
              << "-sector boundaries.\n";
         if (!beQuiet)
            cout << "Use 'l' on the experts' menu to adjust alignment\n";
      } else {
         cout << "Information: Sector not aligned on " << sectorAlignment
              << "-sector boundary and could not be moved.\n"
              << "If you're using a Western Digital Advanced Format or similar disk with\n"
              << "underlying 4096-byte sectors or certain types of RAID array, performance\n"
              << "may suffer.\n";
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

   *low = numParts + 1; // code for "not found"
   *high = 0;
   if (numParts > 0) { // only try if partition table exists...
      for (i = 0; i < numParts; i++) {
         if (partitions[i].GetFirstLBA() != UINT64_C(0)) { // it exists
            *high = i; // since we're counting up, set the high value
	        // Set the low value only if it's not yet found...
            if (*low == (numParts + 1)) *low = i;
               numFound++;
         } // if
      } // for
   } // if

   // Above will leave *low pointing to its "not found" value if no partitions
   // are defined, so reset to 0 if this is the case....
   if (*low == (numParts + 1))
      *low = 0;
   return numFound;
} // GPTData::GetPartRange()

// Returns the value of the first free partition, or -1 if none is
// unused.
int GPTData::FindFirstFreePart(void) {
   int i = 0;

   if (partitions != NULL) {
      while ((partitions[i].IsUsed()) && (i < (int) numParts))
         i++;
      if (i >= (int) numParts)
         i = -1;
   } else i = -1;
   return i;
} // GPTData::FindFirstFreePart()

// Returns the number of defined partitions.
uint32_t GPTData::CountParts(void) {
   uint32_t i, counted = 0;

   for (i = 0; i < numParts; i++) {
      if (partitions[i].IsUsed())
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
      for (i = 0; i < numParts; i++) {
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

// Find the last available block on the disk.
// Returns 0 if there are no available partitions
uint64_t GPTData::FindLastAvailable(void) {
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
      for (i = 0; i < numParts; i++) {
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
   for (i = 0; i < numParts; i++) {
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
   if (diskSize > 0) {
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
   } // if
   *numSegments = num;
   return totalFound;
} // GPTData::FindFreeBlocks()

// Returns 1 if sector is unallocated, 0 if it's allocated to a partition.
// If it's allocated, return the partition number to which it's allocated
// in partNum, if that variable is non-NULL. (A value of UINT32_MAX is
// returned in partNum if the sector is in use by basic GPT data structures.)
int GPTData::IsFree(uint64_t sector, uint32_t *partNum) {
   int isFree = 1;
   uint32_t i;

   for (i = 0; i < numParts; i++) {
      if ((sector >= partitions[i].GetFirstLBA()) &&
           (sector <= partitions[i].GetLastLBA())) {
         isFree = 0;
         if (partNum != NULL)
            *partNum = i;
      } // if
   } // for
   if ((sector < mainHeader.firstUsableLBA) ||
        (sector > mainHeader.lastUsableLBA)) {
      isFree = 0;
      if (partNum != NULL)
         *partNum = UINT32_MAX;
   } // if
   return (isFree);
} // GPTData::IsFree()

// Returns 1 if partNum is unused.
int GPTData::IsFreePartNum(uint32_t partNum) {
   int retval = 1;

   if ((partNum < numParts) && (partitions != NULL)) {
      if (partitions[partNum].IsUsed()) {
         retval = 0;
      } // if partition is in use
   } else retval = 0;

   return retval;
} // GPTData::IsFreePartNum()


/***********************************************************
 *                                                         *
 * Change how functions work or return information on them *
 *                                                         *
 ***********************************************************/

// Set partition alignment value; partitions will begin on multiples of
// the specified value
void GPTData::SetAlignment(uint32_t n) {
   sectorAlignment = n;
} // GPTData::SetAlignment()

// Compute sector alignment based on the current partitions (if any). Each
// partition's starting LBA is examined, and if it's divisible by a power-of-2
// value less than or equal to the DEFAULT_ALIGNMENT value, but not by the
// previously-located alignment value, then the alignment value is adjusted
// down. If the computed alignment is less than 8 and the disk is bigger than
// SMALLEST_ADVANCED_FORMAT, resets it to 8. This is a safety measure for WD
// Advanced Format and similar drives. If no partitions are defined, the
// alignment value is set to DEFAULT_ALIGNMENT (2048). The result is that new
// drives are aligned to 2048-sector multiples but the program won't complain
// about other alignments on existing disks unless a smaller-than-8 alignment
// is used on small disks (as safety for WD Advanced Format drives).
// Returns the computed alignment value.
uint32_t GPTData::ComputeAlignment(void) {
   uint32_t i = 0, found, exponent = 31;
   uint64_t align = DEFAULT_ALIGNMENT;

   exponent = (uint32_t) log2(DEFAULT_ALIGNMENT);
   for (i = 0; i < numParts; i++) {
      if (partitions[i].IsUsed()) {
         found = 0;
         while (!found) {
            align = PowerOf2(exponent);
            if ((partitions[i].GetFirstLBA() % align) == 0) {
               found = 1;
            } else {
               exponent--;
            } // if/else
         } // while
      } // if
   } // for
   if ((align < 8) && (diskSize >= SMALLEST_ADVANCED_FORMAT))
      align = 8;
   SetAlignment(align);
   return align;
} // GPTData::ComputeAlignment()

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
   ReverseBytes(header->reserved2, GPT_RESERVED);
} // GPTData::ReverseHeaderBytes()

// Reverse byte order for all partitions.
void GPTData::ReversePartitionBytes() {
   uint32_t i;

   for (i = 0; i < numParts; i++) {
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

