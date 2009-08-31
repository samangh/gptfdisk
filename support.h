/* This program is copyright (c) 2009 by Roderick W. Smith. It is distributed
  under the terms of the GNU GPL version 2, as detailed in the COPYING file. */

#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>

#if defined (__FreeBSD__) || defined (__APPLE__)
// Darwin (Mac OS) only: disk IOCTLs are different, and there is no lseek64
// This used to use __DARWIN_UNIX03 rather than __APPLE__, but __APPLE__
// is more general. If the code fails to work on older versions of OS X/
// Darwin, this may need to be changed back (and in various .cc files).
#include <sys/disk.h>
#define lseek64 lseek
#else

// Linux only....
#include <linux/fs.h>
#endif

#include <string>

#ifndef __GPTSUPPORT
#define __GPTSUPPORT

// Set this as a default
#define SECTOR_SIZE UINT32_C(512)

// Signatures for Apple (APM) disks, multiplied by 0x100000000
#define APM_SIGNATURE1 UINT64_C(0x00004D5000000000)
#define APM_SIGNATURE2 UINT64_C(0x0000535400000000)

/**************************
 * Some GPT constants.... *
 **************************/

#define GPT_SIGNATURE UINT64_C(0x5452415020494645)

// Number and size of GPT entries...
#define NUM_GPT_ENTRIES 128
#define GPT_SIZE 128
#define HEADER_SIZE 92
#define GPT_RESERVED 420
#define NAME_SIZE 72

using namespace std;

// a GUID
struct GUIDData {
   uint64_t data1;
   uint64_t data2;
}; // struct GUIDData

int GetNumber(int low, int high, int def, const char prompt[]);
char GetYN(void);
uint64_t GetSectorNum(uint64_t low, uint64_t high, uint64_t def, char prompt[]);
char* BytesToSI(uint64_t size, char theValue[]);
int GetBlockSize(int fd);
char* GUIDToStr(struct GUIDData theGUID, char* theString);
GUIDData GetGUID(void);
int IsLittleEndian(void); // Returns 1 if CPU is little-endian, 0 if it's big-endian
void ReverseBytes(void* theValue, int numBytes); // Reverses byte-order of theValue
uint64_t PowerOf2(int value);
int OpenForWrite(char* deviceFilename);

uint64_t disksize(int fd, int* err);

#endif
