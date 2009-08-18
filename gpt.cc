/* gpt.cc -- Functions for loading, saving, and manipulating legacy MBR and GPT partition
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
#include "crc32.h"
#include "gpt.h"
#include "support.h"
#include "parttypes.h"
#include "attributes.h"

using namespace std;

/****************************************
 *                                      *
 * GPTData class and related structures *
 *                                      *
 ****************************************/

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
   srand((unsigned int) time(NULL));
   LoadPartitions(filename);
} // GPTData(char* filename) constructor

GPTData::~GPTData(void) {
   free(partitions);
} // GPTData destructor

// Resizes GPT to specified number of entries. Creates a new table if
// necessary, copies data if it already exists.
int GPTData::SetGPTSize(uint32_t numEntries) {
   struct GPTPartition* newParts;
   struct GPTPartition* trash;
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

   newParts = (struct GPTPartition*) calloc(numEntries, sizeof (struct GPTPartition));
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

// Checks to see if the GPT tables overrun existing partitions; if they
// do, issues a warning but takes no action. Returns 1 if all is OK, 0
// if problems were detected.
int GPTData::CheckGPTSize(void) {
   uint64_t overlap, firstUsedBlock, lastUsedBlock;
   uint32_t i;
   int allOK = 1;

   // first, locate the first & last used blocks
   firstUsedBlock = UINT64_MAX;
   lastUsedBlock = 0;
   for (i = 0; i < mainHeader.numParts; i++) {
      if ((partitions[i].firstLBA < firstUsedBlock) &&
          (partitions[i].firstLBA != 0))
         firstUsedBlock = partitions[i].firstLBA;
      if (partitions[i].lastLBA > lastUsedBlock)
         lastUsedBlock = partitions[i].lastLBA;
   } // for

   // If the disk size is 0 (the default), then it means that various
   // variables aren't yet set, so the below tests will be useless;
   // therefore we should skip everything
   if (diskSize != 0) {
      if (mainHeader.firstUsableLBA > firstUsedBlock) {
         overlap = mainHeader.firstUsableLBA - firstUsedBlock;
         printf("Warning! Main partition table overlaps the first partition by %lu\n"
                "blocks! Try reducing the partition table size by %lu entries.\n",
                (unsigned long) overlap, (unsigned long) (overlap * 4));
         printf("(Use the 's' item on the experts' menu.)\n");
         allOK = 0;
      } // Problem at start of disk
      if (mainHeader.lastUsableLBA < lastUsedBlock) {
         overlap = lastUsedBlock - mainHeader.lastUsableLBA;
         printf("Warning! Secondary partition table overlaps the last partition by %lu\n"
                "blocks! Try reducing the partition table size by %lu entries.\n",
                (unsigned long) overlap, (unsigned long) (overlap * 4));
         printf("(Use the 's' item on the experts' menu.)\n");
         allOK = 0;
      } // Problem at end of disk
   } // if (diskSize != 0)
   return allOK;
} // GPTData::CheckGPTSize()

// Read GPT data from a disk.
int GPTData::LoadPartitions(char* deviceFilename) {
   int fd, err;
   int allOK = 1, i;
   uint64_t firstBlock, lastBlock;

   if ((fd = open(deviceFilename, O_RDONLY)) != -1) {
      // store disk information....
      diskSize = disksize(fd, &err);
      blockSize = (uint32_t) GetBlockSize(fd);
      strcpy(device, deviceFilename);

      // Read the MBR
      protectiveMBR.ReadMBRData(fd);

      // Load the GPT data, whether or not it's valid
      ForceLoadGPTData(fd);

      switch (UseWhichPartitions()) {
         case use_mbr:
//            printf("In LoadPartitions(), using MBR\n");
            XFormPartitions(&protectiveMBR);
            break;
         case use_gpt:
            break;
         case use_new:
//            printf("In LoadPartitions(), making new\n");
            ClearGPTData();
            protectiveMBR.MakeProtectiveMBR();
            break;
      } // switch

      // Now find the first and last sectors used by partitions...
      if (allOK) {
         firstBlock = mainHeader.backupLBA; // start high
	 lastBlock = 0; // start low
         for (i = 0; i < mainHeader.numParts; i++) {
	    if ((partitions[i].firstLBA < firstBlock) &&
                (partitions[i].firstLBA > 0))
	       firstBlock = partitions[i].firstLBA;
            if (partitions[i].lastLBA > lastBlock)
               lastBlock = partitions[i].lastLBA;
	 } // for
      } // if
      CheckGPTSize();
   } else {
      allOK = 0;
      fprintf(stderr, "Problem opening %s for reading!\n",
              deviceFilename);
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

   // Load backup header, check its CRC, and store the results of
   // the check for future reference
   seekTo = (diskSize * blockSize) - UINT64_C(512);
   if (lseek64(fd, seekTo, SEEK_SET) != (off_t) -1) {
      read(fd, &secondHeader, 512); // read secondary GPT header
      secondCrcOk = CheckHeaderCRC(&secondHeader);
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
      retval = 1;
   } // if
   return retval;
} // GPTData::LoadMainTable()

// Examines the MBR & GPT data, and perhaps asks the user questions, to
// determine which set of data to use: the MBR (use_mbr), the GPT (use_gpt),
// or create a new set of partitions (use_new)
WhichToUse GPTData::UseWhichPartitions(void) {
   WhichToUse which = use_new;
   MBRValidity mbrState;
   int answer;

   mbrState = protectiveMBR.GetValidity();

   if ((state == gpt_invalid) && (mbrState == mbr)) {
      printf("\n\a***************************************************************\n"
                    "Found invalid GPT and valid MBR; converting MBR to GPT format.\n"
             "THIS OPERATON IS POTENTIALLY DESTRUCTIVE! Exit by typing 'q' if\n"
             "you don't want to convert your MBR partitions to GPT format!\n"
             "***************************************************************\n\n");
      which = use_mbr;
   } // if
   if ((state == gpt_valid) && (mbrState == gpt)) {
      printf("Found valid GPT with protective MBR; using GPT.\n");
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
      if (mbrState == mbr) {
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
      } else {
         printf("\a\a****************************************************************************\n"
                "Caution: Found protective or hybrid MBR and corrupt GPT. Using GPT, but disk\n"
                "verification and recovery are STRONGLY recommended.\n"
                "****************************************************************************\n");
      } // if
   } // if

   if (which == use_new)
      printf("Creating new GPT entries.\n");

   return which;
} // UseWhichPartitions()

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
         if (partitions[i].firstLBA != UINT64_C(0)) { // it exists
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
   printf("\nNumber  Start (block)     End (block)  Size        Code  Name\n");
   for (i = 0; i < mainHeader.numParts; i++) {
      if (partitions[i].firstLBA != 0) {
         BytesToSI(blockSize * (partitions[i].lastLBA - partitions[i].firstLBA + 1),
                   sizeInSI);
         printf("%4d  %14lu  %14lu  ", i + 1, (unsigned long) partitions[i].firstLBA,
                (unsigned long) partitions[i].lastLBA);
         printf(" %-10s  %04X  ", sizeInSI,
                typeHelper.GUIDToID(partitions[i].partitionType));
         j = 0;
         while ((partitions[i].name[j] != '\0') && (j < 44)) {
            printf("%c", partitions[i].name[j]);
	    j += 2;
         } // while
         printf("\n");
      } // if
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
   char temp[255];
   int i;
   uint64_t size;

   if (partitions[partNum].firstLBA != 0) {
      printf("Partition GUID code: %s ", GUIDToStr(partitions[partNum].partitionType, temp));
      printf("(%s)\n", typeHelper.GUIDToName(partitions[partNum].partitionType, temp));
      printf("Partition unique GUID: %s\n", GUIDToStr(partitions[partNum].uniqueGUID, temp));

      printf("First sector: %llu (at %s)\n", (unsigned long long) 
             partitions[partNum].firstLBA,
	     BytesToSI(partitions[partNum].firstLBA * blockSize, temp));
      printf("Last sector: %llu (at %s)\n", (unsigned long long) 
             partitions[partNum].lastLBA,
	     BytesToSI(partitions[partNum].lastLBA * blockSize, temp));
      size = (partitions[partNum].lastLBA - partitions[partNum].firstLBA + 1);
      printf("Partition size: %llu sectprs (%s)\n", (unsigned long long)
             size, BytesToSI(size * ((uint64_t) blockSize), temp));
      printf("Attribute flags: %016llx\n", (unsigned long long)
             partitions[partNum].attributes);
      printf("Partition name: ");
      i = 0;
      while ((partitions[partNum].name[i] != '\0') && (i < NAME_SIZE)) {
         printf("%c", partitions[partNum].name[i]);
	 i += 2;
      } // while
      printf("\n");
   } else {
      printf("Partition #%d does not exist.", (int) (partNum + 1));
   } // if
} // GPTData::ShowPartDetails()

// Interactively create a partition
void GPTData::CreatePartition(void) {
   uint64_t firstBlock, lastBlock, sector;
   char prompt[255];
   int partNum, firstFreePart = 0;

   // Find first free partition...
   while (partitions[firstFreePart].firstLBA != 0) {
      firstFreePart++;
   } // while

   if (((firstBlock = FindFirstAvailable()) != 0) &&
       (firstFreePart < mainHeader.numParts)) {
      lastBlock = FindLastAvailable(firstBlock);

      // Get partition number....
      do {
         sprintf(prompt, "Partition number (%d-%d, default %d): ", firstFreePart + 1,
                 mainHeader.numParts, firstFreePart + 1);
         partNum = GetNumber(firstFreePart + 1, mainHeader.numParts,
                             firstFreePart + 1, prompt) - 1;
	 if (partitions[partNum].firstLBA != 0)
	    printf("partition %d is in use.\n", partNum + 1);
      } while (partitions[partNum].firstLBA != 0);

      // Get first block for new partition...
      sprintf(prompt, "First sector (%llu-%llu, default = %llu): ", firstBlock,
              lastBlock, firstBlock);
      do {
	 sector = GetNumber(firstBlock, lastBlock, firstBlock, prompt);
      } while (IsFree(sector) == 0);
      firstBlock = sector;

      // Get last block for new partitions...
      lastBlock = FindLastInFree(firstBlock);
      sprintf(prompt, "Last sector or +size or +sizeM or +sizeK (%llu-%llu, default = %d): ",
              firstBlock, lastBlock, lastBlock);
      do {
         sector = GetLastSector(firstBlock, lastBlock, prompt);
      } while (IsFree(sector) == 0);
      lastBlock = sector;

      partitions[partNum].firstLBA = firstBlock;
      partitions[partNum].lastLBA = lastBlock;

      // rand() is only 32 bits on 32-bit systems, so multiply together to
      // fill a 64-bit value.
      partitions[partNum].uniqueGUID.data1 = (uint64_t) rand() * (uint64_t) rand();
      partitions[partNum].uniqueGUID.data2 = (uint64_t) rand() * (uint64_t) rand();
      ChangeGPTType(&partitions[partNum]);
   } else {
      printf("No free sectors available\n");
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
      BlankPartition(&partitions[partNum - 1]);
   } else {
      printf("No partitions\n");
   } // if/else
} // GPTData::DeletePartition

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
         if ((first >= partitions[i].firstLBA) &&
             (first <= partitions[i].lastLBA)) { // in existing part.
            first = partitions[i].lastLBA + 1;
            firstMoved = 1;
          } // if
      } // for
   } while (firstMoved == 1);
   if (first > mainHeader.lastUsableLBA)
      first = 0;
   return (first);
} // GPTData::FindFirstAvailable()

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
         if ((last >= partitions[i].firstLBA) &&
             (last <= partitions[i].lastLBA)) { // in existing part.
            last = partitions[i].firstLBA - 1;
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
      if ((nearestStart > partitions[i].firstLBA) &&
          (partitions[i].firstLBA > start)) {
         nearestStart = partitions[i].firstLBA - 1;
      } // if
   } // for
   return (nearestStart);
} // GPTData::FindLastInFree()

// Returns 1 if sector is unallocated, 0 if it's allocated to a partition
int GPTData::IsFree(uint64_t sector) {
   int isFree = 1;
   uint32_t i;

   for (i = 0; i < mainHeader.numParts; i++) {
      if ((sector >= partitions[i].firstLBA) &&
          (sector <= partitions[i].lastLBA)) {
         isFree = 0;
      } // if
   } // for
   if ((sector < mainHeader.firstUsableLBA) || 
       (sector > mainHeader.lastUsableLBA)) {
      isFree = 0;
   } // if
   return (isFree);
} // GPTData::IsFree()

int GPTData::XFormPartitions(MBRData* origParts) {
   int i, j;
   int numToConvert;
   uint8_t origType;

   // Clear out old data & prepare basics....
   ClearGPTData();

   // Convert the smaller of the # of GPT or MBR partitions
   if (mainHeader.numParts > (NUM_LOGICALS + 4))
      numToConvert = NUM_LOGICALS + 4;
   else
      numToConvert = mainHeader.numParts;

//   printf("In XFormPartitions(), numToConvert = %d\n", numToConvert);

   for (i = 0; i < numToConvert; i++) {
      origType = origParts->GetType(i);
//      printf("Converting partition of type 0x%02X\n", (int) origType);

      // don't convert extended partitions or null (non-existent) partitions
      if ((origType != 0x05) && (origType != 0x0f) && (origType != 0x00)) {
         partitions[i].firstLBA = (uint64_t) origParts->GetFirstSector(i);
         partitions[i].lastLBA = partitions[i].firstLBA + (uint64_t)
                                 origParts->GetLength(i) - 1;
         partitions[i].partitionType = typeHelper.IDToGUID(((uint16_t) origType) * 0x0100);

         // Create random unique GUIDs for the partitions
         // rand() is only 32 bits, so multiply together to fill a 64-bit value
	 partitions[i].uniqueGUID.data1 = (uint64_t) rand() * (uint64_t) rand();
	 partitions[i].uniqueGUID.data2 = (uint64_t) rand() * (uint64_t) rand();
	 partitions[i].attributes = 0;
	 for (j = 0; j < NAME_SIZE; j++)
            partitions[i].name[j] = '\0';
      } // if
   } // for

   // Convert MBR into protective MBR
   protectiveMBR.MakeProtectiveMBR();

   // Record that all original CRCs were OK so as not to raise flags
   // when doing a disk verification
   mainCrcOk = secondCrcOk = mainPartsCrcOk = secondPartsCrcOk = 1;

   return (1);
} // XFormPartitions()

// Sort the GPT entries, eliminating gaps and making for a logical
// ordering. Relies on QuickSortGPT() for the bulk of the work
void GPTData::SortGPT(void) {
   int i, lastPart = 0;
   struct GPTPartition temp;

   // First, find the last partition with data, so as not to
   // spend needless time sorting empty entries....
   for (i = 0; i < GPT_SIZE; i++) {
      if (partitions[i].firstLBA > 0)
         lastPart = i;
   } // for

   // Now swap empties with the last partitions, to simplify the logic
   // in the Quicksort function....
   i = 0;
   while (i < lastPart) {
      if (partitions[i].firstLBA == 0) {
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

// Recursive quick sort algorithm for GPT partitions. Note that if there
// are any empties in the specified range, they'll be sorted to the
// start, resulting in a sorted set of partitions that begins with
// partition 2, 3, or higher.
void QuickSortGPT(struct GPTPartition* partitions, int start, int finish) {
   uint64_t starterValue; // starting location of median partition
   int left, right;
   struct GPTPartition temp;

   left = start;
   right = finish;
   starterValue = partitions[(start + finish) / 2].firstLBA;
   do {
      while (partitions[left].firstLBA < starterValue)
         left++;
      while (partitions[right].firstLBA > starterValue)
         right--;
      if (left <= right) {
         temp = partitions[left];
	 partitions[left] = partitions[right];
	 partitions[right] = temp;
	 left++;
	 right--;
      } // if
   } while (left <= right);
   if (start < right) QuickSortGPT(partitions, start, right);
   if (finish > left) QuickSortGPT(partitions, left, finish);
} // QuickSortGPT()

// Blank (delete) a single partition
void BlankPartition(struct GPTPartition* partition) {
   int j;

   partition->uniqueGUID.data1 = 0;
   partition->uniqueGUID.data2 = 0;
   partition->partitionType.data1 = 0;
   partition->partitionType.data2 = 0;
   partition->firstLBA = 0;
   partition->lastLBA = 0;
   partition->attributes = 0;
   for (j = 0; j < NAME_SIZE; j++)
      partition->name[j] = '\0';
} // BlankPartition

// Blank the partition array
void GPTData::BlankPartitions(void) {
   uint32_t i;

   for (i = 0; i < mainHeader.numParts; i++) {
      BlankPartition(&partitions[i]);
   } // for
} // GPTData::BlankPartitions()

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
   return (goOn);
} // GPTData::ClearGPTData()

// Returns 1 if the two partitions overlap, 0 if they don't
int TheyOverlap(struct GPTPartition* first, struct GPTPartition* second) {
   int theyDo = 0;

   // Don't bother checking unless these are defined (both start and end points
   // are 0 for undefined partitions, so just check the start points)
   if ((first->firstLBA != 0) && (second->firstLBA != 0)) {
      if ((first->firstLBA < second->lastLBA) && (first->lastLBA >= second->firstLBA))
         theyDo = 1;
      if ((second->firstLBA < first->lastLBA) && (second->lastLBA >= first->firstLBA))
         theyDo = 1;
   } // if
   return (theyDo);
} // Overlap()

// Change the type code on the specified partition.
// Note: The GPT CRCs must be recomputed after calling this function!
void ChangeGPTType(struct GPTPartition* part) {
   char typeName[255], line[255];
   uint16_t typeNum = 0xFFFF;
   PartTypes typeHelper;
   GUIDData newType;

   printf("Current type is '%s'\n", typeHelper.GUIDToName(part->partitionType, typeName));
   while ((!typeHelper.Valid(typeNum)) && (typeNum != 0)) {
      printf("Hex code (L to show codes, 0 to enter raw code): ");
      fgets(line, 255, stdin);
      sscanf(line, "%x", &typeNum);
      if (line[0] == 'L')
         typeHelper.ShowTypes();
   } // while
   if (typeNum != 0) // user entered a code, so convert it
      newType = typeHelper.IDToGUID(typeNum);
   else // user wants to enter the GUID directly, so do that
      newType = GetGUID();
   part->partitionType = newType;
   printf("Changed system type of partition to '%s'\n",
          typeHelper.GUIDToName(part->partitionType, typeName));
} // ChangeGPTType()

// Prompt user for a partition number, then change its type code
// using ChangeGPTType(struct GPTPartition*) function.
void GPTData::ChangePartType(void) {
   int partNum;
   uint32_t low, high;

   if (GetPartRange(&low, &high) > 0) {
      partNum = GetPartNum();
      ChangeGPTType(&partitions[partNum]);
   } else {
      printf("No partitions\n");
   } // if/else
} // GPTData::ChangePartType()

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

// Prompt user for attributes to change on the specified partition
// and change them.
void GPTData::SetAttributes(uint32_t partNum) {
   Attributes theAttr;

   theAttr.SetAttributes(partitions[partNum].attributes);
   theAttr.DisplayAttributes();
   theAttr.ChangeAttributes();
   partitions[partNum].attributes = theAttr.GetAttributes();
} // GPTData::SetAttributes()

// Set the name for a partition to theName, or prompt for a name if
// theName is a NULL pointer. Note that theName is a standard C-style
// string, although the GUID partition definition requires a UTF-16LE
// string. This function creates a simple-minded copy for this.
void GPTData::SetName(uint32_t partNum, char* theName) {
   char newName[NAME_SIZE]; // New name
   int i;

   // Blank out new name string, just to be on the safe side....
   for (i = 0; i < NAME_SIZE; i++)
      newName[i] = '\0';

   if (theName == NULL) { // No name specified, so get one from the user
      printf("Enter name: ");
      fgets(newName, NAME_SIZE / 2, stdin);

      // Input is likely to include a newline, so remove it....
      i = strlen(newName);
      if (newName[i - 1] == '\n')
         newName[i - 1] = '\0';
   } else {
      strcpy(newName, theName);
   } // if

   // Copy the C-style ASCII string from newName into a form that the GPT
   // table will accept....
   for (i = 0; i < NAME_SIZE; i++) {
      if ((i % 2) == 0) {
         partitions[partNum].name[i] = newName[(i / 2)];
      } else {
         partitions[partNum].name[i] = '\0';
      } // if/else
   } // for
} // GPTData::SetName()

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
      if (partitions[pn].firstLBA != UINT64_C(0)) {
	 partitions[pn].uniqueGUID = theGUID;
         retval = 1;
      } // if
   } // if
   return retval;
} // GPTData::SetPartitionGUID()

// Check the validity of the GPT header. Returns 1 if the main header
// is valid, 2 if the backup header is valid, 3 if both are valid, and
// 0 if neither is valid. Note that this function just checks the GPT
// signature and revision numbers, not CRCs or other data.
int GPTData::CheckHeaderValidity(void) {
   int valid = 3;

   if (mainHeader.signature != GPT_SIGNATURE) {
      valid -= 1;
      printf("Main GPT signature invalid; read 0x%016llX, should be\n0x%016llX\n",
             (unsigned long long) mainHeader.signature, (unsigned long long) GPT_SIGNATURE);
   } else if ((mainHeader.revision != 0x00010000) && valid) {
      valid -= 1;
      printf("Unsupported GPT version in main header; read 0x%08lX, should be\n0x%08lX\n",
             (unsigned long) mainHeader.revision, UINT32_C(0x00010000));
   } // if/else/if

   if (secondHeader.signature != GPT_SIGNATURE) {
      valid -= 2;
      printf("Secondary GPT signature invalid; read 0x%016llX, should be\n0x%016llX\n",
             (unsigned long long) secondHeader.signature, (unsigned long long) GPT_SIGNATURE);
   } else if ((secondHeader.revision != 0x00010000) && valid) {
      valid -= 2;
      printf("Unsupported GPT version in backup header; read 0x%08lX, should be\n0x%08lX\n",
             (unsigned long) mainHeader.revision, UINT32_C(0x00010000));
   } // if/else/if

   return valid;
} // GPTData::CheckHeaderValidity()

// Check the header CRC to see if it's OK...
int GPTData::CheckHeaderCRC(struct GPTHeader* header) {
   uint32_t oldCRC, newCRC;

   // Back up old header and then blank it, since it must be 0 for
   // computation to be valid
   oldCRC = header->headerCRC;
   header->headerCRC = UINT32_C(0);

   // Initialize CRC functions...
   chksum_crc32gentab();

   // Compute CRC, restore original, and return result of comparison
   newCRC = chksum_crc32((unsigned char*) header, HEADER_SIZE);
   mainHeader.headerCRC = oldCRC;
   return (oldCRC == newCRC);
} // GPTData::CheckHeaderCRC()

// Recompute all the CRCs. Must be called before saving if any changes
// have been made.
void GPTData::RecomputeCRCs(void) {
   uint32_t crc;

   // Initialize CRC functions...
   chksum_crc32gentab();

   // Compute CRC of partition tables & store in main and secondary headers
   crc = chksum_crc32((unsigned char*) partitions, mainHeader.numParts * GPT_SIZE);
   mainHeader.partitionEntriesCRC = crc;
   secondHeader.partitionEntriesCRC = crc;

   // Zero out GPT tables' own CRCs (required for correct computation)
   mainHeader.headerCRC = 0;
   secondHeader.headerCRC = 0;

   // Compute & store CRCs of main & secondary headers...
   crc = chksum_crc32((unsigned char*) &mainHeader, HEADER_SIZE);
   mainHeader.headerCRC = crc;
   crc = chksum_crc32((unsigned char*) &secondHeader, HEADER_SIZE);
   secondHeader.headerCRC = crc;
} // GPTData::RecomputeCRCs()

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
   for (i = 1; i < mainHeader.numParts; i++) {
      for (j = 0; j < i; j++) {
         if (TheyOverlap(&partitions[i], &partitions[j])) {
            problems++;
            printf("\nProblem: partitions %d and %d overlap:\n", i + 1, j + 1);
            printf("  Partition %d: %llu to %llu\n", i, 
                   (unsigned long long) partitions[i].firstLBA,
                   (unsigned long long) partitions[i].lastLBA);
            printf("  Partition %d: %llu to %llu\n", j,
                   (unsigned long long) partitions[j].firstLBA,
                   (unsigned long long) partitions[j].lastLBA);
	 } // if
      } // for j...
   } // for i...

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
} // RebuildSecondHeader()

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

/*
// Create a hybrid MBR -- an ugly, funky thing that helps GPT work with
// OSes that don't understand GPT.
void GPTData::MakeHybrid(void) {
   uint32_t partNums[3];
   char line[255];
   int numParts, i, j, typeCode, bootable;
   uint64_t length;

   // First, rebuild the protective MBR...
   protectiveMBR.MakeProtectiveMBR();

   // Now get the numbers of up to three partitions to add to the
   // hybrid MBR....
   printf("Type from one to three partition numbers to be added to the hybrid MBR, in\n"
          "sequence: ");
   fgets(line, 255, stdin);
   numParts = sscanf(line, "%d %d %d", &partNums[0], &partNums[1], &partNums[2]);
   for (i = 0; i < numParts; i++) {
      j = partNums[i] - 1;
      printf("Creating entry for partition #%d\n", j + 1);
      if ((j >= 0) && (j < mainHeader.numParts)) {
         if (partitions[j].lastLBA < UINT32_MAX) {
            printf("Enter an MBR hex code (suggested %02X): ",
                   typeHelper.GUIDToID(partitions[j].partitionType) / 256);
            fgets(line, 255, stdin);
	    sscanf(line, "%x", &typeCode);
	    printf("Set the bootable flag? ");
	    bootable = (GetYN() == 'Y');
            length = partitions[j].lastLBA - partitions[j].firstLBA + UINT64_C(1);
            protectiveMBR.MakePart(i + 1, (uint32_t) partitions[j].firstLBA,
                    (uint32_t) length, typeCode, bootable);
         } else { // partition out of range
            printf("Partition %d ends beyond the 2TiB limit of MBR partitions; omitting it.\n",
                   j + 1);
         } // if/else
      } else {
         printf("Partition %d is out of range; omitting it.\n", j + 1);
      } // if/else
   } // for
} // GPTData::MakeHybrid()
*/

// Writes GPT (and protective MBR) to disk. Returns 1 on successful
// write, 0 if there was a problem.
int GPTData::SaveGPTData(void) {
   int allOK = 1, i, j;
   char answer, line[256];
   int fd;
   uint64_t secondTable;
   off_t offset;

   if (strlen(device) == 0) {
      printf("Device not defined.\n");
   } // if

   // First do some final sanity checks....
   // Is there enough space to hold the GPT headers and partition tables,
   // given the partition sizes?
   if (CheckGPTSize() == 0) {
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
   for (i = 1; i < mainHeader.numParts; i++) {
      for (j = 0; j < i; j++) {
         if (TheyOverlap(&partitions[i], &partitions[j])) {
            fprintf(stderr, "\Error: partitions %d and %d overlap:\n", i + 1, j + 1);
            fprintf(stderr, "  Partition %d: %llu to %llu\n", i, 
                   (unsigned long long) partitions[i].firstLBA,
                   (unsigned long long) partitions[i].lastLBA);
            fprintf(stderr, "  Partition %d: %llu to %llu\n", j,
                   (unsigned long long) partitions[j].firstLBA,
                   (unsigned long long) partitions[j].lastLBA);
            fprintf(stderr, "Aborting write operation!\n");
	    allOK = 0;
	 } // if
      } // for j...
   } // for i...

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
      fd = open(device, O_WRONLY); // try to open the device; may fail....
#ifdef __APPLE__
      // MacOS X requires a shared lock under some circumstances....
      if (fd < 0) {
         fd = open(device, O_WRONLY|O_SHLOCK);
      } // if
#endif
      if (fd != -1) {
         // First, write the protective MBR...
	 protectiveMBR.WriteMBRData(fd);

         // Now write the main GPT header...
         if (allOK)
            if (write(fd, &mainHeader, 512) == -1)
              allOK = 0;

         // Now write the main partition tables...
	 if (allOK) {
	    if (write(fd, partitions, GPT_SIZE * mainHeader.numParts) == -1)
               allOK = 0;
         } // if

         // Now seek to near the end to write the secondary GPT....
	 if (allOK) {
            secondTable = secondHeader.partitionEntriesLBA;
            offset = (off_t) secondTable * (off_t) (blockSize);
            if (lseek64(fd, offset, SEEK_SET) == (off_t) - 1) {
               allOK = 0;
               printf("Unable to seek to end of disk!\n");
            } // if
         } // if

         // Now write the secondary partition tables....
	 if (allOK)
            if (write(fd, partitions, GPT_SIZE * mainHeader.numParts) == -1)
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
	    sleep(2);
            i = ioctl(fd, BLKRRPART);
            if (i)
               printf("Warning: The kernel is still using the old partition table.\n"
                      "The new table will be used at the next reboot.\n");
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
         fprintf(stderr, "Unable to open device %s for writing! Errno is %d! Aborting!\n", device, errno);
	 allOK = 0;
      } // if/else
   } else {
      printf("Aborting write of new partition table.\n");
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
   int fd, allOK = 1;;

   if ((fd = open(filename, O_WRONLY | O_CREAT, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH)) != -1) {
      // First, write the protective MBR...
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
         if (write(fd, partitions, GPT_SIZE * mainHeader.numParts) == -1)
            allOK = 0;
      } // if

      if (allOK) { // writes completed OK
         printf("The operation has completed successfully.\n");
      } else {
         printf("Warning! An error was reported when writing the backup file.\n");
         printf("It may not be useable!\n");
      } // if/else
      close(fd);
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

   if ((fd = open(filename, O_RDONLY)) != -1) {
      // Let the MBRData class load the saved MBR...
      protectiveMBR.ReadMBRData(fd);

      // Load the main GPT header, check its vaility, and set the GPT
      // size based on the data
      read(fd, &mainHeader, 512);
      mainCrcOk = CheckHeaderCRC(&mainHeader);

      // Load the backup GPT header in much the same way as the main
      // GPT header....
      read(fd, &secondHeader, 512);
      secondCrcOk = CheckHeaderCRC(&secondHeader);

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

// Check to be sure that data type sizes are correct. The basic types (uint*_t) should
// never fail these tests, but the struct types may fail depending on compile options.
// Specifically, the -fpack-struct option to gcc may be required to ensure proper structure
// sizes.
int SizesOK(void) {
   int allOK = 1;
   union {
      uint32_t num;
      unsigned char uc[sizeof(uint32_t)];
   } endian;

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
      fprintf(stderr, "MBRRecord is %d bytes, should be 16 bytes; aborting!\n", sizeof(uint32_t));
      allOK = 0;
   } // if
   if (sizeof(struct EBRRecord) != 512) {
      fprintf(stderr, "EBRRecord is %d bytes, should be 512 bytes; aborting!\n", sizeof(uint32_t));
      allOK = 0;
   } // if
   if (sizeof(struct GPTHeader) != 512) {
      fprintf(stderr, "GPTHeader is %d bytes, should be 512 bytes; aborting!\n", sizeof(uint32_t));
      allOK = 0;
   } // if
   // Determine endianness; set allOK = 0 if running on big-endian hardware
   endian.num = 1;
   if (endian.uc[0] != (unsigned char) 1) {
      fprintf(stderr, "Running on big-endian hardware, but this program only works on little-endian\n"
                      "systems; aborting!\n");
      allOK = 0;
   } // if
   return (allOK);
} // SizesOK()

