/* This program is copyright (c) 2009 by Roderick W. Smith. It is distributed
  under the terms of the GNU GPL version 2, as detailed in the COPYING file. */

#include <stdint.h>
#include <stdlib.h>
#include <string>

#ifndef __GPTSUPPORT
#define __GPTSUPPORT

#if defined (__FreeBSD__) || defined (__FreeBSD_kernel__) || defined (__APPLE__)
// Darwin (Mac OS) only: disk IOCTLs are different, and there is no lseek64
// This used to use __DARWIN_UNIX03 rather than __APPLE__, but __APPLE__
// is more general. If the code fails to work on older versions of OS X/
// Darwin, this may need to be changed back (and in various .cc files).
#include <sys/disk.h>
#define lseek64 lseek
#endif

// Linux only....
#ifdef __linux__
#include <linux/fs.h>
#endif

// Set this as a default
#define SECTOR_SIZE UINT32_C(512)

// Signatures for Apple (APM) disks, multiplied by 0x100000000
#define APM_SIGNATURE1 UINT64_C(0x00004D5000000000)
#define APM_SIGNATURE2 UINT64_C(0x0000535400000000)

// Maximum line length ignored on some input functions
#define MAX_IGNORED 999

/**************************
 * Some GPT constants.... *
 **************************/

#define GPT_SIGNATURE UINT64_C(0x5452415020494645)

// Number and size of GPT entries...
#define NUM_GPT_ENTRIES 128
#define GPT_SIZE 128
#define HEADER_SIZE UINT32_C(92)
#define GPT_RESERVED 420
#define NAME_SIZE 72

using namespace std;

int GetNumber(int low, int high, int def, const string & prompt);
char GetYN(void);
uint64_t GetSectorNum(uint64_t low, uint64_t high, uint64_t def, uint64_t sSize, const std::string& prompt);
uint64_t SIToInt(string SIValue, uint64_t sSize, uint64_t low, uint64_t high, uint64_t def = 0);
string BytesToSI(uint64_t size, uint32_t sectorSize = 1);
unsigned char StrToHex(const string & input, unsigned int position);
int IsHex(const string & input); // Returns 1 if input can be hexadecimal number....
int IsLittleEndian(void); // Returns 1 if CPU is little-endian, 0 if it's big-endian
void ReverseBytes(void* theValue, int numBytes); // Reverses byte-order of theValue

// Extract colon-separated fields from a string....
uint64_t GetInt(const string & argument, int itemNum);
string GetString(const string & Info, int itemNum);

#endif
