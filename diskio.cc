//
// C++ Interface: diskio (platform-independent components)
//
// Description: Class to handle low-level disk I/O for GPT fdisk
//
//
// Author: Rod Smith <rodsmith@rodsbooks.com>, (C) 2009
//
// Copyright: See COPYING file that comes with this distribution
//
//
// This program is copyright (c) 2009 by Roderick W. Smith. It is distributed
// under the terms of the GNU GPL version 2, as detailed in the COPYING file.

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
#include <windows.h>
#include <winioctl.h>
#define fstat64 fstat
#define stat64 stat
#define S_IRGRP 0
#define S_IROTH 0
#else
#include <sys/ioctl.h>
#endif
#include <stdio.h>
#include <string>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <iostream>

#include "support.h"
#include "diskio.h"

using namespace std;

DiskIO::DiskIO(void) {
   userFilename = "";
   realFilename = "";
   isOpen = 0;
   openForWrite = 0;
   sectorData = NULL;
} // constructor

DiskIO::~DiskIO(void) {
   Close();
   free(sectorData);
} // destructor

// Open a disk device for reading. Returns 1 on success, 0 on failure.
int DiskIO::OpenForRead(const string & filename) {
   int shouldOpen = 1;

   if (isOpen) { // file is already open
      if (((realFilename != filename) && (userFilename != filename)) || (openForWrite)) {
         Close();
      } else {
         shouldOpen = 0;
      } // if/else
   } // if

   if (shouldOpen) {
      userFilename = filename;
      MakeRealName();
      OpenForRead();
   } // if

   return isOpen;
} // DiskIO::OpenForRead(string filename)

// Open a disk for reading and writing by filename.
// Returns 1 on success, 0 on failure.
int DiskIO::OpenForWrite(const string & filename) {
   int retval = 0;

   if ((isOpen) && (openForWrite) && ((filename == realFilename) || (filename == userFilename))) {
      retval = 1;
   } else {
      userFilename = filename;
      MakeRealName();
      retval = OpenForWrite();
      if (retval == 0) {
         realFilename = userFilename = "";
      } // if
   } // if/else
   return retval;
} // DiskIO::OpenForWrite(string filename)

// My original FindAlignment() function (after this one) isn't working, since
// the BLKPBSZGET ioctl() isn't doing what I expected (it returns 512 even on
// a WD Advanced Format drive). Therefore, I'm using a simpler function that
// returns 1-sector alignment for unusual sector sizes and drives smaller than
// a size defined by SMALLEST_ADVANCED_FORMAT, and 8-sector alignment for
// larger drives with 512-byte sectors.
int DiskIO::FindAlignment(void) {
   int err, result;

   if ((GetBlockSize() == 512) && (DiskSize(&err) >= SMALLEST_ADVANCED_FORMAT)) {
      result = 8; // play it safe; align for 4096-byte sectors
   } else {
      result = 1; // unusual sector size; assume it's the real physical size
   } // if/else
   return result;
} // DiskIO::FindAlignment

// Return the partition alignment value in sectors. Right now this works
// only for Linux 2.6.32 and later, since I can't find equivalent ioctl()s
// for OS X or FreeBSD, and the Linux ioctl is new
/* int DiskIO::FindAlignment(int fd) {
   int err = -2, errnum = 0, result = 8, physicalSectorSize = 4096;
   uint64_t diskSize;

   cout << "Entering FindAlignment()\n";
#if defined (__linux__) && defined (BLKPBSZGET)
   err = ioctl(fd, BLKPBSZGET, &physicalSectorSize);
   cout << "In FindAlignment(), physicalSectorSize = " << physicalSectorSize
        << ", err = " << err << "\n";
#else
   err = -1;
#endif

   if (err < 0) { // ioctl didn't work; have to guess....
      if (GetBlockSize(fd) == 512) {
         result = 8; // play it safe; align for 4096-byte sectors
} else {
         result = 1; // unusual sector size; assume it's the real physical size
} // if/else
} else { // ioctl worked; compute alignment
      result = physicalSectorSize / GetBlockSize(fd);
      // Disks with larger physical than logical sectors must theoretically
      // have a total disk size that's a multiple of the physical sector
      // size; however, some such disks have compatibility jumper settings
      // meant for one-partition MBR setups, and these reduce the total
      // number of sectors by 1. If such a setting is used, it'll result
      // in improper alignment, so look for this condition and warn the
      // user if it's found....
      diskSize = disksize(fd, &errnum);
      if ((diskSize % (uint64_t) result) != 0) {
         fprintf(stderr, "\aWarning! Disk size (%llu) is not a multiple of alignment\n"
                         "size (%d), but it should be! Check disk manual and jumper settings!\n",
                         (unsigned long long) diskSize, result);
} // if
} // if/else
   if (result <= 0) // can happen if physical sector size < logical sector size
      result = 1;
   return result;
} // DiskIO::FindAlignment(int) */

// The same as FindAlignment(int), but opens and closes a device by filename
int DiskIO::FindAlignment(const string & filename) {
   int fd;
   int retval = 1;

   if (!isOpen)
      OpenForRead(filename);
   if (isOpen) {
      retval = FindAlignment();
   } // if
   return retval;
} // DiskIO::FindAlignment(char)
