// fixparts
// Program to fix certain types of damaged Master Boot Record (MBR) partition
// tables
//
// Copyright 2011 by Roderick W. Smith
//
// This program is distributed under the terms of the GNU GPL, as described
// in the COPYING file.
//
// Based on C++ classes originally created for GPT fdisk (gdisk and sgdisk)
// programs

#include <stdio.h>
#include <string.h>
#include <string>
#include <iostream>
#include <sstream>
#include "basicmbr.h"
#include "support.h"

using namespace std;

int main(int argc, char* argv[]) {
   BasicMBRData mbrTable;
   string device;
   int doItAgain;

   switch (argc) {
      case 1:
         cout << "Type device filename, or press <Enter> to exit: ";
         device = ReadString();
         if (device.length() == 0)
            exit(0);
         break;
      case 2:
         device = argv[1];
         break;
      default:
         cerr << "Usage: " << argv[0] << " device_filename\n";
         exit(1);
   } // switch

   cout << "FixParts " << GPTFDISK_VERSION << "\n";
   cout << "\nLoading MBR data from " << device << "\n";
   if (mbrTable.ReadMBRData(device)) {
      if (mbrTable.CheckForGPT() > 0) {
         if ((mbrTable.GetValidity() == hybrid) || (mbrTable.GetValidity() == gpt)) {
            cerr << "\nThis disk appears to be a GPT disk. Use GNU Parted or GPT fdisk on it!\n";
            cerr << "Exiting!\n\n";
            exit(1);
         } else {
            cout << "\nNOTICE: GPT signatures detected on the disk, but no 0xEE protective "
               << "partition!\nThe GPT signatures are probably left over from a previous "
               << "partition table.\nDo you want to delete them (if you answer 'Y', this "
               << "will happen\nimmediately)? ";
            if (GetYN() == 'Y') {
               cout << "Erasing GPT data!\n";
               if (mbrTable.BlankGPTData() != 1)
                  cerr << "GPT signature erasure failed!\n";
            } // if
         } // if/else
      } // if
      mbrTable.MakeItLegal();
      do {
         doItAgain = 0;
         if (mbrTable.DoMenu() > 0) {
            cout << "\nFinal checks complete. About to write MBR data. THIS WILL OVERWRITE EXISTING\n"
               << "PARTITIONS!!\n\nDo you want to proceed? ";
            if (GetYN() == 'Y') {
               mbrTable.WriteMBRData();
               mbrTable.DiskSync();
         doItAgain = 0;
            } else {
               doItAgain = 1;
            } // else
         } // if
      } while (doItAgain);
   } // if read OK
   return 0;
} // main()
