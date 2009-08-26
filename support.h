#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>

#ifdef __APPLE__
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

using namespace std;

// a GUID
struct GUIDData {
   uint64_t data1;
   uint64_t data2;
}; // struct GUIDData

int GetNumber(int low, int high, int def, const char prompt[]);
char GetYN(void);
uint64_t GetLastSector(uint64_t low, uint64_t high, char prompt[]);
char* BytesToSI(uint64_t size, char theValue[]);
int GetBlockSize(int fd);
char* GUIDToStr(struct GUIDData theGUID, char* theString);
GUIDData GetGUID(void);
int IsLittleEndian(void); // Returns 1 if CPU is little-endian, 0 if it's big-endian
void ReverseBytes(char* theValue, int numBytes); // Reverses byte-order of theValue
uint64_t PowerOf2(int value);

uint64_t disksize(int fd, int* err);

#endif
