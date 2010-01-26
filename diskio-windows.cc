//
// C++ Interface: diskio (Windows-specific components)
//
// Description: Class to handle low-level disk I/O for GPT fdisk
//
//
// Author: Rod Smith <rodsmith@rodsbooks.com>, (C) 2009
//
// Copyright: See COPYING file that comes with this distribution
//
//
// This program is copyright (c) 2009, 2010 by Roderick W. Smith. It is distributed
// under the terms of the GNU GPL version 2, as detailed in the COPYING file.

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include <windows.h>
#include <winioctl.h>
#define fstat64 fstat
#define stat64 stat
#define S_IRGRP 0
#define S_IROTH 0
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

// Returns the official Windows name for a shortened version of same.
void DiskIO::MakeRealName(void) {
   int colonPos;

   colonPos = userFilename.find(':', 0);
   if ((colonPos != string::npos) && (colonPos <= 3)) {
      realFilename = "\\\\.\\physicaldrive";
      realFilename += userFilename.substr(0, colonPos);
   } // if/else
   printf("Exiting DiskIO::MakeRealName(); translated '%s' ", userFilename.c_str());
   printf("to '%s'\n", realFilename.c_str());
} // DiskIO::MakeRealName()

// Open the currently on-record file for reading
int DiskIO::OpenForRead(void) {
   int shouldOpen = 1;

   if (isOpen) { // file is already open
      if (openForWrite) {
         Close();
      } else {
         shouldOpen = 0;
      } // if/else
   } // if

   if (shouldOpen) {
      printf("Opening '%s' for reading.\n", realFilename.c_str());
      fd = CreateFile(realFilename.c_str(),GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                      NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
      if (fd == INVALID_HANDLE_VALUE) {
         CloseHandle(fd);
         fprintf(stderr, "Problem opening %s for reading!\n", realFilename.c_str());
         realFilename = "";
         userFilename = "";
         isOpen = 0;
         openForWrite = 0;
      } else {
         isOpen = 1;
         openForWrite = 0;
      } // if/else
   } // if

   return isOpen;
} // DiskIO::OpenForRead(void)

// An extended file-open function. This includes some system-specific checks.
// Returns 1 if the file is open, 0 otherwise....
int DiskIO::OpenForWrite(void) {
   if ((isOpen) && (openForWrite))
      return 1;

   // Close the disk, in case it's already open for reading only....
   Close();

   // try to open the device; may fail....
   fd = CreateFile(realFilename.c_str(), GENERIC_READ | GENERIC_WRITE,
                   FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                   FILE_ATTRIBUTE_NORMAL, NULL);
   if (fd == INVALID_HANDLE_VALUE) {
      CloseHandle(fd);
      isOpen = 1;
      openForWrite = 1;
   } else {
      isOpen = 0;
      openForWrite = 0;
   } // if/else
   return isOpen;
} // DiskIO::OpenForWrite(void)

// Close the disk device. Note that this does NOT erase the stored filenames,
// so the file can be re-opened without specifying the filename.
void DiskIO::Close(void) {
   if (isOpen)
      CloseHandle(fd);
   isOpen = 0;
   openForWrite = 0;
} // DiskIO::Close()

// Returns block size of device pointed to by fd file descriptor. If the ioctl
// returns an error condition, print a warning but return a value of SECTOR_SIZE
// (512)..
int DiskIO::GetBlockSize(void) {
   int err;
   DWORD blockSize, junk1, junk2, junk3;

   // If disk isn't open, try to open it....
   if (!isOpen) {
      OpenForRead();
   } // if

   if (isOpen) {
/*      BOOL WINAPI GetDiskFreeSpace(
                                   __in   LPCTSTR lpRootPathName,
                                   __out  LPDWORD lpSectorsPerCluster,
                                   __out  LPDWORD lpBytesPerSector,
                                   __out  LPDWORD lpNumberOfFreeClusters,
                                   __out  LPDWORD lpTotalNumberOfClusters
                                  ); */
//      err = GetDiskFreeSpace(realFilename.c_str(), &junk1, &blockSize, &junk2, &junk3);
      err = 1;
      blockSize = 512;

      if (err == 0) {
         blockSize = SECTOR_SIZE;
         // ENOTTY = inappropriate ioctl; probably being called on a disk image
         // file, so don't display the warning message....
         // 32-bit code returns EINVAL, I don't know why. I know I'm treading on
         // thin ice here, but it should be OK in all but very weird cases....
         if ((errno != ENOTTY) && (errno != EINVAL)) {
            printf("\aError %d when determining sector size! Setting sector size to %d\n",
                   GetLastError(), SECTOR_SIZE);
         } // if
      } // if (err == -1)
   } // if (isOpen)

   return (blockSize);
} // DiskIO::GetBlockSize()

// Resync disk caches so the OS uses the new partition table. This code varies
// a lot from one OS to another.
void DiskIO::DiskSync(void) {
   int i;

   // If disk isn't open, try to open it....
   if (!isOpen) {
      OpenForRead();
   } // if

   if (isOpen) {
#ifndef MINGW
      sync();
#endif
#ifdef MINGW
      printf("Warning: I don't know how to sync disks in Windows! The old partition table is\n"
            "probably still in use!\n");
#endif
   } // if (isOpen)
} // DiskIO::DiskSync()

// Seek to the specified sector. Returns 1 on success, 0 on failure.
int DiskIO::Seek(uint64_t sector) {
   int retval = 1;
   LARGE_INTEGER seekTo;
   uint32_t lowBits, highBits;
   uint64_t bytePos;

   // If disk isn't open, try to open it....
   if (!isOpen) {
      retval = OpenForRead();
   } // if

   if (isOpen) {
      bytePos = sector * (uint64_t) GetBlockSize();
      lowBits = (uint32_t) (bytePos / UINT64_C(4294967296));
      highBits = (uint32_t) (bytePos % UINT64_C(4294967296));
      seekTo.LowPart = lowBits;
      seekTo.HighPart = highBits;
//      seekTo.QuadPart = (LONGLONG) (sector * (uint64_t) GetBlockSize());
/*      printf("In DiskIO::Seek(), sector = %llu, ", sector);
      printf("block size = %d, ", GetBlockSize());
      printf("seekTo.QuadPart = %lld\n", seekTo.QuadPart);
      printf("   seekTo.LowPart = %lu, ", seekTo.LowPart);
      printf("seekTo.HighPart = %lu\n", seekTo.HighPart); */
      retval = SetFilePointerEx(fd, seekTo, NULL, FILE_BEGIN);
      if (retval == 0) {
         errno = GetLastError();
         fprintf(stderr, "Error when seeking to %lld! Error is %d\n",
                 seekTo.QuadPart, errno);
         retval = 0;
      } // if
   } // if
   return retval;
} // DiskIO::Seek()

// A variant on the standard read() function. Done to work around
// limitations in FreeBSD concerning the matching of the sector
// size with the number of bytes read.
// Returns the number of bytes read into buffer.
int DiskIO::Read(void* buffer, int numBytes) {
   int blockSize = 512, i, numBlocks;
   char* tempSpace;
   DWORD retval = 0;

   // If disk isn't open, try to open it....
   if (!isOpen) {
      OpenForRead();
   } // if

   if (isOpen) {
      // Compute required space and allocate memory
      blockSize = GetBlockSize();
      if (numBytes <= blockSize) {
         numBlocks = 1;
         tempSpace = (char*) malloc(blockSize);
      } else {
         numBlocks = numBytes / blockSize;
         if ((numBytes % blockSize) != 0) numBlocks++;
         tempSpace = (char*) malloc(numBlocks * blockSize);
      } // if/else

      // Read the data into temporary space, then copy it to buffer
//      retval = read(fd, tempSpace, numBlocks * blockSize);
      ReadFile(fd, tempSpace, numBlocks * blockSize, &retval, NULL);
      printf("In DiskIO::Read(), have read %d bytes.\n", (int) retval);
      for (i = 0; i < numBytes; i++) {
         ((char*) buffer)[i] = tempSpace[i];
      } // for

      // Adjust the return value, if necessary....
      if (((numBlocks * blockSize) != numBytes) && (retval > 0))
         retval = numBytes;

      free(tempSpace);
   } // if (isOpen)
   return retval;
} // DiskIO::Read()

// A variant on the standard write() function. Done to work around
// limitations in FreeBSD concerning the matching of the sector
// size with the number of bytes read.
// Returns the number of bytes written.
int DiskIO::Write(void* buffer, int numBytes) {
   int blockSize = 512, i, numBlocks, retval = 0;
   char* tempSpace;
   DWORD numWritten;

   // If disk isn't open, try to open it....
   if ((!isOpen) || (!openForWrite)) {
      OpenForWrite();
   } // if

   if (isOpen) {
      // Compute required space and allocate memory
      blockSize = GetBlockSize();
      if (numBytes <= blockSize) {
         numBlocks = 1;
         tempSpace = (char*) malloc(blockSize);
      } else {
         numBlocks = numBytes / blockSize;
         if ((numBytes % blockSize) != 0) numBlocks++;
         tempSpace = (char*) malloc(numBlocks * blockSize);
      } // if/else

      // Copy the data to my own buffer, then write it
      for (i = 0; i < numBytes; i++) {
         tempSpace[i] = ((char*) buffer)[i];
      } // for
      for (i = numBytes; i < numBlocks * blockSize; i++) {
         tempSpace[i] = 0;
      } // for
//      retval = write(fd, tempSpace, numBlocks * blockSize);
      WriteFile(fd, tempSpace, numBlocks * blockSize, &numWritten, NULL);
      retval = (int) numWritten;

      // Adjust the return value, if necessary....
      if (((numBlocks * blockSize) != numBytes) && (retval > 0))
         retval = numBytes;

      free(tempSpace);
   } // if (isOpen)
   return retval;
} // DiskIO:Write()

// Returns the size of the disk in blocks.
uint64_t DiskIO::DiskSize(int *err) {
   uint64_t sectors = 0; // size in sectors
   off_t bytes = 0; // size in bytes
   struct stat64 st;
   GET_LENGTH_INFORMATION buf;
   DWORD i;

   // If disk isn't open, try to open it....
   if (!isOpen) {
      OpenForRead();
   } // if

   if (isOpen) {
      // Note to self: I recall testing a simplified version of
      // this code, similar to what's in the __APPLE__ block,
      // on Linux, but I had some problems. IIRC, it ran OK on 32-bit
      // systems but not on 64-bit. Keep this in mind in case of
      // 32/64-bit issues on MacOS....
/*      HANDLE fin;
      fin = CreateFile(realFilename.c_str(), GENERIC_READ,
                     FILE_SHARE_READ|FILE_SHARE_WRITE,
                     NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL); */
      if (DeviceIoControl(fd, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &buf, sizeof(buf), &i, NULL)) {
         sectors = (uint64_t) buf.Length.QuadPart / GetBlockSize();
//         printf("disk_get_size_win32 IOCTL_DISK_GET_LENGTH_INFO = %llu\n",
//               (long long unsigned) sectors);
      } else {
         fprintf(stderr, "Couldn't determine disk size!\n");
      }

/*      // The above methods have failed, so let's assume it's a regular
      // file (a QEMU image, dd backup, or what have you) and see what
      // fstat() gives us....
      if ((sectors == 0) || (*err == -1)) {
         if (fstat64(fd, &st) == 0) {
            bytes = (off_t) st.st_size;
            if ((bytes % UINT64_C(512)) != 0)
               fprintf(stderr, "Warning: File size is not a multiple of 512 bytes!"
                       " Misbehavior is likely!\n\a");
            sectors = bytes / UINT64_C(512);
         } // if
      } // if */
   } // if (isOpen)
   return sectors;
} // DiskIO::DiskSize()
