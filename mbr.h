/* mbr.h -- MBR data structure definitions, types, and functions */

/* This program is copyright (c) 2009 by Roderick W. Smith. It is distributed
  under the terms of the GNU GPL version 2, as detailed in the COPYING file. */

#include <stdint.h>
#include <sys/types.h>
#include "gptpart.h"
#include "diskio.h"

#ifndef __MBRSTRUCTS
#define __MBRSTRUCTS

#define MBR_SIGNATURE UINT16_C(0xAA55)
#define MAX_HEADS 255        /* numbered 0 - 254 */
#define MAX_SECSPERTRACK 63  /* numbered 1 - 63 */
#define MAX_CYLINDERS 1024   /* numbered 0 - 1023 */

// Maximum number of MBR partitions
#define MAX_MBR_PARTS 128

using namespace std;

/****************************************
 *                                      *
 * MBRData class and related structures *
 *                                      *
 ****************************************/

// Data for a single MBR partition record
// Note that firstSector and lastSector are in CHS addressing, which
// splits the bits up in a weird way.
#pragma pack(1)
struct MBRRecord {
   uint8_t status;
   uint8_t firstSector[3];
   uint8_t partitionType;
   uint8_t lastSector[3];
   uint32_t firstLBA;
   uint32_t lengthLBA;
}; // struct MBRRecord

// A 512-byte data structure into which the MBR can be loaded in one
// go. Also used when loading logical partitions.
#pragma pack(1)
struct TempMBR {
   uint8_t code[440];
   uint32_t diskSignature;
   uint16_t nulls;
   struct MBRRecord partitions[4];
   uint16_t MBRSignature;
}; // struct TempMBR

// Possible states of the MBR
enum MBRValidity {invalid, gpt, hybrid, mbr};

// Full data in tweaked MBR format
class MBRData {
protected:
   uint8_t code[440];
   uint32_t diskSignature;
   uint16_t nulls;
   // MAX_MBR_PARTS defaults to 128. This array holds both the primary and
   // the logical partitions, to simplify data retrieval for GPT conversions.
   struct MBRRecord partitions[MAX_MBR_PARTS];
   uint16_t MBRSignature;

   // Above are basic MBR data; now add more stuff....
   uint32_t blockSize; // block size (usually 512)
   uint64_t diskSize; // size in blocks
   uint64_t numHeads; // number of heads, in CHS scheme
   uint64_t numSecspTrack; // number of sectors per track, in CHS scheme
   DiskIO* myDisk;
   string device;
   MBRValidity state;
   struct MBRRecord* GetPartition(int i); // Return primary or logical partition
public:
   MBRData(void);
   MBRData(string deviceFilename);
   ~MBRData(void);

   // File I/O functions...
   int ReadMBRData(const string & deviceFilename);
   void ReadMBRData(DiskIO * theDisk, int checkBlockSize = 1);
   // ReadLogicalPart() returns last partition # read to logicals[] array,
   // or -1 if there was a problem....
   int ReadLogicalPart(uint32_t extendedStart, uint32_t diskOffset,
                       int partNum);
   int WriteMBRData(void);
   int WriteMBRData(DiskIO *theDisk);
   int WriteMBRData(const string & deviceFilename);

   // Display data for user...
   void DisplayMBRData(void);
   void ShowState(void);

   // Functions that set or get disk metadata (size, CHS geometry, etc.)
   void SetDiskSize(uint64_t ds) {diskSize = ds;}
   MBRValidity GetValidity(void) {return state;}
   void SetHybrid(void) {state = hybrid;} // Set hybrid flag
   void SetCHSGeom(uint32_t h, uint32_t s);
   int LBAtoCHS(uint64_t lba, uint8_t * chs); // Convert LBA to CHS

   // Functions to create, delete, or change partitions
   // Pass EmptyMBR 1 to clear the boot loader code, 0 to leave it intact
   void EmptyMBR(int clearBootloader = 1);
   void MakeProtectiveMBR(int clearBoot = 0);
   void MakePart(int num, uint32_t startLBA, uint32_t lengthLBA, int type = 0x07,
                 int bootable = 0);
   int MakeBiggestPart(int i, int type); // Make partition filling most space
   void DeletePartition(int i);
   int DeleteByLocation(uint64_t start64, uint64_t length64);
   void OptimizeEESize(void);

   // Functions to find information on free space....
   uint32_t FindFirstAvailable(uint32_t start = 1);
   uint32_t FindLastInFree(uint32_t start);
   uint32_t FindFirstInFree(uint32_t start);
   int IsFree(uint32_t sector);

   // Functions to extract data on specific partitions....
   uint8_t GetStatus(int i);
   uint8_t GetType(int i);
   uint32_t GetFirstSector(int i);
   uint32_t GetLength(int i);
   GPTPart AsGPT(int i);
}; // struct MBRData

#endif
