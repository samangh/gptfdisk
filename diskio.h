//
// C++ Interface: diskio
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

#ifndef __DISKIO_H
#define __DISKIO_H

#include <string>
#include <stdint.h>
#include <sys/types.h>
#ifdef MINGW
#include <windows.h>
#include <winioctl.h>
#else
#include <sys/ioctl.h>
#endif

#if defined (__FreeBSD__) || defined (__APPLE__)
#define fstat64 fstat
#define stat64 stat
#endif

#include "support.h"
#include "parttypes.h"

using namespace std;

// Below constant corresponds to an 800GB disk -- a somewhat arbitrary
// cutoff
#define SMALLEST_ADVANCED_FORMAT UINT64_C(1677721600)

/***************************************
 *                                     *
 * DiskIO class and related structures *
 *                                     *
 ***************************************/

class DiskIO {
   protected:
      string userFilename;
      string realFilename;
      int isOpen;
      int openForWrite;
      uint8_t *sectorData;
#ifdef MINGW
      HANDLE fd;
#else
      int fd;
#endif
   public:
      DiskIO(void);
//      DiskIO(const DiskIO & orig);
      ~DiskIO(void);

//      DiskIO & operator=(const DiskIO & orig);
      void MakeRealName(void);
      int OpenForRead(string filename);
      int OpenForRead(void);
      int OpenForWrite(string filename);
      int OpenForWrite(void);
      void Close();
      int Seek(uint64_t sector);
      int Read(void* buffer, int numBytes);
      int Write(void* buffer, int numBytes);
      void DiskSync(void); // resync disk caches to use new partitions
      int GetBlockSize(void);
      int FindAlignment(void);
      int FindAlignment(string filename);
      int IsOpen(void) {return isOpen;}
      int IsOpenForWrite(void) {return openForWrite;}

      uint64_t DiskSize(int* err);
}; // struct GPTPart

#endif
