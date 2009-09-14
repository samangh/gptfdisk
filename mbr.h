/* mbr.h -- MBR data structure definitions, types, and functions */

/* This program is copyright (c) 2009 by Roderick W. Smith. It is distributed
  under the terms of the GNU GPL version 2, as detailed in the COPYING file. */

#include <stdint.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include "gptpart.h"

#ifndef __MBRSTRUCTS
#define __MBRSTRUCTS

#define MBR_SIGNATURE UINT16_C(0xAA55)

// Maximum number of logical partitions supported
#define NUM_LOGICALS 124

using namespace std;

/****************************************
 *                                      *
 * MBRData class and related structures *
 *                                      *
 ****************************************/

// Data for a single MBR partition record
// Note that firstSector and lastSector are in CHS addressing, which
// splits the bits up in a weird way.
struct MBRRecord {
   uint8_t status;
   uint8_t firstSector[3];
   uint8_t partitionType;
   uint8_t lastSector[3];
   uint32_t firstLBA;
   uint32_t lengthLBA;
}; // struct MBRRecord

// Create a 512-byte data structure into which the MBR can be loaded in one
// go, for the benefit of FreeBSD which seems to flake out when loading
// from block devices in multiples other than the block size....
struct TempMBR {
   uint8_t code[440];
   uint32_t diskSignature;
   uint16_t nulls;
   struct MBRRecord partitions[4];
   uint16_t MBRSignature;
}; // struct TempMBR

// Extended Boot Record (EBR) data, used to hold one logical partition's
// data within an extended partition. Includes pointer to next record for
// in-memory linked-list access. This is similar to MBRData, but with a
// few tweaks....
struct EBRRecord {
   uint8_t code[446]; // generally 0s (and we don't care if they aren't)
   // First partition entry defines partition; second points to next
   // entry in on-disk linked list; remaining two are unused. Note that
   // addresses are relative to the extended partition, not to the disk
   // as a whole.
   struct MBRRecord partitions[4];
   uint16_t MBRSignature;
}; // struct EBRRecord

// Possible states of the MBR
enum MBRValidity {invalid, gpt, hybrid, mbr};

// Full data in tweaked MBR format
class MBRData {
protected:
   uint8_t code[440];
   uint32_t diskSignature;
   uint16_t nulls;
   struct MBRRecord partitions[4];
   uint16_t MBRSignature;

   // Above are basic MBR data; now add more stuff....
   uint32_t blockSize; // block size (usually 512)
   uint64_t diskSize; // size in blocks
   char device[256];
   // Now an array of partitions for the logicals, in array form (easier
   // than a linked list, and good enough for the GPT itself, so....)
   struct MBRRecord logicals[NUM_LOGICALS];
   MBRValidity state;
   struct MBRRecord* GetPartition(int i); // Return primary or logical partition
public:
   MBRData(void);
   MBRData(char* deviceFilename);
   ~MBRData(void);
   // Pass EmptyMBR 1 to clear the boot loader code, 0 to leave it intact
   void EmptyMBR(int clearBootloader = 1);
   void SetDiskSize(uint64_t ds) {diskSize = ds;}
   int ReadMBRData(char* deviceFilename);
   void ReadMBRData(int fd, int checkBlockSize = 1);
   int WriteMBRData(void);
   void WriteMBRData(int fd);
   // ReadLogicalPart() returns last partition # read to logicals[] array,
   // or -1 if there was a problem....
   int ReadLogicalPart(int fd, uint32_t extendedStart, uint32_t diskOffset,
                       int partNum);
   void DisplayMBRData(void);
   void MakeProtectiveMBR(int clearBoot = 0);
   MBRValidity GetValidity(void) {return state;}
   void ShowValidity(void);
   void ShowState(void);
   void MakePart(int num, uint32_t startLBA, uint32_t lengthLBA, int type = 0x07,
                 int bootable = 0);
   int MakeBiggestPart(int i, int type); // Make partition filling most space
   void DeletePartition(int i);
   int DeleteByLocation(uint64_t start64, uint64_t length64);
   void OptimizeEESize(void);
   void SetHybrid(void) {state = hybrid;} // Set hybrid flag

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
