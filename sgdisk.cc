// sgdisk.cc
// Command-line-based version of gdisk. This program is named after sfdisk,
// and it can serve a similar role (easily scripted, etc.), but it's used
// strictly via command-line arguments, and it doesn't bear much resemblance
// to sfdisk in actual use.
//
// by Rod Smith, project began February 2009; sgdisk begun January 2010.

/* This program is copyright (c) 2009-2011 by Roderick W. Smith. It is distributed
  under the terms of the GNU GPL version 2, as detailed in the COPYING file. */

#include <stdio.h>
#include <popt.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <string>
#include <iostream>
#include <sstream>
#include "mbr.h"
#include "gpt.h"
#include "support.h"
#include "parttypes.h"
#include "attributes.h"

using namespace std;

#define MAX_OPTIONS 50

int BuildMBR(GPTData& theGPT, char* argument, int isHybrid);
int CountColons(char* argument);

int main(int argc, char *argv[]) {
   GPTData theGPT, secondDevice;
   uint32_t sSize;
   uint64_t low, high;
   int opt, numOptions = 0, saveData = 0, neverSaveData = 0;
   int partNum = 0, deletePartNum = 0, infoPartNum = 0, bsdPartNum = 0, largestPartNum = 0;
   int saveNonGPT = 1;
   uint32_t gptPartNum = 0;
   int alignment = DEFAULT_ALIGNMENT, retval = 0, pretend = 0;
   uint32_t tableSize = 128;
   uint64_t startSector, endSector;
   uint64_t temp; // temporary variable; free to use in any case
   char *attributeOperation = NULL;
   char *device = NULL;
   char *newPartInfo = NULL, *typeCode = NULL, *partName = NULL;
   char *backupFile = NULL, *twoParts = NULL, *hybrids = NULL, *mbrParts;
   char *partGUID = NULL, *diskGUID = NULL, *outDevice = NULL;
   string cmd, typeGUID;
   PartType typeHelper;

   poptContext poptCon;
   struct poptOption theOptions[] =
   {
      {"attributes", 'A', POPT_ARG_STRING, &attributeOperation, 'A', "operate on partition attributes", "list|[partnum:show|or|nand|xor|=|set|clear|toggle|get[:bitnum|hexbitmask]]"},
      {"set-alignment", 'a', POPT_ARG_INT, &alignment, 'a', "set sector alignment", "value"},
      {"backup", 'b', POPT_ARG_STRING, &backupFile, 'b', "backup GPT to file", "file"},
      {"change-name", 'c', POPT_ARG_STRING, &partName, 'c', "change partition's name", "partnum:name"},
      {"recompute-chs", 'C', POPT_ARG_NONE, NULL, 'C', "recompute CHS values in protective/hybrid MBR", ""},
      {"delete", 'd', POPT_ARG_INT, &deletePartNum, 'd', "delete a partition", "partnum"},
      {"display-alignment", 'D', POPT_ARG_NONE, NULL, 'D', "show number of sectors per allocation block", ""},
      {"move-second-header", 'e', POPT_ARG_NONE, NULL, 'e', "move second header to end of disk", ""},
      {"end-of-largest", 'E', POPT_ARG_NONE, NULL, 'E', "show end of largest free block", ""},
      {"first-in-largest", 'f', POPT_ARG_NONE, NULL, 'f', "show start of the largest free block", ""},
      {"first-aligned-in-largest", 'F', POPT_ARG_NONE, NULL, 'F', "show start of the largest free block, aligned", ""},
      {"mbrtogpt", 'g', POPT_ARG_NONE, NULL, 'g', "convert MBR to GPT", ""},
      {"randomize-guids", 'G', POPT_ARG_NONE, NULL, 'G', "randomize disk and partition GUIDs", ""},
      {"hybrid", 'h', POPT_ARG_STRING, &hybrids, 'h', "create hybrid MBR", "partnum[:partnum...]"},
      {"info", 'i', POPT_ARG_INT, &infoPartNum, 'i', "show detailed information on partition", "partnum"},
      {"load-backup", 'l', POPT_ARG_STRING, &backupFile, 'l', "load GPT backup from file", "file"},
      {"list-types", 'L', POPT_ARG_NONE, NULL, 'L', "list known partition types", ""},
      {"gpttombr", 'm', POPT_ARG_STRING, &mbrParts, 'm', "convert GPT to MBR", "partnum[:partnum...]"},
      {"new", 'n', POPT_ARG_STRING, &newPartInfo, 'n', "create new partition", "partnum:start:end"},
      {"largest-new", 'N', POPT_ARG_INT, &largestPartNum, 'N', "create largest possible new partition", "partnum"},
      {"clear", 'o', POPT_ARG_NONE, NULL, 'o', "clear partition table", ""},
      {"print", 'p', POPT_ARG_NONE, NULL, 'p', "print partition table", ""},
      {"pretend", 'P', POPT_ARG_NONE, NULL, 'P', "make changes in memory, but don't write them", ""},
      {"transpose", 'r', POPT_ARG_STRING, &twoParts, 'r', "transpose two partitions", "partnum:partnum"},
      {"replicate", 'R', POPT_ARG_STRING, &outDevice, 'R', "replicate partition table", "device_filename"},
      {"sort", 's', POPT_ARG_NONE, NULL, 's', "sort partition table entries", ""},
      {"resize-table", 'S', POPT_ARG_INT, &tableSize, 'S', "resize partition table", "numparts"},
      {"typecode", 't', POPT_ARG_STRING, &typeCode, 't', "change partition type code", "partnum:{hexcode|GUID}"},
      {"transform-bsd", 'T', POPT_ARG_INT, &bsdPartNum, 'T', "transform BSD disklabel partition to GPT", "partnum"},
      {"partition-guid", 'u', POPT_ARG_STRING, &partGUID, 'u', "set partition GUID", "partnum:guid"},
      {"disk-guid", 'U', POPT_ARG_STRING, &diskGUID, 'U', "set disk GUID", "guid"},
      {"verify", 'v', POPT_ARG_NONE, NULL, 'v', "check partition table integrity", ""},
      {"version", 'V', POPT_ARG_NONE, NULL, 'V', "display version information", ""},
      {"zap", 'z', POPT_ARG_NONE, NULL, 'z', "zap (destroy) GPT (but not MBR) data structures", ""},
      {"zap-all", 'Z', POPT_ARG_NONE, NULL, 'Z', "zap (destroy) GPT and MBR data structures", ""},
      POPT_AUTOHELP { NULL, 0, 0, NULL, 0 }
   };

   // Create popt context...
   poptCon = poptGetContext(NULL, argc, (const char**) argv, theOptions, 0);

   poptSetOtherOptionHelp(poptCon, " [OPTION...] <device>");

   if (argc < 2) {
      poptPrintUsage(poptCon, stderr, 0);
      exit(1);
   }

   // Do one loop through the options to find the device filename and deal
   // with options that don't require a device filename....
   while ((opt = poptGetNextOpt(poptCon)) > 0) {
      switch (opt) {
         case 'A':
            cmd = GetString(attributeOperation, 1);
            if (cmd == "list")
               Attributes::ListAttributes();
            break;
         case 'L':
            typeHelper.ShowAllTypes();
            break;
         case 'P':
            pretend = 1;
            break;
         case 'V':
            cout << "GPT fdisk (sgdisk) version " << GPTFDISK_VERSION << "\n\n";
            break;
         default:
            break;
      } // switch
      numOptions++;
   } // while

   // Assume first non-option argument is the device filename....
   device = (char*) poptGetArg(poptCon);
   poptResetContext(poptCon);

   if (device != NULL) {
      theGPT.JustLooking(); // reset as necessary
      theGPT.BeQuiet(); // Tell called functions to be less verbose & interactive
      if (theGPT.LoadPartitions((string) device)) {
         if ((theGPT.WhichWasUsed() == use_mbr) || (theGPT.WhichWasUsed() == use_bsd))
            saveNonGPT = 0; // flag so we don't overwrite unless directed to do so
         sSize = theGPT.GetBlockSize();
         while ((opt = poptGetNextOpt(poptCon)) > 0) {
            switch (opt) {
               case 'A': {
                  if (cmd != "list") {
                     partNum = (int) GetInt(attributeOperation, 1) - 1;
                     if ((partNum >= 0) && (partNum < (int) theGPT.GetNumParts())) {
                        switch (theGPT.ManageAttributes(partNum, GetString(attributeOperation, 2),
                              GetString(attributeOperation, 3))) {
                           case -1:
                              saveData = 0;
                              neverSaveData = 1;
                              break;
                           case 1:
                              theGPT.JustLooking(0);
                              saveData = 1;
                              break;
                           default:
                              break;
                        } // switch
                     } else {
                        cerr << "Error: Invalid partition number " << partNum + 1 << "\n";
                        saveData = 0;
                        neverSaveData = 1;
                     } // if/else reasonable partition #
                  } // if (cmd != "list")
                  break;
               } // case 'A':
               case 'a':
                  theGPT.SetAlignment(alignment);
                  break;
               case 'b':
                  theGPT.SaveGPTBackup(backupFile);
                  free(backupFile);
                  break;
               case 'c':
                  theGPT.JustLooking(0);
                  partNum = (int) GetInt(partName, 1) - 1;
                  if (theGPT.SetName(partNum, GetString(partName, 2))) {
                     saveData = 1;
                  } else {
                     cerr << "Unable to set partition " << partNum + 1
                          << "'s name to '" << GetString(partName, 2) << "'!\n";
                     neverSaveData = 1;
                  } // if/else
                  free(partName);
                  break;
               case 'C':
                  theGPT.JustLooking(0);
                  theGPT.RecomputeCHS();
                  saveData = 1;
                  break;
               case 'd':
                  theGPT.JustLooking(0);
                  if (theGPT.DeletePartition(deletePartNum - 1) == 0) {
                     cerr << "Error " << errno << " deleting partition!\n";
                     neverSaveData = 1;
                  } else saveData = 1;
                  break;
               case 'D':
                  cout << theGPT.GetAlignment() << "\n";
                  break;
               case 'e':
                  theGPT.JustLooking(0);
                  theGPT.MoveSecondHeaderToEnd();
                  saveData = 1;
                  break;
               case 'E':
                  cout << theGPT.FindLastInFree(theGPT.FindFirstInLargest()) << "\n";
                  break;
               case 'f':
                  cout << theGPT.FindFirstInLargest() << "\n";
                  break;
               case 'F':
                  temp = theGPT.FindFirstInLargest();
                  theGPT.Align(&temp);
                  cout << temp << "\n";
                  break;
               case 'g':
                  theGPT.JustLooking(0);
                  saveData = 1;
                  saveNonGPT = 1;
                  break;
               case 'G':
                  theGPT.JustLooking(0);
                  saveData = 1;
                  theGPT.RandomizeGUIDs();
                  break;
               case 'h':
                  theGPT.JustLooking(0);
                  if (BuildMBR(theGPT, hybrids, 1) == 1)
                     saveData = 1;
                  break;
               case 'i':
                  theGPT.ShowPartDetails(infoPartNum - 1);
                  break;
               case 'l':
                  if (theGPT.LoadGPTBackup((string) backupFile) == 1) {
                     theGPT.JustLooking(0);
                     saveData = 1;
                  } else {
                     saveData = 0;
                     neverSaveData = 1;
                     cerr << "Error loading backup file!\n";
                  } // else
                  free(backupFile);
                  break;
               case 'L':
                  break;
               case 'm':
                  theGPT.JustLooking(0);
                  if (BuildMBR(theGPT, mbrParts, 0) == 1) {
                     if (!pretend) {
                        if (theGPT.SaveMBR()) {
                           theGPT.DestroyGPT();
                        } else
                           cerr << "Problem saving MBR!\n";
                     } // if
                     saveNonGPT = 0;
                     pretend = 1; // Not really, but works around problem if -g is used with this...
                     saveData = 0;
                  } // if
                  break;
               case 'n':
                  theGPT.JustLooking(0);
                  partNum = (int) GetInt(newPartInfo, 1) - 1;
                  if (partNum < 0)
                     partNum = theGPT.FindFirstFreePart();
                  low = theGPT.FindFirstInLargest();
                  high = theGPT.FindLastInFree(low);
                  startSector = IeeeToInt(GetString(newPartInfo, 2), sSize, low, high, low);
                  endSector = IeeeToInt(GetString(newPartInfo, 3), sSize, startSector, high, high);
                  if (theGPT.CreatePartition(partNum, startSector, endSector)) {
                     saveData = 1;
                  } else {
                     cerr << "Could not create partition " << partNum + 1 << " from "
                          << startSector << " to " << endSector << "\n";
                     neverSaveData = 1;
                  } // if/else
                  free(newPartInfo);
                  break;
               case 'N':
                  theGPT.JustLooking(0);
                  startSector = theGPT.FindFirstInLargest();
                  endSector = theGPT.FindLastInFree(startSector);
                  if (largestPartNum < 0)
                     largestPartNum = theGPT.FindFirstFreePart();
                  if (theGPT.CreatePartition(largestPartNum - 1, startSector, endSector)) {
                     saveData = 1;
                  } else {
                     cerr << "Could not create partition " << largestPartNum << " from "
                          << startSector << " to " << endSector << "\n";
                     neverSaveData = 1;
                  } // if/else
                  break;
               case 'o':
                  theGPT.JustLooking(0);
                  theGPT.ClearGPTData();
                  saveData = 1;
                  break;
               case 'p':
                  theGPT.DisplayGPTData();
                  break;
               case 'P':
                  pretend = 1;
                  break;
               case 'r':
                  theGPT.JustLooking(0);
                  uint64_t p1, p2;
                  p1 = GetInt(twoParts, 1) - 1;
                  p2 = GetInt(twoParts, 2) - 1;
                  if (theGPT.SwapPartitions((uint32_t) p1, (uint32_t) p2) == 0) {
                     neverSaveData = 1;
                     cerr << "Cannot swap partitions " << p1 + 1 << " and " << p2 + 1 << "\n";
                  } else saveData = 1;
                  break;
               case 'R':
                  secondDevice = theGPT;
                  secondDevice.SetDisk(outDevice);
                  secondDevice.JustLooking(0);
                  secondDevice.SaveGPTData(1);
                  break;
               case 's':
                  theGPT.JustLooking(0);
                  theGPT.SortGPT();
                  saveData = 1;
                  break;
               case 'S':
                  theGPT.JustLooking(0);
                  if (theGPT.SetGPTSize(tableSize) == 0)
                     neverSaveData = 1;
                  else
                     saveData = 1;
                  break;
               case 't':
                  theGPT.JustLooking(0);
                  partNum = (int) GetInt(typeCode, 1) - 1;
                  typeHelper = GetString(typeCode, 2);
                  if ((typeHelper != (GUIDData) "00000000-0000-0000-0000-000000000000") &&
                      (theGPT.ChangePartType(partNum, typeHelper))) {
                     saveData = 1;
                  } else {
                     cerr << "Could not change partition " << partNum + 1
                          << "'s type code to " << GetString(typeCode, 2) << "!\n";
                     neverSaveData = 1;
                  } // if/else
                  free(typeCode);
                  break;
               case 'T':
                  theGPT.JustLooking(0);
                  theGPT.XFormDisklabel(bsdPartNum - 1);
                  saveData = 1;
                  break;
               case 'u':
                  theGPT.JustLooking(0);
                  saveData = 1;
                  gptPartNum = (int) GetInt(partGUID, 1) - 1;
                  theGPT.SetPartitionGUID(gptPartNum, GetString(partGUID, 2).c_str());
                  break;
               case 'U':
                  theGPT.JustLooking(0);
                  saveData = 1;
                  theGPT.SetDiskGUID(diskGUID);
                  break;
               case 'v':
                  theGPT.Verify();
                  break;
               case 'z':
                  if (!pretend) {
                     theGPT.DestroyGPT();
                  } // if
                  saveNonGPT = 0;
                  saveData = 0;
                  break;
               case 'Z':
                  if (!pretend) {
                     theGPT.DestroyGPT();
                     theGPT.DestroyMBR();
                  } // if
                  saveNonGPT = 0;
                  saveData = 0;
                  break;
               default:
                  cerr << "Unknown option (-" << opt << ")!\n";
                  break;
            } // switch
         } // while
         if ((saveData) && (!neverSaveData) && (saveNonGPT) && (!pretend)) {
            theGPT.SaveGPTData(1);
         }
         if (saveData && (!saveNonGPT)) {
            cout << "Non-GPT disk; not saving changes. Use -g to override.\n";
            retval = 3;
         } // if
         if (neverSaveData) {
            cerr << "Error encountered; not saving changes.\n";
            retval = 4;
         } // if
      } else { // if loaded OK
         poptResetContext(poptCon);
         // Do a few types of operations even if there are problems....
         while ((opt = poptGetNextOpt(poptCon)) > 0) {
            switch (opt) {
               case 'v':
                  cout << "Verification may miss some problems or report too many!\n";
                  theGPT.Verify();
                  break;
               case 'z':
                  if (!pretend) {
                     theGPT.DestroyGPT();
                  } // if
                  saveNonGPT = 0;
                  saveData = 0;
                  break;
               case 'Z':
                  if (!pretend) {
                     theGPT.DestroyGPT();
                     theGPT.DestroyMBR();
                  } // if
                  saveNonGPT = 0;
                  saveData = 0;
                  break;
            } // switch
         } // while
         retval = 2;
      } // if/else loaded OK
   } // if (device != NULL)
   poptFreeContext(poptCon);

   return retval;
} // main

// Create a hybrid or regular MBR from GPT data structures
int BuildMBR(GPTData & theGPT, char* argument, int isHybrid) {
   int numParts, allOK = 1, i, origPartNum;
   MBRPart newPart;
   BasicMBRData newMBR;

   if ((&theGPT != NULL) && (argument != NULL)) {
      numParts = CountColons(argument) + 1;
      cout << "numParts = " << numParts << "\n";
      if (numParts <= (4 - isHybrid)) {
         newMBR.SetDisk(theGPT.GetDisk());
         for (i = 0; i < numParts; i++) {
            origPartNum = GetInt(argument, i + 1) - 1;
            newPart.SetInclusion(PRIMARY);
            newPart.SetLocation(theGPT[origPartNum].GetFirstLBA(),
                                theGPT[origPartNum].GetLengthLBA());
            newPart.SetStatus(0);
            newPart.SetType((uint8_t)(theGPT[origPartNum].GetHexType() / 0x0100));
            newMBR.AddPart(i + isHybrid, newPart);
         } // for
         if (isHybrid) {
            newPart.SetInclusion(PRIMARY);
            newPart.SetLocation(1, newMBR.FindLastInFree(1));
            newPart.SetStatus(0);
            newPart.SetType(0xEE);
            newMBR.AddPart(0, newPart);
         } // if
         theGPT.SetProtectiveMBR(newMBR);
      } else allOK = 0;
   } else allOK = 0;
   if (!allOK)
      cerr << "Problem creating MBR!\n";
   return allOK;
} // BuildMBR()

// Returns the number of colons in argument string, ignoring the
// first character (thus, a leading colon is ignored, as GetString()
// does).
int CountColons(char* argument) {
   int num = 0;

   while ((argument[0] != '\0') && (argument = strchr(&argument[1], ':')))
      num++;

   return num;
} // CountColons()
