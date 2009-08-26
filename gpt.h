/* gpt.h -- GPT and data structure definitions, types, and
   functions */

#include <stdint.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include "support.h"
#include "parttypes.h"
#include "mbr.h"

#ifndef __GPTSTRUCTS
#define __GPTSTRUCTS

#define GPT_SIGNATURE UINT64_C(0x5452415020494645)
// Signatures for Apple (APM) disks, multiplied by 0x100000000
#define APM_SIGNATURE1 UINT64_C(0x00004D5000000000)
#define APM_SIGNATURE2 UINT64_C(0x0000535400000000)

/* Number and size of GPT entries... */
#define NUM_GPT_ENTRIES 128
#define GPT_SIZE 128
/* Offset, in 512-byte sectors, for GPT table and partition data.
   Note this is above two multiplied together, divided by 512, with 2
   added 
#define GPT_OFFSET (((NUM_GPT_ENTRIES * GPT_SIZE) / SECTOR_SIZE) + 2)
*/

#define HEADER_SIZE 92

#define GPT_RESERVED 420
#define NAME_SIZE 72

using namespace std;

/****************************************
 *                                      *
 * GPTData class and related structures *
 *                                      *
 ****************************************/

// Validity state of GPT data
enum GPTValidity {gpt_valid, gpt_corrupt, gpt_invalid};

// Which set of partition data to use
enum WhichToUse {use_gpt, use_mbr, use_new};

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

struct GPTPartition {
   struct GUIDData partitionType;
   struct GUIDData uniqueGUID;
   uint64_t firstLBA;
   uint64_t lastLBA;
   uint64_t attributes;
   unsigned char name[NAME_SIZE];
}; // struct GPTPartition

// Data in GPT format
class GPTData {
protected:
   struct GPTHeader mainHeader;
   struct GPTPartition *partitions;
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
//   uint32_t units; // display units, in multiples of sectors
   PartTypes typeHelper;
public:
   GPTData(void);
   GPTData(char* deviceFilename);
   ~GPTData(void);
   int SetGPTSize(uint32_t numEntries);
   int CheckGPTSize(void);
   int LoadPartitions(char* deviceFilename);
   int ForceLoadGPTData(int fd);
   int LoadMainTable(void);
   WhichToUse UseWhichPartitions(void);
   void ResizePartitionTable(void);
   int GetPartRange(uint32_t* low, uint32_t* high);
   void DisplayGPTData(void);
   void DisplayMBRData(void) {protectiveMBR.DisplayMBRData();}
   void ShowDetails(void);
   void ShowPartDetails(uint32_t partNum);
   void CreatePartition(void);
   void DeletePartition(void);
   void BlankPartitions(void);
   uint64_t FindFirstAvailable(uint64_t start = 0);
   uint64_t FindLastAvailable(uint64_t start);
   uint64_t FindLastInFree(uint64_t start);
   int IsFree(uint64_t sector);
   int XFormPartitions(MBRData* origParts);
   void SortGPT(void);
   int ClearGPTData(void);
   void ChangePartType(void);
   uint32_t GetPartNum(void);
   void SetAttributes(uint32_t partNum);
   void SetName(uint32_t partNum, char* theName = NULL);
   void SetDiskGUID(GUIDData newGUID);
   int SetPartitionGUID(uint32_t pn, GUIDData theGUID);
   int CheckHeaderValidity(void);
   int CheckHeaderCRC(struct GPTHeader* header);
   void RecomputeCRCs(void);
   int Verify(void);
   void RebuildMainHeader(void);
   void RebuildSecondHeader(void);
   void LoadSecondTableAsMain(void);
   uint64_t FindFreeBlocks(int *numSegments, uint64_t *largestSegment);
   void MakeHybrid(void);
   void MakeProtectiveMBR(void);
   int SaveGPTData(void);
   int SaveGPTBackup(char* filename);
   int LoadGPTBackup(char* filename);
   int DestroyGPT(void); // Returns 1 if user proceeds
   void ReverseHeaderBytes(struct GPTHeader* header); // for endianness
   void ReversePartitionBytes(); // for endianness

   // Return data about the GPT structures....
   uint32_t GetNumParts(void) {return mainHeader.numParts;}
   uint64_t GetMainHeaderLBA(void) {return mainHeader.currentLBA;}
   uint64_t GetSecondHeaderLBA(void) {return secondHeader.currentLBA;}
   uint64_t GetMainPartsLBA(void) {return mainHeader.partitionEntriesLBA;}
   uint64_t GetSecondPartsLBA(void) {return secondHeader.partitionEntriesLBA;}
   uint64_t GetBlocksInPartTable(void) {return (mainHeader.numParts *
                   mainHeader.sizeOfPartitionEntries) / blockSize;}
}; // class GPTData

// Function prototypes....
void BlankPartition(struct GPTPartition* partition);
//int XFormType(uint8_t oldType, struct GUIDData* newType, int partNum);
void QuickSortGPT(struct GPTPartition* partitions, int start, int finish);
int TheyOverlap(struct GPTPartition* first, struct GPTPartition* second);
void ChangeGPTType(struct GPTPartition* part);
int SizesOK(void);

#endif
