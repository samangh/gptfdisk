// support.cc
// Non-class support functions for gdisk program.
// Primarily by Rod Smith, February 2009, but with a few functions
// copied from other sources (see attributions below).

/* This program is copyright (c) 2009 by Roderick W. Smith. It is distributed
  under the terms of the GNU GPL version 2, as detailed in the COPYING file. */

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "support.h"

#include <sys/types.h>

using namespace std;

// Get a numeric value from the user, between low and high (inclusive).
// Keeps looping until the user enters a value within that range.
// If user provides no input, def (default value) is returned.
// (If def is outside of the low-high range, an explicit response
// is required.)
int GetNumber(int low, int high, int def, const char prompt[]) {
   int response, num;
   char line[255];

   if (low != high) { // bother only if low and high differ...
      response = low - 1; // force one loop by setting response outside range
      while ((response < low) || (response > high)) {
         printf(prompt);
         fgets(line, 255, stdin);
         num = sscanf(line, "%d", &response);
         if (num == 1) { // user provided a response
            if ((response < low) || (response > high))
               printf("Value out of range\n");
         } else { // user hit enter; return default
            response = def;
         } // if/else
      } // while
   } else { // low == high, so return this value
      printf("Using %d\n", low);
      response = low;
   } // else
   return (response);
} // GetNumber()

// Gets a Y/N response (and converts lowercase to uppercase)
char GetYN(void) {
   char line[255];
   char response = '\0';

   while ((response != 'Y') && (response != 'N')) {
      printf("(Y/N): ");
      fgets(line, 255, stdin);
      sscanf(line, "%c", &response);
      if (response == 'y') response = 'Y';
      if (response == 'n') response = 'N';
   } // while
   return response;
} // GetYN(void)

// Obtains a sector number, between low and high, from the
// user, accepting values prefixed by "+" to add sectors to low,
// or the same with "K", "M", "G", or "T" as suffixes to add
// kilobytes, megabytes, gigabytes, or terabytes, respectively.
// If a "-" prefix is used, use the high value minus the user-
// specified number of sectors (or KiB, MiB, etc.). Use the def
 //value as the default if the user just hits Enter
uint64_t GetSectorNum(uint64_t low, uint64_t high, uint64_t def, char prompt[]) {
   unsigned long long response;
   int num;
   int plusFlag = 0;
   uint64_t mult = 1;
   char suffix;
   char line[255];

   response = low - 1; // Ensure one pass by setting a too-low initial value
   while ((response < low) || (response > high)) {
      printf(prompt);
      fgets(line, 255, stdin);

      // Remove leading spaces, if present
      while (line[0] == ' ')
         strcpy(line, &line[1]);

      // If present, flag and remove leading plus sign
      if (line[0] == '+') {
         plusFlag = 1;
         strcpy(line, &line[1]);
      } // if

      // If present, flag and remove leading minus sign
      if (line[0] == '-') {
         plusFlag = -1;
         strcpy(line, &line[1]);
      } // if

      // Extract numeric response and, if present, suffix
      num = sscanf(line, "%llu%c", &response, &suffix);

      // If no response, use default (def)
      if (num <= 0) {
         response = (unsigned long long) def;
	 suffix = ' ';
         plusFlag = 0;
      } // if

      // Set multiplier based on suffix
      switch (suffix) {
         case 'K':
         case 'k':
            mult = (uint64_t) 1024 / SECTOR_SIZE;
	    break;
         case 'M':
	 case 'm':
	    mult = (uint64_t) 1048576 / SECTOR_SIZE;
            break;
         case 'G':
         case 'g':
            mult = (uint64_t) 1073741824 / SECTOR_SIZE;
            break;
         case 'T':
	 case 't':
            mult = ((uint64_t) 1073741824 * (uint64_t) 1024) / (uint64_t) SECTOR_SIZE;
            break;
         default:
            mult = 1;
      } // switch

      // Adjust response based on multiplier and plus flag, if present
      response *= (unsigned long long) mult;
      if (plusFlag == 1) {
         response = response + (unsigned long long) low - UINT64_C(1);
      } // if
      if (plusFlag == -1) {
         response = (unsigned long long) high - response;
      } // if
   } // while
   return ((uint64_t) response);
} // GetSectorNum()

// Takes a size in bytes (in size) and converts this to a size in
// SI units (KiB, MiB, GiB, TiB, or PiB), returned in C++ string
// form
char* BytesToSI(uint64_t size, char theValue[]) {
   char units[8];
   float sizeInSI;

   if (theValue != NULL) {
      sizeInSI = (float) size;
      strcpy (units, " bytes");
      if (sizeInSI > 1024.0) {
         sizeInSI /= 1024.0;
         strcpy(units, " KiB");
      } // if
      if (sizeInSI > 1024.0) {
         sizeInSI /= 1024.0;
         strcpy(units, " MiB");
      } // if
      if (sizeInSI > 1024.0) {
         sizeInSI /= 1024.0;
         strcpy(units, " GiB");
      } // if
      if (sizeInSI > 1024.0) {
         sizeInSI /= 1024.0;
         strcpy(units, " TiB");
      } // if
      if (sizeInSI > 1024.0) {
         sizeInSI /= 1024.0;
         strcpy(units, " PiB");
      } // if
      if (strcmp(units, " bytes") == 0) { // in bytes, so no decimal point
         sprintf(theValue, "%1.0f%s", sizeInSI, units);
      } else {
         sprintf(theValue, "%1.1f%s", sizeInSI, units);
      } // if/else
   } // if
   return theValue;
} // BlocksToSI()

// Returns block size of device pointed to by fd file descriptor. If the ioctl
// returns an error condition, print a warning but return a value of SECTOR_SIZE
// (512)..
int GetBlockSize(int fd) {
   int err, result;

#ifdef __APPLE__
   err = ioctl(fd, DKIOCGETBLOCKSIZE, &result);
#else
#ifdef __FreeBSD__
   err = ioctl(fd, DIOCGSECTORSIZE, &result);
#else
   err = ioctl(fd, BLKSSZGET, &result);
#endif
#endif

   if (err == -1) {
      result = SECTOR_SIZE;
      // ENOTTY = inappropriate ioctl; probably being called on a disk image
      // file, so don't display the warning message....
      if (errno != ENOTTY) {
         printf("\aError %d when determining sector size! Setting sector size to %d\n",
                errno, SECTOR_SIZE);
      } // if
   } // if

   if (result != 512) {
      printf("\aWARNING! Sector size is not 512 bytes! This program is likely to ");
      printf("misbehave!\nProceed at your own risk!\n\n");
   } // if

   return (result);
} // GetBlockSize()

// Return a plain-text name for a partition type.
// Convert a GUID to a string representation, suitable for display
// to humans....
char* GUIDToStr(struct GUIDData theGUID, char* theString) {
   uint64_t block;

   if (theString != NULL) {
      block = (theGUID.data1 & UINT64_C(0x00000000FFFFFFFF));
      sprintf(theString, "%08llX-", (unsigned long long) block);
      block = (theGUID.data1 & UINT64_C(0x0000FFFF00000000)) >> 32;
      sprintf(theString, "%s%04llX-", theString, (unsigned long long) block);
      block = (theGUID.data1 & UINT64_C(0xFFFF000000000000)) >> 48;
      sprintf(theString, "%s%04llX-", theString, (unsigned long long) block);
      block = (theGUID.data2 & UINT64_C(0x00000000000000FF));
      sprintf(theString, "%s%02llX", theString, (unsigned long long) block);
      block = (theGUID.data2 & UINT64_C(0x000000000000FF00)) >> 8;
      sprintf(theString, "%s%02llX-", theString, (unsigned long long) block);
      block = (theGUID.data2 & UINT64_C(0x0000000000FF0000)) >> 16;
      sprintf(theString, "%s%02llX", theString, (unsigned long long) block);
      block = (theGUID.data2 & UINT64_C(0x00000000FF000000)) >> 24;
      sprintf(theString, "%s%02llX", theString, (unsigned long long) block);
      block = (theGUID.data2 & UINT64_C(0x000000FF00000000)) >> 32;
      sprintf(theString, "%s%02llX", theString, (unsigned long long) block);
      block = (theGUID.data2 & UINT64_C(0x0000FF0000000000)) >> 40;
      sprintf(theString, "%s%02llX", theString, (unsigned long long) block);
      block = (theGUID.data2 & UINT64_C(0x00FF000000000000)) >> 48;
      sprintf(theString, "%s%02llX", theString, (unsigned long long) block);
      block = (theGUID.data2 & UINT64_C(0xFF00000000000000)) >> 56;
      sprintf(theString, "%s%02llX", theString, (unsigned long long) block);
   } // if
   return theString;
} // GUIDToStr()

// Get a GUID from the user
GUIDData GetGUID(void) {
   uint64_t part1, part2, part3, part4, part5;
   int entered = 0;
   char temp[255], temp2[255];
   GUIDData theGUID;

   printf("\nA GUID is entered in five segments of from two to six bytes, with\n"
          "dashes between segments.\n");
   printf("Enter the entire GUID, a four-byte hexadecimal number for the first segment, or\n"
          "'R' to generate the entire GUID randomly: ");
   fgets(temp, 255, stdin);

   // If user entered 'r' or 'R', generate GUID randomly....
   if ((temp[0] == 'r') || (temp[0] == 'R')) {
      theGUID.data1 = (uint64_t) rand() * (uint64_t) rand();
      theGUID.data2 = (uint64_t) rand() * (uint64_t) rand();
      entered = 1;
   } // if user entered 'R' or 'r'

   // If string length is right for whole entry, try to parse it....
   if ((strlen(temp) == 37) && (entered == 0)) {
      strncpy(temp2, &temp[0], 8);
      temp2[8] = '\0';
      sscanf(temp2, "%llx", &part1);
      strncpy(temp2, &temp[9], 4);
      temp2[4] = '\0';
      sscanf(temp2, "%llx", &part2);
      strncpy(temp2, &temp[14], 4);
      temp2[4] = '\0';
      sscanf(temp2, "%llx", &part3);
      theGUID.data1 = (part3 << 48) + (part2 << 32) + part1;
      strncpy(temp2, &temp[19], 4);
      temp2[4] = '\0';
      sscanf(temp2, "%llx", &part4);
      strncpy(temp2, &temp[24], 12);
      temp2[12] = '\0';
      sscanf(temp2, "%llx", &part5);
      theGUID.data2 = ((part4 & UINT64_C(0x000000000000FF00)) >> 8) +
                      ((part4 & UINT64_C(0x00000000000000FF)) << 8) +
                      ((part5 & UINT64_C(0x0000FF0000000000)) >> 24) +
                      ((part5 & UINT64_C(0x000000FF00000000)) >> 8) +
                      ((part5 & UINT64_C(0x00000000FF000000)) << 8) +
                      ((part5 & UINT64_C(0x0000000000FF0000)) << 24) +
                      ((part5 & UINT64_C(0x000000000000FF00)) << 40) +
                      ((part5 & UINT64_C(0x00000000000000FF)) << 56);
      entered = 1;
   } // if

   // If neither of the above methods of entry was used, use prompted
   // entry....
   if (entered == 0) {
      sscanf(temp, "%llx", &part1);
      printf("Enter a two-byte hexadecimal number for the second segment: ");
      fgets(temp, 255, stdin);
      sscanf(temp, "%llx", &part2);
      printf("Enter a two-byte hexadecimal number for the third segment: ");
      fgets(temp, 255, stdin);
      sscanf(temp, "%llx", &part3);
      theGUID.data1 = (part3 << 48) + (part2 << 32) + part1;
      printf("Enter a two-byte hexadecimal number for the fourth segment: ");
      fgets(temp, 255, stdin);
      sscanf(temp, "%llx", &part4);
      printf("Enter a six-byte hexadecimal number for the fifth segment: ");
      fgets(temp, 255, stdin);
      sscanf(temp, "%llx", &part5);
      theGUID.data2 = ((part4 & UINT64_C(0x000000000000FF00)) >> 8) +
                      ((part4 & UINT64_C(0x00000000000000FF)) << 8) +
                      ((part5 & UINT64_C(0x0000FF0000000000)) >> 24) +
                      ((part5 & UINT64_C(0x000000FF00000000)) >> 8) +
                      ((part5 & UINT64_C(0x00000000FF000000)) << 8) +
                      ((part5 & UINT64_C(0x0000000000FF0000)) << 24) +
                      ((part5 & UINT64_C(0x000000000000FF00)) << 40) +
                      ((part5 & UINT64_C(0x00000000000000FF)) << 56);
      entered = 1;
   } // if/else
   printf("New GUID: %s\n", GUIDToStr(theGUID, temp));
   return theGUID;
} // GetGUID()

// Return 1 if the CPU architecture is little endian, 0 if it's big endian....
int IsLittleEndian(void) {
   int littleE = 1; // assume little-endian (Intel-style)
   union {
      uint32_t num;
      unsigned char uc[sizeof(uint32_t)];
   } endian;

   endian.num = 1;
   if (endian.uc[0] != (unsigned char) 1) {
      littleE = 0;
   } // if
   return (littleE);
} // IsLittleEndian()

// Reverse the byte order of theValue; numBytes is number of bytes
void ReverseBytes(void* theValue, int numBytes) {
   char* origValue;
   char* tempValue;
   int i;

   origValue = (char*) theValue;
   tempValue = (char*) malloc(numBytes);
   for (i = 0; i < numBytes; i++)
      tempValue[i] = origValue[i];
   for (i = 0; i < numBytes; i++)
      origValue[i] = tempValue[numBytes - i - 1];
   free(tempValue);
} // ReverseBytes()

// Compute (2 ^ value). Given the return type, value must be 63 or less.
// Used in some bit-fiddling functions
uint64_t PowerOf2(int value) {
   uint64_t retval = 1;
   int i;

   if ((value < 64) && (value >= 0)) {
      for (i = 0; i < value; i++) {
         retval *= 2;
      } // for
   } else retval = 0;
   return retval;
} // PowerOf2()

// An extended file-open function. This includes some system-specific checks.
// I want them in a function because I use these calls twice and I don't want
// to forget to change them in one location if I need to change them in
// the other....
int OpenForWrite(char* deviceFilename) {
   int fd;

   fd = open(deviceFilename, O_WRONLY); // try to open the device; may fail....
#ifdef __APPLE__
   // MacOS X requires a shared lock under some circumstances....
   if (fd < 0) {
      fd = open(deviceFilename, O_WRONLY|O_SHLOCK);
   } // if
#endif
	return fd;
} // OpenForWrite()

// Resync disk caches so the OS uses the new partition table. This code varies
// a lot from one OS to another.
void DiskSync(int fd) {
   int i;

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
   printf("Warning: The kernel may continue to use old or deleted partitions.\n"
          "You should reboot or remove the drive.\n");
#else
   sleep(2);
   i = ioctl(fd, BLKRRPART);
   if (i)
      printf("Warning: The kernel is still using the old partition table.\n"
            "The new table will be used at the next reboot.\n");
#endif
#endif
} // DiskSync()

/**************************************************************************************
 *                                                                                    *
 * Below functions are lifted from various sources, as documented in comments before  *
 * each one.                                                                          *
 *                                                                                    *
 **************************************************************************************/

// The disksize function is taken from the Linux fdisk code and modified
// to work around a problem returning a uint64_t value on Mac OS.
uint64_t disksize(int fd, int *err) {
   long sz; // Do not delete; needed for Linux
   long long b; // Do not delete; needed for Linux
   uint64_t sectors = 0, bytes = 0; // size in sectors & bytes
   struct stat st;

   // Note to self: I recall testing a simplified version of
   // this code, similar to what's in the __APPLE__ block,
   // on Linux, but I had some problems. IIRC, it ran OK on 32-bit
   // systems but not on 64-bit. Keep this in mind in case of
   // 32/64-bit issues on MacOS....
#ifdef __APPLE__
   *err = ioctl(fd, DKIOCGETBLOCKCOUNT, &sectors);
#else
#ifdef __FreeBSD__
   *err = ioctl(fd, DIOCGMEDIASIZE, &sz);
   b = GetBlockSize(fd);
   sectors = sz / b;
#else
   *err = ioctl(fd, BLKGETSIZE, &sz);
   if (*err) {
      sectors = sz = 0;
   } // if
   if ((errno == EFBIG) || (!*err)) {
      *err = ioctl(fd, BLKGETSIZE64, &b);
      if (*err || b == 0 || b == sz)
         sectors = sz;
      else
         sectors = (b >> 9);
   } // if

//         if (*err) {
//            sz = 0;
//            if (errno != EFBIG)
//               return sz;
//         }
//         *err = ioctl(fd, BLKGETSIZE64, &b);
//         if (*err || b == 0 || b == sz)
//            sectors = sz;
//         else
//            sectors = (b >> 9);

#endif
#endif

   // The above methods have failed (or it's a bum filename reference),
   // so let's assume it's a regular file (a QEMU image, dd backup, or
   // what have you) and see what stat() gives us....
   if (sectors == 0) {
      if (fstat(fd, &st) == 0) {
         bytes = (uint64_t) st.st_size;
         if ((bytes % UINT64_C(512)) != 0)
            fprintf(stderr, "Warning: File size is not a multiple of 512 bytes!"
                            " Misbehavior is likely!\n\a");
         sectors = bytes / UINT64_C(512);
      } // if
   } // if
//   printf("In disksize(), sectors is %lld.\n", sectors);
   return sectors;
}
