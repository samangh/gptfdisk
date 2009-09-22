/* gpt.h -- GPT and data structure definitions, types, and
   functions */

/* This program is copyright (c) 2009 by Roderick W. Smith. It is distributed
  under the terms of the GNU GPL version 2, as detailed in the COPYING file. */

#include <stdint.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include "support.h"
#include "parttypes.h"
#include "mbr.h"
#include "bsd.h"
#include "gptpart.h"

#ifndef __GPTSTRUCTS
#define __GPTSTRUCTS


using namespace std;

/****************************************
 *                                      *
 * GPTData class and related structures *
 *                                      *
 ****************************************/

// Validity state of GPT data
enum GPTValidity {gpt_valid, gpt_corrupt, gpt_invalid};

// Which set of partition data to use
enum WhichToUse {use_gpt, use_mbr, use_bsd, use_new};

// Header (first 512 bytes) of GPT table
struct GPTHeader {
   uint64_t signature;
   uint32_t revision;
   uint32_t headerSize;
   uint32_t headerCRC;
   uint32_t reserved;
   uint64_t currentLBA;
   uint64_t backupLBA;
   uint64_t firstUsableLBA;
   uint64_t lastUsableLBA;
   struct GUIDData diskGUID;
   uint64_t partitionEntriesLBA;
   uint32_t numParts;
   uint32_t sizeOfPartitionEntries;
   uint32_t partitionEntriesCRC;
   unsigned char reserved2[GPT_RESERVED];
}; // struct GPTHeader

// Data in GPT format
class GPTData {
protected:
   struct GPTHeader mainHeader;
   struct GPTPart *partitions;
   struct GPTHeader secondHeader;
   MBRData protectiveMBR;
   char device[256]; // device filename
   uint32_t blockSize; // device block size
   uint64_t diskSize; // size of device, in blocks
   GPTValidity state; // is GPT valid?
   int mainCrcOk;
   int secondCrcOk;
   int mainPartsCrcOk;
   int secondPartsCrcOk;
   int apmFound; // set to 1 if APM detected
   int bsdFound; // set to 1 if BSD disklabel detected in MBR
   PartTypes typeHelper;
public:
   // Basic necessary functions....
   GPTData(void);
   GPTData(char* deviceFilename);
   ~GPTData(void);

   // Verify (or update) data integrity
   int Verify(void);
   int CheckGPTSize(void);
   int CheckHeaderValidity(void);
   int CheckHeaderCRC(struct GPTHeader* header);
   void RecomputeCRCs(void);
   void RebuildMainHeader(void);
   void RebuildSecondHeader(void);
   int FindHybridMismatches(void);
   int FindOverlaps(void);

   // Load or save data from/to disk
   int LoadMBR(char* f) {return protectiveMBR.ReadMBRData(f);}
   void PartitionScan(int fd);
   int LoadPartitions(char* deviceFilename);
   int ForceLoadGPTData(int fd);
   int LoadMainTable(void);
   void LoadSecondTableAsMain(void);
   int SaveGPTData(void);
   int SaveGPTBackup(char* filename);
   int LoadGPTBackup(char* filename);

   // Display data....
   void ShowAPMState(void);
   void ShowGPTState(void);
   void DisplayGPTData(void);
   void DisplayMBRData(void) {protectiveMBR.DisplayMBRData();}
   void ShowDetails(void);
   void ShowPartDetails(uint32_t partNum);

   // Request information from the user (& possibly do something with it)
   uint32_t GetPartNum(void);
   void ResizePartitionTable(void);
   void CreatePartition(void);
   void DeletePartition(void);
   void ChangePartType(void);
   void SetAttributes(uint32_t partNum);
   int DestroyGPT(int prompt = 1); // Returns 1 if user proceeds

   // Convert between GPT and other formats (may require user interaction)
   WhichToUse UseWhichPartitions(void);
   int XFormPartitions(void);
   int XFormDisklabel(int OnGptPart = -1);
   int XFormDisklabel(BSDData* disklabel, int startPart);
   int OnePartToMBR(uint32_t gptPart, int mbrPart); // add one partition to MBR. Returns 1 if successful
   int XFormToMBR(void); // convert GPT to MBR, wiping GPT afterwards. Returns 1 if successful
   void MakeHybrid(void);

   // Adjust GPT structures WITHOUT user interaction...
   int SetGPTSize(uint32_t numEntries);
   void BlankPartitions(void);
   void SortGPT(void);
   int ClearGPTData(void);
   void SetName(uint32_t partNum, char* theName = NULL);
   void SetDiskGUID(GUIDData newGUID);
   int SetPartitionGUID(uint32_t pn, GUIDData theGUID);
   void MakeProtectiveMBR(void) {protectiveMBR.MakeProtectiveMBR();}

   // Return data about the GPT structures....
   int GetPartRange(uint32_t* low, uint32_t* high);
   uint32_t GetNumParts(void) {return mainHeader.numParts;}
   uint64_t GetMainHeaderLBA(void) {return mainHeader.currentLBA;}
   uint64_t GetSecondHeaderLBA(void) {return secondHeader.currentLBA;}
   uint64_t GetMainPartsLBA(void) {return mainHeader.partitionEntriesLBA;}
   uint64_t GetSecondPartsLBA(void) {return secondHeader.partitionEntriesLBA;}
   uint64_t GetBlocksInPartTable(void) {return (mainHeader.numParts *
                   mainHeader.sizeOfPartitionEntries) / blockSize;}
   uint32_t CountParts(void);


   // Find information about free space
   uint64_t FindFirstAvailable(uint64_t start = 0);
   uint64_t FindFirstInLargest(void);
   uint64_t FindLastAvailable(uint64_t start);
   uint64_t FindLastInFree(uint64_t start);
   uint64_t FindFreeBlocks(int *numSegments, uint64_t *largestSegment);
   int IsFree(uint64_t sector);

   // Endianness functions
   void ReverseHeaderBytes(struct GPTHeader* header); // for endianness
   void ReversePartitionBytes(); // for endianness
}; // class GPTData

// Function prototypes....
int SizesOK(void);

#endif
