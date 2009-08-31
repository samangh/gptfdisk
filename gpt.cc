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
#include "crc32.h"
#include "gpt.h"
#include "bsd.h"
#include "support.h"
#include "parttypes.h"
#include "attributes.h"

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
   strcpy(device, "");
   mainCrcOk = 0;
   secondCrcOk = 0;
   mainPartsCrcOk = 0;
   secondPartsCrcOk = 0;
   apmFound = 0;
   bsdFound = 0;
   srand((unsigned int) time(NULL));
   SetGPTSize(NUM_GPT_ENTRIES);
} // GPTData default constructor

// The following constructor loads GPT data from a device file
GPTData::GPTData(char* filename) {
   blockSize = SECTOR_SIZE; // set a default
   diskSize = 0;
   partitions = NULL;
   state = gpt_invalid;
   strcpy(device, "");
   mainCrcOk = 0;
   secondCrcOk = 0;
   mainPartsCrcOk = 0;
   secondPartsCrcOk = 0;
   apmFound = 0;
   bsdFound = 0;
   srand((unsigned int) time(NULL));
   LoadPartitions(filename);
} // GPTData(char* filename) constructor

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
   int problems = 0, numSegments, i, j;
   uint64_t totalFree, largestSegment;
   char tempStr[255], siTotal[255], siLargest[255];

   // First, check for CRC errors in the GPT data....
   if (!mainCrcOk) {
      problems++;
      printf("\nProblem: The CRC for the main GPT header is invalid. The main GPT header may\n"
            "be corrupt. Consider loading the backup GPT header to rebuild the main GPT\n"
            "header\n");
   } // if
   if (!mainPartsCrcOk) {
      problems++;
      printf("\nProblem: The CRC for the main partition table is invalid. This table may be\n"
            "corrupt. Consider loading the backup partition table.\n");
   } // if
   if (!secondCrcOk) {
      problems++;
      printf("\nProblem: The CRC for the backup GPT header is invalid. The backup GPT header\n"
            "may be corrupt. Consider using the main GPT header to rebuild the backup GPT\n"
            "header.\n");
   } // if
   if (!secondPartsCrcOk) {
      problems++;
      printf("\nCaution: The CRC for the backup partition table is invalid. This table may\n"
            "be corrupt. This program will automatically create a new backup partition\n"
            "table when you save your partitions.\n");
   } // if

   // Now check that critical main and backup GPT entries match
   if (mainHeader.currentLBA != secondHeader.backupLBA) {
      problems++;
      printf("\nProblem: main GPT header's current LBA pointer (%llu) doesn't\n"
            "match the backup GPT header's LBA pointer(%llu)\n",
            (unsigned long long) mainHeader.currentLBA,
             (unsigned long long) secondHeader.backupLBA);
   } // if
   if (mainHeader.backupLBA != secondHeader.currentLBA) {
      problems++;
      printf("\nProblem: main GPT header's backup LBA pointer (%llu) doesn't\n"
            "match the backup GPT header's current LBA pointer (%llu)\n",
            (unsigned long long) mainHeader.backupLBA,
             (unsigned long long) secondHeader.currentLBA);
   } // if
   if (mainHeader.firstUsableLBA != secondHeader.firstUsableLBA) {
      problems++;
      printf("\nProblem: main GPT header's first usable LBA pointer (%llu) doesn't\n"
            "match the backup GPT header's first usable LBA pointer (%llu)\n",
            (unsigned long long) mainHeader.firstUsableLBA,
             (unsigned long long) secondHeader.firstUsableLBA);
   } // if
   if (mainHeader.lastUsableLBA != secondHeader.lastUsableLBA) {
      problems++;
      printf("\nProblem: main GPT header's last usable LBA pointer (%llu) doesn't\n"
            "match the backup GPT header's last usable LBA pointer (%llu)\n",
            (unsigned long long) mainHeader.lastUsableLBA,
             (unsigned long long) secondHeader.lastUsableLBA);
   } // if
   if ((mainHeader.diskGUID.data1 != secondHeader.diskGUID.data1) ||
        (mainHeader.diskGUID.data2 != secondHeader.diskGUID.data2)) {
      problems++;
      printf("\nProblem: main header's disk GUID (%s) doesn't\n",
             GUIDToStr(mainHeader.diskGUID, tempStr));
      printf("match the backup GPT header's disk GUID (%s)\n",
             GUIDToStr(secondHeader.diskGUID, tempStr));
   } // if
   if (mainHeader.numParts != secondHeader.numParts) {
      problems++;
      printf("\nProblem: main GPT header's number of partitions (%lu) doesn't\n"
            "match the backup GPT header's number of partitions (%lu)\n",
            (unsigned long) mainHeader.numParts,
            (unsigned long) secondHeader.numParts);
   } // if
   if (mainHeader.sizeOfPartitionEntries != secondHeader.sizeOfPartitionEntries) {
      problems++;
      printf("\nProblem: main GPT header's size of partition entries (%lu) doesn't\n"
            "match the backup GPT header's size of partition entries (%lu)\n",
            (unsigned long) mainHeader.sizeOfPartitionEntries,
            (unsigned long) secondHeader.sizeOfPartitionEntries);
   } // if

   // Now check for a few other miscellaneous problems...
   // Check that the disk size will hold the data...
   if (mainHeader.backupLBA > diskSize) {
      problems++;
      printf("\nProblem: Disk is too small to hold all the data!\n");
      printf("(Disk size is %llu sectors, needs to be %llu sectors.)\n",
            (unsigned long long) diskSize,
               (unsigned long long) mainHeader.backupLBA);
   } // if

   // Check for overlapping partitions....
   problems += FindOverlaps();

   // Check for mismatched MBR and GPT partitions...
   problems += FindHybridMismatches();

   // Verify that partitions don't run into GPT data areas....
   problems += CheckGPTSize();

   // Now compute available space, but only if no problems found, since
   // problems could affect the results
   if (problems == 0) {
      totalFree = FindFreeBlocks(&numSegments, &largestSegment);
      BytesToSI(totalFree * (uint64_t) blockSize, siTotal);
      BytesToSI(largestSegment * (uint64_t) blockSize, siLargest);
      printf("No problems found. %llu free sectors (%s) available in %u\n"
             "segments, the largest of which is %llu sectors (%s) in size\n",
             (unsigned long long) totalFree,
              siTotal, numSegments, (unsigned long long) largestSegment,
                                     siLargest);
   } else {
      printf("\nIdentified %d problems!\n", problems);
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
         printf("Warning! Main partition table overlaps the first partition by %lu blocks!\n",
                (unsigned long) overlap);
         if (firstUsedBlock > 2) {
            printf("Try reducing the partition table size by %lu entries.\n",
                   (unsigned long) (overlap * 4));
            printf("(Use the 's' item on the experts' menu.)\n");
         } else {
            printf("You will need to delete this partition or resize it in another utility.\n");
         } // if/else
         numProbs++;
      } // Problem at start of disk
      if (mainHeader.lastUsableLBA < lastUsedBlock) {
         overlap = lastUsedBlock - mainHeader.lastUsableLBA;
         printf("Warning! Secondary partition table overlaps the last partition by %lu blocks\n",
                (unsigned long) overlap);
         if (lastUsedBlock > (diskSize - 2)) {
            printf("You will need to delete this partition or resize it in another utility.\n");
         } else {
            printf("Try reducing the partition table size by %lu entries.\n",
                   (unsigned long) (overlap * 4));
            printf("(Use the 's' item on the experts' menu.)\n");
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

   if (mainHeader.signature != GPT_SIGNATURE) {
      valid -= 1;
//      printf("Main GPT signature invalid; read 0x%016llX, should be\n0x%016llX\n",
//             (unsigned long long) mainHeader.signature, (unsigned long long) GPT_SIGNATURE);
   } else if ((mainHeader.revision != 0x00010000) && valid) {
      valid -= 1;
      printf("Unsupported GPT version in main header; read 0x%08lX, should be\n0x%08lX\n",
             (unsigned long) mainHeader.revision, UINT32_C(0x00010000));
   } // if/else/if

   if (secondHeader.signature != GPT_SIGNATURE) {
      valid -= 2;
//      printf("Secondary GPT signature invalid; read 0x%016llX, should be\n0x%016llX\n",
//             (unsigned long long) secondHeader.signature, (unsigned long long) GPT_SIGNATURE);
   } else if ((secondHeader.revision != 0x00010000) && valid) {
      valid -= 2;
      printf("Unsupported GPT version in backup header; read 0x%08lX, should be\n0x%08lX\n",
             (unsigned long) mainHeader.revision, UINT32_C(0x00010000));
   } // if/else/if

   // If MBR bad, check for an Apple disk signature
   if ((protectiveMBR.GetValidity() == invalid) && 
        (((mainHeader.signature << 32) == APM_SIGNATURE1) ||
        (mainHeader.signature << 32) == APM_SIGNATURE2)) {
      apmFound = 1; // Will display warning message later
        } // if

        return valid;
} // GPTData::CheckHeaderValidity()

// Check the header CRC to see if it's OK...
// Note: Must be called BEFORE byte-order reversal on big-endian
// systems!
int GPTData::CheckHeaderCRC(struct GPTHeader* header) {
   uint32_t oldCRC, newCRC;

   // Back up old header CRC and then blank it, since it must be 0 for
   // computation to be valid
   oldCRC = header->headerCRC;
   if (IsLittleEndian() == 0)
      ReverseBytes(&oldCRC, 4);
   header->headerCRC = UINT32_C(0);

   // Initialize CRC functions...
   chksum_crc32gentab();

   // Compute CRC, restore original, and return result of comparison
   newCRC = chksum_crc32((unsigned char*) header, HEADER_SIZE);
   mainHeader.headerCRC = oldCRC;
   return (oldCRC == newCRC);
} // GPTData::CheckHeaderCRC()

// Recompute all the CRCs. Must be called before saving (but after reversing
// byte order on big-endian systems) if any changes have been made.
void GPTData::RecomputeCRCs(void) {
   uint32_t crc;
   uint32_t trueNumParts, crcTemp;
   int littleEndian = 1;

   // Initialize CRC functions...
   chksum_crc32gentab();

   littleEndian = IsLittleEndian();

   // Compute CRC of partition tables & store in main and secondary headers
   trueNumParts = mainHeader.numParts;
   if (littleEndian == 0)
      ReverseBytes(&trueNumParts, 4); // unreverse this key piece of data....
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
   crc = chksum_crc32((unsigned char*) &mainHeader, HEADER_SIZE);
   if (littleEndian == 0)
      ReverseBytes(&crc, 4);
   mainHeader.headerCRC = crc;
   crc = chksum_crc32((unsigned char*) &secondHeader, HEADER_SIZE);
   if (littleEndian == 0)
      ReverseBytes(&crc, 4);
   secondHeader.headerCRC = crc;
} // GPTData::RecomputeCRCs()

// Rebuild the main GPT header, using the secondary header as a model.
// Typically called when the main header has been found to be corrupt.
void GPTData::RebuildMainHeader(void) {
   int i;

   mainHeader.signature = GPT_SIGNATURE;
   mainHeader.revision = secondHeader.revision;
   mainHeader.headerSize = HEADER_SIZE;
   mainHeader.headerCRC = UINT32_C(0);
   mainHeader.reserved = secondHeader.reserved;
   mainHeader.currentLBA = secondHeader.backupLBA;
   mainHeader.backupLBA = secondHeader.currentLBA;
   mainHeader.firstUsableLBA = secondHeader.firstUsableLBA;
   mainHeader.lastUsableLBA = secondHeader.lastUsableLBA;
   mainHeader.diskGUID.data1 = secondHeader.diskGUID.data1;
   mainHeader.diskGUID.data2 = secondHeader.diskGUID.data2;
   mainHeader.partitionEntriesLBA = UINT64_C(2);
   mainHeader.numParts = secondHeader.numParts;
   mainHeader.sizeOfPartitionEntries = secondHeader.sizeOfPartitionEntries;
   mainHeader.partitionEntriesCRC = secondHeader.partitionEntriesCRC;
   for (i = 0 ; i < GPT_RESERVED; i++)
      mainHeader.reserved2[i] = secondHeader.reserved2[i];
} // GPTData::RebuildMainHeader()

// Rebuild the secondary GPT header, using the main header as a model.
void GPTData::RebuildSecondHeader(void) {
   int i;

   secondHeader.signature = GPT_SIGNATURE;
   secondHeader.revision = mainHeader.revision;
   secondHeader.headerSize = HEADER_SIZE;
   secondHeader.headerCRC = UINT32_C(0);
   secondHeader.reserved = mainHeader.reserved;
   secondHeader.currentLBA = mainHeader.backupLBA;
   secondHeader.backupLBA = mainHeader.currentLBA;
   secondHeader.firstUsableLBA = mainHeader.firstUsableLBA;
   secondHeader.lastUsableLBA = mainHeader.lastUsableLBA;
   secondHeader.diskGUID.data1 = mainHeader.diskGUID.data1;
   secondHeader.diskGUID.data2 = mainHeader.diskGUID.data2;
   secondHeader.partitionEntriesLBA = secondHeader.lastUsableLBA + UINT64_C(1);
   secondHeader.numParts = mainHeader.numParts;
   secondHeader.sizeOfPartitionEntries = mainHeader.sizeOfPartitionEntries;
   secondHeader.partitionEntriesCRC = mainHeader.partitionEntriesCRC;
   for (i = 0 ; i < GPT_RESERVED; i++)
      secondHeader.reserved2[i] = mainHeader.reserved2[i];
} // GPTData::RebuildSecondHeader()

// Search for hybrid MBR entries that have no corresponding GPT partition.
// Returns number of such mismatches found
int GPTData::FindHybridMismatches(void) {
   int i, j, found, numFound = 0;
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
            printf("\nWarning! Mismatched GPT and MBR partitions! MBR partition "
                   "%d, of type 0x%02X,\nhas no corresponding GPT partition! "
                   "You may continue, but this condition\nmight cause data loss"
                   " in the future!\a\n", i + 1, protectiveMBR.GetType(i));
         } // if
      } // if
   } // for
   return numFound;
} // GPTData::FindHybridMismatches

// Find overlapping partitions and warn user about them. Returns number of
// overlapping partitions.
int GPTData::FindOverlaps(void) {
   int i, j, problems = 0;

   for (i = 1; i < mainHeader.numParts; i++) {
      for (j = 0; j < i; j++) {
         if (partitions[i].DoTheyOverlap(&partitions[j])) {
            problems++;
            printf("\nProblem: partitions %d and %d overlap:\n", i + 1, j + 1);
            printf("  Partition %d: %llu to %llu\n", i,
                   (unsigned long long) partitions[i].GetFirstLBA(),
                    (unsigned long long) partitions[i].GetLastLBA());
            printf("  Partition %d: %llu to %llu\n", j,
                   (unsigned long long) partitions[j].GetFirstLBA(),
                    (unsigned long long) partitions[j].GetLastLBA());
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
void GPTData::PartitionScan(int fd) {
   BSDData bsdDisklabel;
//   int bsdFound;

   printf("Partition table scan:\n");

   // Read the MBR & check for BSD disklabel
   protectiveMBR.ReadMBRData(fd);
   protectiveMBR.ShowState();
   bsdDisklabel.ReadBSDData(fd, 0, diskSize - 1);
   bsdFound = bsdDisklabel.ShowState();
//   bsdDisklabel.DisplayBSDData();

   // Load the GPT data, whether or not it's valid
   ForceLoadGPTData(fd);
   ShowAPMState(); // Show whether there's an Apple Partition Map present
   ShowGPTState(); // Show GPT status
   printf("\n");

   if (apmFound) {
      printf("\n*******************************************************************\n");
      printf("This disk appears to contain an Apple-format (APM) partition table!\n");
      printf("It will be destroyed if you continue!\n");
      printf("*******************************************************************\n\n\a");
   } // if
/*   if (bsdFound) {
   printf("\n*************************************************************************\n");
   printf("This disk appears to contain a BSD disklabel! It will be destroyed if you\n"
   "continue!\n");
   printf("*************************************************************************\n\n\a");
} // if */
} // GPTData::PartitionScan()

// Read GPT data from a disk.
int GPTData::LoadPartitions(char* deviceFilename) {
   int fd, err;
   int allOK = 1, i;
   uint64_t firstBlock, lastBlock;
   BSDData bsdDisklabel;

   // First, do a test to see if writing will be possible later....
   fd = OpenForWrite(deviceFilename);
   if (fd == -1)
      printf("\aNOTE: Write test failed with error number %d. It will be "
             "impossible to save\nchanges to this disk's partition table!\n\n",
             errno);
   close(fd);

   if ((fd = open(deviceFilename, O_RDONLY)) != -1) {
      // store disk information....
      diskSize = disksize(fd, &err);
      blockSize = (uint32_t) GetBlockSize(fd);
      strcpy(device, deviceFilename);
      PartitionScan(fd); // Check for partition types & print summary

      switch (UseWhichPartitions()) {
         case use_mbr:
            XFormPartitions();
            break;
         case use_bsd:
            bsdDisklabel.ReadBSDData(fd, 0, diskSize - 1);
//            bsdDisklabel.DisplayBSDData();
            ClearGPTData();
            protectiveMBR.MakeProtectiveMBR(1); // clear boot area (option 1)
            XFormDisklabel(&bsdDisklabel, 0);
            break;
         case use_gpt:
            break;
         case use_new:
            ClearGPTData();
            protectiveMBR.MakeProtectiveMBR();
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
      } // if
      CheckGPTSize();
   } else {
      allOK = 0;
      fprintf(stderr, "Problem opening %s for reading! Error is %d\n",
              deviceFilename, errno);
      if (errno == EACCES) { // User is probably not running as root
         fprintf(stderr, "You must run this program as root or use sudo!\n");
      } // if
   } // if/else
   return (allOK);
} // GPTData::LoadPartitions()

// Loads the GPT, as much as possible. Returns 1 if this seems to have
// succeeded, 0 if there are obvious problems....
int GPTData::ForceLoadGPTData(int fd) {
   int allOK = 1, validHeaders;
   off_t seekTo;
   char* storage;
   uint32_t newCRC, sizeOfParts;

   // Seek to and read the main GPT header
   lseek64(fd, 512, SEEK_SET);
   read(fd, &mainHeader, 512); // read main GPT header
   mainCrcOk = CheckHeaderCRC(&mainHeader);
   if (IsLittleEndian() == 0) // big-endian system; adjust header byte order....
      ReverseHeaderBytes(&mainHeader);

   // Load backup header, check its CRC, and store the results of
   // the check for future reference
   seekTo = (diskSize * blockSize) - UINT64_C(512);
   if (lseek64(fd, seekTo, SEEK_SET) != (off_t) -1) {
      read(fd, &secondHeader, 512); // read secondary GPT header
      secondCrcOk = CheckHeaderCRC(&secondHeader);
      if (IsLittleEndian() == 0) // big-endian system; adjust header byte order....
         ReverseHeaderBytes(&secondHeader);
   } else {
      allOK = 0;
      state = gpt_invalid;
      fprintf(stderr, "Unable to seek to secondary GPT at sector %llu!\n",
              diskSize - (UINT64_C(1)));
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
      if (validHeaders == 2) { // valid backup header, invalid main header
         printf("Caution: invalid main GPT header, but valid backup; regenerating main header\n"
               "from backup!\n");
         RebuildMainHeader();
         mainCrcOk = secondCrcOk; // Since copied, use CRC validity of backup
      } else if (validHeaders == 1) { // valid main header, invalid backup
         printf("Caution: invalid backup GPT header, but valid main header; regenerating\n"
               "backup header from main header.\n");
         RebuildSecondHeader();
         secondCrcOk = mainCrcOk; // Since regenerated, use CRC validity of main
      } // if/else/if

      // Load the main partition table, including storing results of its
      // CRC check
      if (LoadMainTable() == 0)
         allOK = 0;

      // Load backup partition table into temporary storage to check
      // its CRC and store the results, then discard this temporary
      // storage, since we don't use it in any but recovery operations
      seekTo = secondHeader.partitionEntriesLBA * (off_t) blockSize;
      if ((lseek64(fd, seekTo, SEEK_SET) != (off_t) -1) && (secondCrcOk)) {
         sizeOfParts = secondHeader.numParts * secondHeader.sizeOfPartitionEntries;
         storage = (char*) malloc(sizeOfParts);
         read(fd, storage, sizeOfParts);
         newCRC = chksum_crc32((unsigned char*) storage,  sizeOfParts);
         free(storage);
         secondPartsCrcOk = (newCRC == secondHeader.partitionEntriesCRC);
      } // if

      // Check for valid CRCs and warn if there are problems
      if ((mainCrcOk == 0) || (secondCrcOk == 0) || (mainPartsCrcOk == 0) ||
           (secondPartsCrcOk == 0)) {
         printf("Warning! One or more CRCs don't match. You should repair the disk!\n");
         state = gpt_corrupt;
           } // if
   } else {
      state = gpt_invalid;
   } // if/else
   return allOK;
} // GPTData::ForceLoadGPTData()

// Loads the partition tables pointed to by the main GPT header. The
// main GPT header in memory MUST be valid for this call to do anything
// sensible!
int GPTData::LoadMainTable(void) {
   int fd, retval = 0;
   uint32_t newCRC, sizeOfParts;

   if ((fd = open(device, O_RDONLY)) != -1) {
      // Set internal data structures for number of partitions on the disk
      SetGPTSize(mainHeader.numParts);

      // Load main partition table, and record whether its CRC
      // matches the stored value
      lseek64(fd, mainHeader.partitionEntriesLBA * blockSize, SEEK_SET);
      sizeOfParts = mainHeader.numParts * mainHeader.sizeOfPartitionEntries;
      read(fd, partitions, sizeOfParts);
      newCRC = chksum_crc32((unsigned char*) partitions, sizeOfParts);
      mainPartsCrcOk = (newCRC == mainHeader.partitionEntriesCRC);
      if (IsLittleEndian() == 0)
         ReversePartitionBytes();
      retval = 1;
   } // if
   return retval;
} // GPTData::LoadMainTable()

// Load the second (backup) partition table as the primary partition
// table. Used in repair functions
void GPTData::LoadSecondTableAsMain(void) {
   int fd;
   off_t seekTo;
   uint32_t sizeOfParts, newCRC;

   if ((fd = open(device, O_RDONLY)) != -1) {
      seekTo = secondHeader.partitionEntriesLBA * (off_t) blockSize;
      if (lseek64(fd, seekTo, SEEK_SET) != (off_t) -1) {
         SetGPTSize(secondHeader.numParts);
         sizeOfParts = secondHeader.numParts * secondHeader.sizeOfPartitionEntries;
         read(fd, partitions, sizeOfParts);
         newCRC = chksum_crc32((unsigned char*) partitions, sizeOfParts);
         secondPartsCrcOk = (newCRC == secondHeader.partitionEntriesCRC);
         mainPartsCrcOk = secondPartsCrcOk;
         if (IsLittleEndian() == 0)
            ReversePartitionBytes();
         if (!secondPartsCrcOk) {
            printf("Error! After loading backup partitions, the CRC still doesn't check out!\n");
         } // if
      } else {
         printf("Error! Couldn't seek to backup partition table!\n");
      } // if/else
   } else {
      printf("Error! Couldn't open device %s when recovering backup partition table!\n");
   } // if/else
} // GPTData::LoadSecondTableAsMain()

// Writes GPT (and protective MBR) to disk. Returns 1 on successful
// write, 0 if there was a problem.
int GPTData::SaveGPTData(void) {
   int allOK = 1, i, j;
   char answer, line[256];
   int fd;
   uint64_t secondTable;
   uint32_t numParts;
   off_t offset;

   if (strlen(device) == 0) {
      printf("Device not defined.\n");
   } // if

   // First do some final sanity checks....
   // Is there enough space to hold the GPT headers and partition tables,
   // given the partition sizes?
   if (CheckGPTSize() > 0) {
      allOK = 0;
   } // if

   // Check that disk is really big enough to handle this...
   if (mainHeader.backupLBA > diskSize) {
      fprintf(stderr, "Error! Disk is too small -- either the original MBR is corrupt or you're\n");
      fprintf(stderr, "working from an MBR copied to a file! Aborting!\n");
      printf("(Disk size is %ld sectors, needs to be %ld sectors.)\n", diskSize,
             mainHeader.backupLBA);
      allOK = 0;
   } // if

   // Check for overlapping partitions....
   if (FindOverlaps() > 0) {
      allOK = 0;
      fprintf(stderr, "Aborting write operation!\n");
   } // if

   // Check for mismatched MBR and GPT data, but let it pass if found
   // (function displays warning message)
   FindHybridMismatches();

   // Pull out some data that's needed before doing byte-order reversal on
   // big-endian systems....
   numParts = mainHeader.numParts;
   secondTable = secondHeader.partitionEntriesLBA;
   if (IsLittleEndian() == 0) {
      // Reverse partition bytes first, since that function requires non-reversed
      // data from the main header....
      ReversePartitionBytes();
      ReverseHeaderBytes(&mainHeader);
      ReverseHeaderBytes(&secondHeader);
   } // if
   RecomputeCRCs();

   if (allOK) {
      printf("\nFinal checks complete. About to write GPT data. THIS WILL OVERWRITE EXISTING\n");
      printf("MBR PARTITIONS!! THIS PROGRAM IS BETA QUALITY AT BEST. IF YOU LOSE ALL YOUR\n");
      printf("DATA, YOU HAVE ONLY YOURSELF TO BLAME IF YOU ANSWER 'Y' BELOW!\n\n");
      printf("Do you want to proceed, possibly destroying your data? (Y/N) ");
      fgets(line, 255, stdin);
      sscanf(line, "%c", &answer);
      if ((answer == 'Y') || (answer == 'y')) {
         printf("OK; writing new GPT partition table.\n");
      } else {
         allOK = 0;
      } // if/else
   } // if

   // Do it!
   if (allOK) {
      fd = OpenForWrite(device);
      if (fd != -1) {
         // First, write the protective MBR...
         protectiveMBR.WriteMBRData(fd);

         // Now write the main GPT header...
         if (allOK)
            if (write(fd, &mainHeader, 512) == -1)
               allOK = 0;

         // Now write the main partition tables...
         if (allOK) {
            if (write(fd, partitions, GPT_SIZE * numParts) == -1)
               allOK = 0;
         } // if

         // Now seek to near the end to write the secondary GPT....
         if (allOK) {
            offset = (off_t) secondTable * (off_t) (blockSize);
            if (lseek64(fd, offset, SEEK_SET) == (off_t) - 1) {
               allOK = 0;
               printf("Unable to seek to end of disk!\n");
            } // if
         } // if

         // Now write the secondary partition tables....
         if (allOK)
            if (write(fd, partitions, GPT_SIZE * numParts) == -1)
               allOK = 0;

         // Now write the secondary GPT header...
         if (allOK)
            if (write(fd, &secondHeader, 512) == -1)
               allOK = 0;

         // re-read the partition table
         if (allOK) {
            sync();
#ifdef __APPLE__
            printf("Warning: The kernel may continue to use old or deleted partitions.\n"
                  "You should reboot or remove the drive.\n");
	    /* don't know if this helps
	     * it definitely will get things on disk though:
            * http://topiks.org/mac-os-x/0321278542/ch12lev1sec8.html */
            i = ioctl(fd, DKIOCSYNCHRONIZECACHE);
#else
#ifdef __FreeBSD__
            sleep(2);
            i = ioctl(fd, DIOCGFLUSH);
            printf("Warning: The kernel may still provide disk access using old partition IDs.\n");
#else
            sleep(2);
            i = ioctl(fd, BLKRRPART);
            if (i)
               printf("Warning: The kernel is still using the old partition table.\n"
                     "The new table will be used at the next reboot.\n");
#endif
#endif
         } // if

         if (allOK) { // writes completed OK
            printf("The operation has completed successfully.\n");
         } else {
            printf("Warning! An error was reported when writing the partition table! This error\n");
            printf("MIGHT be harmless, but you may have trashed the disk! Use parted and, if\n");
            printf("necessary, restore your original partition table.\n");
         } // if/else
         close(fd);
      } else {
         fprintf(stderr, "Unable to open device %s for writing! Errno is %d! Aborting write!\n",
                 device, errno);
         allOK = 0;
      } // if/else
   } else {
      printf("Aborting write of new partition table.\n");
   } // if

   if (IsLittleEndian() == 0) {
      // Reverse (normalize) header bytes first, since ReversePartitionBytes()
      // requires non-reversed data in mainHeader...
      ReverseHeaderBytes(&mainHeader);
      ReverseHeaderBytes(&secondHeader);
      ReversePartitionBytes();
   } // if

   return (allOK);
} // GPTData::SaveGPTData()

// Save GPT data to a backup file. This function does much less error
// checking than SaveGPTData(). It can therefore preserve many types of
// corruption for later analysis; however, it preserves only the MBR,
// the main GPT header, the backup GPT header, and the main partition
// table; it discards the backup partition table, since it should be
// identical to the main partition table on healthy disks.
int GPTData::SaveGPTBackup(char* filename) {
   int fd, allOK = 1;
   uint32_t numParts;

   if ((fd = open(filename, O_WRONLY | O_CREAT, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH)) != -1) {
      // Reverse the byte order, if necessary....
      numParts = mainHeader.numParts;
      if (IsLittleEndian() == 0) {
         ReversePartitionBytes();
         ReverseHeaderBytes(&mainHeader);
         ReverseHeaderBytes(&secondHeader);
      } // if

      // Now write the protective MBR...
      protectiveMBR.WriteMBRData(fd);

      // Now write the main GPT header...
      if (allOK)
         if (write(fd, &mainHeader, 512) == -1)
            allOK = 0;

      // Now write the secondary GPT header...
      if (allOK)
         if (write(fd, &secondHeader, 512) == -1)
            allOK = 0;

      // Now write the main partition tables...
      if (allOK) {
         if (write(fd, partitions, GPT_SIZE * numParts) == -1)
            allOK = 0;
      } // if

      if (allOK) { // writes completed OK
         printf("The operation has completed successfully.\n");
      } else {
         printf("Warning! An error was reported when writing the backup file.\n");
         printf("It may not be useable!\n");
      } // if/else
      close(fd);

      // Now reverse the byte-order reversal, if necessary....
      if (IsLittleEndian() == 0) {
         ReverseHeaderBytes(&mainHeader);
         ReverseHeaderBytes(&secondHeader);
         ReversePartitionBytes();
      } // if
   } else {
      fprintf(stderr, "Unable to open file %s for writing! Aborting!\n", filename);
      allOK = 0;
   } // if/else
   return allOK;
} // GPTData::SaveGPTBackup()

// Load GPT data from a backup file created by SaveGPTBackup(). This function
// does minimal error checking. It returns 1 if it completed successfully,
// 0 if there was a problem. In the latter case, it creates a new empty
// set of partitions.
int GPTData::LoadGPTBackup(char* filename) {
   int fd, allOK = 1, val;
   uint32_t numParts, sizeOfEntries, sizeOfParts, newCRC;
   int littleEndian = 1;

   if ((fd = open(filename, O_RDONLY)) != -1) {
      if (IsLittleEndian() == 0)
         littleEndian = 0;

      // Let the MBRData class load the saved MBR...
      protectiveMBR.ReadMBRData(fd, 0); // 0 = don't check block size

      // Load the main GPT header, check its vaility, and set the GPT
      // size based on the data
      read(fd, &mainHeader, 512);
      mainCrcOk = CheckHeaderCRC(&mainHeader);

      // Reverse byte order, if necessary
      if (littleEndian == 0) {
         ReverseHeaderBytes(&mainHeader);
      } // if

      // Load the backup GPT header in much the same way as the main
      // GPT header....
      read(fd, &secondHeader, 512);
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
            printf("Warning! Current disk size doesn't match that of the backup!\n"
                  "Adjusting sizes to match, but subsequent problems are possible!\n");
            secondHeader.currentLBA = mainHeader.backupLBA = diskSize - UINT64_C(1);
            mainHeader.lastUsableLBA = diskSize - mainHeader.firstUsableLBA;
            secondHeader.lastUsableLBA = mainHeader.lastUsableLBA;
            secondHeader.partitionEntriesLBA = secondHeader.lastUsableLBA + UINT64_C(1);
         } // if

         // Load main partition table, and record whether its CRC
         // matches the stored value
         sizeOfParts = numParts * sizeOfEntries;
         read(fd, partitions, sizeOfParts);

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
      fprintf(stderr, "Unable to open file %s for reading! Aborting!\n", filename);
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
      printf("  APM: present\n");
   else
      printf("  APM: not present\n");
} // GPTData::ShowAPMState()

// Tell user about the state of the GPT data....
void GPTData::ShowGPTState(void) {
   switch (state) {
      case gpt_invalid:
         printf("  GPT: not present\n");
         break;
      case gpt_valid:
         printf("  GPT: present\n");
         break;
      case gpt_corrupt:
         printf("  GPT: damaged\n");
         break;
      default:
         printf("\a  GPT: unknown -- bug!\n");
         break;
   } // switch
} // GPTData::ShowGPTState()

// Display the basic GPT data
void GPTData::DisplayGPTData(void) {
   int i, j;
   char sizeInSI[255]; // String to hold size of disk in SI units
   char tempStr[255];
   uint64_t temp, totalFree;

   BytesToSI(diskSize * blockSize, sizeInSI);
   printf("Disk %s: %lu sectors, %s\n", device,
          (unsigned long) diskSize, sizeInSI);
   printf("Disk identifier (GUID): %s\n", GUIDToStr(mainHeader.diskGUID, tempStr));
   printf("Partition table holds up to %lu entries\n", (unsigned long) mainHeader.numParts);
   printf("First usable sector is %lu, last usable sector is %lu\n",
          (unsigned long) mainHeader.firstUsableLBA,
           (unsigned long) mainHeader.lastUsableLBA);
   totalFree = FindFreeBlocks(&i, &temp);
   printf("Total free space is %llu sectors (%s)\n", totalFree,
          BytesToSI(totalFree * (uint64_t) blockSize, sizeInSI));
   printf("\nNumber  Start (sector)    End (sector)  Size       Code  Name\n");
   for (i = 0; i < mainHeader.numParts; i++) {
      partitions[i].ShowSummary(i, blockSize, sizeInSI);
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
      printf("No partitions\n");
   } // if/else
} // GPTData::ShowDetails()

// Show detailed information on the specified partition
void GPTData::ShowPartDetails(uint32_t partNum) {
   if (partitions[partNum].GetFirstLBA() != 0) {
      partitions[partNum].ShowDetails(blockSize);
   } else {
      printf("Partition #%d does not exist.", (int) (partNum + 1));
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

   printf("Current partition table size is %lu.\n",
          (unsigned long) mainHeader.numParts);
   GetPartRange(&curLow, &curHigh);
   curHigh++; // since GetPartRange() returns numbers starting from 0...
   // There's no point in having fewer than four partitions....
   if (curHigh < 4)
      curHigh = 4;
   sprintf(prompt, "Enter new size (%d up, default %d): ", (int) curHigh,
           (int) NUM_GPT_ENTRIES);
   newSize = GetNumber(4, 65535, 128, prompt);
   if (newSize < 128) {
      printf("Caution: The partition table size should officially be 16KB or larger,\n"
            "which works out to 128 entries. In practice, smaller tables seem to\n"
            "work with most OSes, but this practice is risky. I'm proceeding with\n"
            "the resize, but you may want to reconsider this action and undo it.\n\n");
   } // if
   SetGPTSize(newSize);
} // GPTData::ResizePartitionTable()

// Interactively create a partition
void GPTData::CreatePartition(void) {
   uint64_t firstBlock, firstInLargest, lastBlock, sector;
   char prompt[255];
   int partNum, firstFreePart = 0;

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
            printf("partition %d is in use.\n", partNum + 1);
      } while (partitions[partNum].GetFirstLBA() != 0);

      // Get first block for new partition...
      sprintf(prompt,
              "First sector (%llu-%llu, default = %llu) or {+-}size{KMGT}: ",
              firstBlock, lastBlock, firstInLargest);
      do {
         sector = GetSectorNum(firstBlock, lastBlock, firstInLargest, prompt);
      } while (IsFree(sector) == 0);
      firstBlock = sector;

      // Get last block for new partitions...
      lastBlock = FindLastInFree(firstBlock);
      sprintf(prompt,
              "Last sector (%llu-%llu, default = %llu) or {+-}size{KMGT}: ",
              firstBlock, lastBlock, lastBlock);
      do {
         sector = GetSectorNum(firstBlock, lastBlock, lastBlock, prompt);
      } while (IsFree(sector) == 0);
      lastBlock = sector;

      partitions[partNum].SetFirstLBA(firstBlock);
      partitions[partNum].SetLastLBA(lastBlock);

      partitions[partNum].SetUniqueGUID(1);
      partitions[partNum].ChangeType();
      partitions[partNum].SetName((unsigned char*) partitions[partNum].GetNameType(prompt));
         } else {
            printf("No free sectors available\n");
         } // if/else
} // GPTData::CreatePartition()

// Interactively delete a partition (duh!)
void GPTData::DeletePartition(void) {
   int partNum;
   uint32_t low, high;
   uint64_t startSector, length;
   char prompt[255];

   if (GetPartRange(&low, &high) > 0) {
      sprintf(prompt, "Partition number (%d-%d): ", low + 1, high + 1);
      partNum = GetNumber(low + 1, high + 1, low, prompt);

      // In case there's a protective MBR, look for & delete matching
      // MBR partition....
      startSector = partitions[partNum - 1].GetFirstLBA();
      length = partitions[partNum - 1].GetLengthLBA();
      protectiveMBR.DeleteByLocation(startSector, length);

      // Now delete the GPT partition
      partitions[partNum - 1].BlankPartition();
   } else {
      printf("No partitions\n");
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
      printf("No partitions\n");
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
int GPTData::DestroyGPT(void) {
   int fd, i, doMore;
   char blankSector[512], goOn;

   for (i = 0; i < 512; i++) {
      blankSector[i] = '\0';
   } // for

   printf("\a\aAbout to wipe out GPT on %s. Proceed? ", device);
   goOn = GetYN();
   if (goOn == 'Y') {
      fd = open(device, O_WRONLY);
#ifdef __APPLE__
      // MacOS X requires a shared lock under some circumstances....
      if (fd < 0) {
         fd = open(device, O_WRONLY|O_SHLOCK);
      } // if
#endif
      if (fd != -1) {
         lseek64(fd, mainHeader.currentLBA * 512, SEEK_SET); // seek to GPT header
         write(fd, blankSector, 512); // blank it out
         lseek64(fd, mainHeader.partitionEntriesLBA * 512, SEEK_SET); // seek to partition table
         for (i = 0; i < GetBlocksInPartTable(); i++)
            write(fd, blankSector, 512);
         lseek64(fd, secondHeader.partitionEntriesLBA * 512, SEEK_SET); // seek to partition table
         for (i = 0; i < GetBlocksInPartTable(); i++)
            write(fd, blankSector, 512);
         lseek64(fd, secondHeader.currentLBA * 512, SEEK_SET); // seek to GPT header
         write(fd, blankSector, 512); // blank it out
         printf("Blank out MBR? ");
         if (GetYN() == 'Y') {
            lseek64(fd, 0, SEEK_SET);
            write(fd, blankSector, 512); // blank it out
         } // if blank MBR
         close(fd);
         printf("GPT data structures destroyed! You may now partition the disk using fdisk or\n"
               "other utilities. Program will now terminate.\n");
      } else {
         printf("Problem opening %s for writing! Program will now terminate.\n");
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
      printf("\n\a***************************************************************\n"
            "Found invalid GPT and valid MBR; converting MBR to GPT format.\n"
            "THIS OPERATON IS POTENTIALLY DESTRUCTIVE! Exit by typing 'q' if\n"
            "you don't want to convert your MBR partitions to GPT format!\n"
            "***************************************************************\n\n");
      which = use_mbr;
   } // if

   if ((state == gpt_invalid) && bsdFound) {
      printf("\n\a**********************************************************************\n"
            "Found invalid GPT and valid BSD disklabel; converting BSD disklabel\n"
            "to GPT format. THIS OPERATON IS POTENTIALLY DESTRUCTIVE! Your first\n"
            "BSD partition will likely be unusable. Exit by typing 'q' if you don't\n"
            "want to convert your BSD partitions to GPT format!\n"
            "**********************************************************************\n\n");
      which = use_bsd;
   } // if

   if ((state == gpt_valid) && (mbrState == gpt)) {
      printf("Found valid GPT with protective MBR; using GPT.\n");
      which = use_gpt;
   } // if
   if ((state == gpt_valid) && (mbrState == hybrid)) {
      printf("Found valid GPT with hybrid MBR; using GPT.\n");
      which = use_gpt;
   } // if
   if ((state == gpt_valid) && (mbrState == invalid)) {
      printf("\aFound valid GPT with corrupt MBR; using GPT and will create new\nprotective MBR on save.\n");
      which = use_gpt;
      protectiveMBR.MakeProtectiveMBR();
   } // if
   if ((state == gpt_valid) && (mbrState == mbr)) {
      printf("Found valid MBR and GPT. Which do you want to use?\n");
      answer = GetNumber(1, 3, 2, (char*) " 1 - MBR\n 2 - GPT\n 3 - Create blank GPT\n\nYour answer: ");
      if (answer == 1) {
         which = use_mbr;
      } else if (answer == 2) {
         which = use_gpt;
         protectiveMBR.MakeProtectiveMBR();
         printf("Using GPT and creating fresh protective MBR.\n");
      } else which = use_new;
   } // if

   // Nasty decisions here -- GPT is present, but corrupt (bad CRCs or other
   // problems)
   if (state == gpt_corrupt) {
      if ((mbrState == mbr) || (mbrState == hybrid)) {
         printf("Found valid MBR and corrupt GPT. Which do you want to use? (Using the\n"
               "GPT MAY permit recovery of GPT data.)\n");
         answer = GetNumber(1, 3, 2, (char*) " 1 - MBR\n 2 - GPT\n 3 - Create blank GPT\n\nYour answer: ");
         if (answer == 1) {
            which = use_mbr;
//            protectiveMBR.MakeProtectiveMBR();
         } else if (answer == 2) {
            which = use_gpt;
         } else which = use_new;
      } else if (mbrState == invalid) {
         printf("Found invalid MBR and corrupt GPT. What do you want to do? (Using the\n"
               "GPT MAY permit recovery of GPT data.)\n");
         answer = GetNumber(1, 2, 1, (char*) " 1 - GPT\n 2 - Create blank GPT\n\nYour answer: ");
         if (answer == 1) {
            which = use_gpt;
         } else which = use_new;
      } else { // corrupt GPT, MBR indicates it's a GPT disk....
         printf("\a\a****************************************************************************\n"
               "Caution: Found protective or hybrid MBR and corrupt GPT. Using GPT, but disk\n"
               "verification and recovery are STRONGLY recommended.\n"
               "****************************************************************************\n");
      } // if/else/else
   } // if (corrupt GPT)

   if (which == use_new)
      printf("Creating new GPT entries.\n");

   return which;
} // UseWhichPartitions()

// Convert MBR partition table into GPT form
int GPTData::XFormPartitions(void) {
   int i, numToConvert;
   uint8_t origType;
   struct newGUID;
   char name[NAME_SIZE];

   // Clear out old data & prepare basics....
   ClearGPTData();

   // Convert the smaller of the # of GPT or MBR partitions
   if (mainHeader.numParts > (NUM_LOGICALS + 4))
      numToConvert = NUM_LOGICALS + 4;
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
// Returns the number of new partitions created.
int GPTData::XFormDisklabel(int i) {
   uint32_t low, high, partNum, startPart;
   uint16_t hexCode;
   int goOn = 1, numDone = 0;
   BSDData disklabel;

   if (GetPartRange(&low, &high) != 0) {
      if ((i < low) || (i > high))
         partNum = GetPartNum();
      else
         partNum = (uint32_t) i;

      // Find the partition after the last used one
      startPart = high + 1;

      // Now see if the specified partition has a BSD type code....
      hexCode = partitions[partNum].GetHexType();
      if ((hexCode != 0xa500) && (hexCode != 0xa900)) {
         printf("Specified partition doesn't have a disklabel partition type "
               "code.\nContinue anyway?");
         goOn = (GetYN() == 'Y');
      } // if

      // If all is OK, read the disklabel and convert it.
      if (goOn) {
         goOn = disklabel.ReadBSDData(device, partitions[partNum].GetFirstLBA(),
                                      partitions[partNum].GetLastLBA());
         if ((goOn) && (disklabel.IsDisklabel())) {
            numDone = XFormDisklabel(&disklabel, startPart);
            if (numDone == 1)
               printf("Converted %d BSD partition.\n", numDone);
            else
               printf("Converted %d BSD partitions.\n", numDone);
         } else {
            printf("Unable to convert partitions! Unrecognized BSD disklabel.\n");
         } // if/else
      } // if
      if (numDone > 0) { // converted partitions; delete carrier
         partitions[partNum].BlankPartition();
      } // if
   } else {
      printf("No partitions\n");
   } // if/else
   return numDone;
} // GPTData::XFormDisklable(int i)

// Transform the partitions on an already-loaded BSD disklabel...
int GPTData::XFormDisklabel(BSDData* disklabel, int startPart) {
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

// Create a hybrid MBR -- an ugly, funky thing that helps GPT work with
// OSes that don't understand GPT.
void GPTData::MakeHybrid(void) {
   uint32_t partNums[3];
   char line[255];
   int numParts, i, j, typeCode, bootable, mbrNum;
   uint64_t length;
   char fillItUp = 'M'; // fill extra partition entries? (Yes/No/Maybe)
   char eeFirst; // Whether EFI GPT (0xEE) partition comes first in table

   printf("\nWARNING! Hybrid MBRs are flaky and potentially dangerous! If you decide not\n"
         "to use one, just hit the Enter key at the below prompt and your MBR\n"
         "partition table will be untouched.\n\n\a");

   // Now get the numbers of up to three partitions to add to the
   // hybrid MBR....
   printf("Type from one to three GPT partition numbers, separated by spaces, to be\n"
         "added to the hybrid MBR, in sequence: ");
   fgets(line, 255, stdin);
   numParts = sscanf(line, "%d %d %d", &partNums[0], &partNums[1], &partNums[2]);

   if (numParts > 0) {
      // Blank out the protective MBR, but leave the boot loader code
      // alone....
      protectiveMBR.EmptyMBR(0);
      protectiveMBR.SetDiskSize(diskSize);
      printf("Place EFI GPT (0xEE) partition first in MBR (good for GRUB)? ");
      eeFirst = GetYN();
   } // if

   for (i = 0; i < numParts; i++) {
      j = partNums[i] - 1;
      printf("\nCreating entry for partition #%d\n", j + 1);
      if ((j >= 0) && (j < mainHeader.numParts)) {
         if (partitions[j].GetLastLBA() < UINT32_MAX) {
            do {
               printf("Enter an MBR hex code (default %02X): ",
                      typeHelper.GUIDToID(partitions[j].GetType()) / 256);
               fgets(line, 255, stdin);
               sscanf(line, "%x", &typeCode);
               if (line[0] == '\n')
                  typeCode = partitions[j].GetHexType() / 256;
            } while ((typeCode <= 0) || (typeCode > 255));
            printf("Set the bootable flag? ");
            bootable = (GetYN() == 'Y');
            length = partitions[j].GetLengthLBA();
            if (eeFirst == 'Y')
               mbrNum = i + 1;
            else
               mbrNum = i;
            protectiveMBR.MakePart(mbrNum, (uint32_t) partitions[j].GetFirstLBA(),
                                   (uint32_t) length, typeCode, bootable);
            protectiveMBR.SetHybrid();
         } else { // partition out of range
            printf("Partition %d ends beyond the 2TiB limit of MBR partitions; omitting it.\n",
                   j + 1);
         } // if/else
      } else {
         printf("Partition %d is out of range; omitting it.\n", j + 1);
      } // if/else
   } // for

   if (numParts > 0) { // User opted to create a hybrid MBR....
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

      // ... and for good measure, if there are any partition spaces left,
      // optionally create another protective EFI partition to cover as much
      // space as possible....
      for (i = 0; i < 4; i++) {
         if (protectiveMBR.GetType(i) == 0x00) { // unused entry....
            if (fillItUp == 'M') {
               printf("\nUnused partition space(s) found. Use one to protect more partitions? ");
               fillItUp = GetYN();
               typeCode = 0x00; // use this to flag a need to get type code
            } // if
            if (fillItUp == 'Y') {
               while ((typeCode <= 0) || (typeCode > 255)) {
                  printf("Enter an MBR hex code (EE is EFI GPT, but may confuse MacOS): ");
                  // Comment on above: Mac OS treats disks with more than one
                  // 0xEE MBR partition as MBR disks, not as GPT disks.
                  fgets(line, 255, stdin);
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
// necessary, copies data if it already exists.
int GPTData::SetGPTSize(uint32_t numEntries) {
   struct GPTPart* newParts;
   struct GPTPart* trash;
   uint32_t i, high, copyNum;
   int allOK = 1;

   // First, adjust numEntries upward, if necessary, to get a number
   // that fills the allocated sectors
   i = blockSize / GPT_SIZE;
   if ((numEntries % i) != 0) {
      printf("Adjusting GPT size from %lu ", (unsigned long) numEntries);
      numEntries = ((numEntries / i) + 1) * i;
      printf("to %lu to fill the sector\n", (unsigned long) numEntries);
   } // if

   newParts = (GPTPart*) calloc(numEntries, sizeof (GPTPart));
   if (newParts != NULL) {
      if (partitions != NULL) { // existing partitions; copy them over
         GetPartRange(&i, &high);
         if (numEntries < (high + 1)) { // Highest entry too high for new #
            printf("The highest-numbered partition is %lu, which is greater than the requested\n"
                  "partition table size of %d; cannot resize. Perhaps sorting will help.\n",
                  (unsigned long) (high + 1), numEntries);
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
      mainHeader.lastUsableLBA = diskSize - mainHeader.firstUsableLBA;
      secondHeader.lastUsableLBA = mainHeader.lastUsableLBA;
      secondHeader.partitionEntriesLBA = secondHeader.lastUsableLBA + UINT64_C(1);
      if (diskSize > 0)
         CheckGPTSize();
   } else { // Bad memory allocation
      fprintf(stderr, "Error allocating memory for partition table!\n");
      allOK = 0;
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

// Sort the GPT entries, eliminating gaps and making for a logical
// ordering. Relies on QuickSortGPT() for the bulk of the work
void GPTData::SortGPT(void) {
   int i, lastPart = 0;
   GPTPart temp;

   // First, find the last partition with data, so as not to
   // spend needless time sorting empty entries....
   for (i = 0; i < mainHeader.numParts; i++) {
      if (partitions[i].GetFirstLBA() > 0)
         lastPart = i;
   } // for

   // Now swap empties with the last partitions, to simplify the logic
   // in the Quicksort function....
   i = 0;
   while (i < lastPart) {
      if (partitions[i].GetFirstLBA() == 0) {
         temp = partitions[i];
         partitions[i] = partitions[lastPart];
         partitions[lastPart] = temp;
         lastPart--;
      } // if
      i++;
   } // while

   // Now call the recursive quick sort routine to do the real work....
   QuickSortGPT(partitions, 0, lastPart);
} // GPTData::SortGPT()

// Set up data structures for entirely new set of partitions on the
// specified device. Returns 1 if OK, 0 if there were problems.
int GPTData::ClearGPTData(void) {
   int goOn, i;

   // Set up the partition table....
   free(partitions);
   partitions = NULL;
   SetGPTSize(NUM_GPT_ENTRIES);

   // Now initialize a bunch of stuff that's static....
   mainHeader.signature = GPT_SIGNATURE;
   mainHeader.revision = 0x00010000;
   mainHeader.headerSize = (uint32_t) HEADER_SIZE;
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
   // rand() is only 32 bits, so multiply together to fill a 64-bit value
   mainHeader.diskGUID.data1 = (uint64_t) rand() * (uint64_t) rand();
   mainHeader.diskGUID.data2 = (uint64_t) rand() * (uint64_t) rand();

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

void GPTData::SetName(uint32_t partNum, char* theName) {
   if ((partNum >= 0) && (partNum < mainHeader.numParts))
      if (partitions[partNum].GetFirstLBA() > 0)
         partitions[partNum].SetName((unsigned char*) theName);
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
   uint64_t start, firstBlock, lastBlock, segmentSize, selectedSize = 0, selectedSegment;

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
uint64_t GPTData::FindFreeBlocks(int *numSegments, uint64_t *largestSegment) {
   uint64_t start = UINT64_C(0); // starting point for each search
   uint64_t totalFound = UINT64_C(0); // running total
   uint64_t firstBlock; // first block in a segment
   uint64_t lastBlock; // last block in a segment
   uint64_t segmentSize; // size of segment in blocks
   int num = 0;

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
   ReverseBytes(&header->diskGUID.data1, 8);
   ReverseBytes(&header->diskGUID.data2, 8);
} // GPTData::ReverseHeaderBytes()

// IMPORTANT NOTE: This function requires non-reversed mainHeader
// structure!
void GPTData::ReversePartitionBytes() {
   uint32_t i;

   // Check GPT signature on big-endian systems; this will mismatch
   // if the function is called out of order. Unfortunately, it'll also
   // mismatch if there's data corruption.
   if ((mainHeader.signature != GPT_SIGNATURE) && (IsLittleEndian() == 0)) {
      printf("GPT signature mismatch in GPTData::ReversePartitionBytes(). This indicates\n"
            "data corruption or a misplaced call to this function.\n");
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
      fprintf(stderr, "uint8_t is %d bytes, should be 1 byte; aborting!\n", sizeof(uint8_t));
      allOK = 0;
   } // if
   if (sizeof(uint16_t) != 2) {
      fprintf(stderr, "uint16_t is %d bytes, should be 2 bytes; aborting!\n", sizeof(uint16_t));
      allOK = 0;
   } // if
   if (sizeof(uint32_t) != 4) {
      fprintf(stderr, "uint32_t is %d bytes, should be 4 bytes; aborting!\n", sizeof(uint32_t));
      allOK = 0;
   } // if
   if (sizeof(uint64_t) != 8) {
      fprintf(stderr, "uint64_t is %d bytes, should be 8 bytes; aborting!\n", sizeof(uint64_t));
      allOK = 0;
   } // if
   if (sizeof(struct MBRRecord) != 16) {
      fprintf(stderr, "MBRRecord is %d bytes, should be 16 bytes; aborting!\n", sizeof(MBRRecord));
      allOK = 0;
   } // if
   if (sizeof(struct EBRRecord) != 512) {
      fprintf(stderr, "EBRRecord is %d bytes, should be 512 bytes; aborting!\n", sizeof(EBRRecord));
      allOK = 0;
   } // if
   if (sizeof(struct GPTHeader) != 512) {
      fprintf(stderr, "GPTHeader is %d bytes, should be 512 bytes; aborting!\n", sizeof(GPTHeader));
      allOK = 0;
   } // if
   if (sizeof(GPTPart) != 128) {
      fprintf(stderr, "GPTPart is %d bytes, should be 128 bytes; aborting!\n", sizeof(GPTPart));
      allOK = 0;
   } // if
// Determine endianness; set allOK = 0 if running on big-endian hardware
   if (IsLittleEndian() == 0) {
      fprintf(stderr, "\aRunning on big-endian hardware. Big-endian support is new and poorly"
            " tested!\nBeware!\n");
      // allOK = 0;
   } // if
   return (allOK);
} // SizesOK()

