/* bsd.h -- BSD disklabel data structure definitions, types, and functions */

/* This program is copyright (c) 2009 by Roderick W. Smith. It is distributed
  under the terms of the GNU GPL version 2, as detailed in the COPYING file. */

#include <stdint.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include "gptpart.h"

#ifndef __BSD_STRUCTS
#define __BSD_STRUCTS

#define BSD_SIGNATURE UINT32_C(0x82564557)  /* BSD disklabel signature ("magic") */

#define LABEL_OFFSET1 64    /* BSD disklabels can start at any of these three */
#define LABEL_OFFSET2 512   /* values; check all for valid signatures */
#define LABEL_OFFSET3 2048

// FreeBSD documents a maximum # of partitions of 8, but I saw 16 on a NetBSD
// disk. I'm quadrupling that for further safety. Note that BSDReadData()
// uses a 4096-byte I/O buffer. In combination with LABEL_OFFSET3 and the
// additional 148-byte offset to the actual partition data, that gives a
// theoretical maximum of 118.75 partitions that the program can handle before
// memory errors will occur.
#define MAX_BSD_PARTS 64


using namespace std;

/****************************************
 *                                      *
 * BSDData class and related structures *
 *                                      *
 ****************************************/

// Possible states of the MBR
enum BSDValidity {unknown, bsd_invalid, bsd};

// Data for a single BSD partition record
// Create entries for all fields, although we only use lengthLBA, firstLBA,
// and fsType, to simplify loading the data from disk....
struct  BSDRecord {      // the partition table
   uint32_t lengthLBA;   // number of sectors in partition
   uint32_t firstLBA;    // starting sector
   uint32_t fragSize;    // filesystem basic fragment size
   uint8_t  fsType;      // filesystem type, see below
   uint8_t  frag;        // filesystem fragments per block
   uint16_t pcpg;        // filesystem cylinders per group
};

// Full data in tweaked BSD format
class BSDData {
   protected:
      // We only need a few items from the main BSD disklabel data structure....
      uint32_t signature;        // the magic number
      uint32_t sectorSize;       // # of bytes per sector
      uint32_t signature2;       // the magic number (again)
      uint16_t numParts;         // number of partitions in table
      BSDRecord* partitions;     // partition array

      // Above are basic BSD disklabel data; now add more stuff....
      uint64_t labelFirstLBA;    // first sector of BSD disklabel (partition or disk)
      uint64_t labelLastLBA;     // final sector of BSD disklabel
      uint64_t labelStart;       // BSD disklabel start point in bytes from labelFirstLBA
      BSDValidity state;
   public:
      BSDData(void);
      ~BSDData(void);
      int ReadBSDData(char* deviceFilename, uint64_t startSector, uint64_t endSector);
      void ReadBSDData(int fd, uint64_t startSector, uint64_t endSector);
      void ReverseMetaBytes(void);
      void DisplayBSDData(void);
      int ShowState(void); // returns 1 if BSD disklabel detected
      int IsDisklabel(void) {return (state == bsd);}

      // Functions to extract data on specific partitions....
      uint8_t GetType(int i);
      uint64_t GetFirstSector(int i);
      uint64_t GetLength(int i);
      int GetNumParts(void);
      GPTPart AsGPT(int i); // Return BSD part. as GPT part.
}; // struct MBRData

#endif
