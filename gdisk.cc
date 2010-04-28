// gdisk.cc
// Program modelled after Linux fdisk, but it manipulates GPT partitions
// rather than MBR partitions.
//
// by Rod Smith, project began February 2009

/* This program is copyright (c) 2009, 2010 by Roderick W. Smith. It is distributed
  under the terms of the GNU GPL version 2, as detailed in the COPYING file. */

#include <stdio.h>
//#include <getopt.h>
#include <string.h>
#include <string>
#include <iostream>
#include <sstream>
#include <locale>
#include "mbr.h"
#include "gpttext.h"
#include "support.h"

// Function prototypes....
void MainMenu(string filename, GPTDataTextUI* theGPT);
void ShowCommands(void);
void ExpertsMenu(string filename, GPTDataTextUI* theGPT);
void ShowExpertCommands(void);
void RecoveryMenu(string filename, GPTDataTextUI* theGPT);
void ShowRecoveryCommands(void);
void WinWarning(void);

int main(int argc, char* argv[]) {
   GPTDataTextUI theGPT;
   int doMore = 1, i;
   char *device = NULL, *junk;

   cout << "GPT fdisk (gdisk) version " << GPTFDISK_VERSION << "\n\n";

   if (!SizesOK())
      exit(1);

   switch (argc) {
      case 1:
         WinWarning();
         cout << "Type device filename, or press <Enter> to exit: ";
         device = new char[255];
         junk = fgets(device, 255, stdin);
         if (device[0] != '\n') {
            i = strlen(device);
            if (i > 0)
               if (device[i - 1] == '\n')
                  device[i - 1] = '\0';
            doMore = theGPT.LoadPartitions(device);
            if (doMore) {
               MainMenu(device, &theGPT);
            } // if (doMore)
         } // if
         delete[] device;
         break;
      case 2: // basic usage
         WinWarning();
         doMore = theGPT.LoadPartitions(argv[1]);
         if (doMore) {
            MainMenu(argv[1], &theGPT);
         } // if (doMore)
         break;
      case 3: // usage with "-l" option
         if (strcmp(argv[1], "-l") == 0) {
            device = argv[2];
         } else if (strcmp(argv[2], "-l") == 0) {
            device = argv[1];
         } else { // 3 arguments, but none is "-l"
            cerr << "Usage: " << argv[0] << " [-l] device_file\n";
         } // if/elseif/else
         if (device != NULL) {
            theGPT.JustLooking();
            doMore = theGPT.LoadPartitions((string) device);
            if (doMore) theGPT.DisplayGPTData();
         } // if
         break;
      default:
         cerr << "Usage: " << argv[0] << " [-l] device_file\n";
         break;
   } // switch
} // main

// Accept a command and execute it. Returns only when the user
// wants to exit (such as after a 'w' or 'q' command).
void MainMenu(string filename, GPTDataTextUI* theGPT) {
   char command, line[255], buFile[255];
   char* junk;
   int goOn = 1;
   PartType typeHelper;
   uint32_t temp1, temp2;

   do {
      cout << "\nCommand (? for help): ";
      junk = fgets(line, 255, stdin);
      sscanf(line, "%c", &command);
      switch (command) {
         case '\n':
            break;
         case 'b': case 'B':
            cout << "Enter backup filename to save: ";
            junk = fgets(line, 255, stdin);
            sscanf(line, "%s", (char*) &buFile);
            theGPT->SaveGPTBackup(buFile);
            break;
         case 'c': case 'C':
            if (theGPT->GetPartRange(&temp1, &temp2) > 0)
               theGPT->SetName(theGPT->GetPartNum());
            else
               cout << "No partitions\n";
            break;
         case 'd': case 'D':
            theGPT->DeletePartition();
            break;
         case 'i': case 'I':
            theGPT->ShowDetails();
            break;
         case 'l': case 'L':
            typeHelper.ShowAllTypes();
            break;
         case 'n': case 'N':
            theGPT->CreatePartition();
            break;
         case 'o': case 'O':
            cout << "This option deletes all partitions and creates a new protective MBR.\n"
                 << "Proceed? ";
            if (GetYN() == 'Y') {
               theGPT->ClearGPTData();
               theGPT->MakeProtectiveMBR();
            } // if
            break;
         case 'p': case 'P':
            theGPT->DisplayGPTData();
            break;
         case 'q': case 'Q':
            goOn = 0;
            break;
         case 'r': case 'R':
            RecoveryMenu(filename, theGPT);
            goOn = 0;
            break;
         case 's': case 'S':
            theGPT->SortGPT();
            cout << "You may need to edit /etc/fstab and/or your boot loader configuration!\n";
            break;
         case 't': case 'T':
            theGPT->ChangePartType();
            break;
         case 'v': case 'V':
            theGPT->Verify();
            break;
         case 'w': case 'W':
            if (theGPT->SaveGPTData() == 1)
               goOn = 0;
            break;
         case 'x': case 'X':
            ExpertsMenu(filename, theGPT);
            goOn = 0;
            break;
         default:
            ShowCommands();
            break;
      } // switch
   } while (goOn);
} // MainMenu()

void ShowCommands(void) {
   cout << "b\tback up GPT data to a file\n";
   cout << "c\tchange a partition's name\n";
   cout << "d\tdelete a partition\n";
   cout << "i\tshow detailed information on a partition\n";
   cout << "l\tlist known partition types\n";
   cout << "n\tadd a new partition\n";
   cout << "o\tcreate a new empty GUID partition table (GPT)\n";
   cout << "p\tprint the partition table\n";
   cout << "q\tquit without saving changes\n";
   cout << "r\trecovery and transformation options (experts only)\n";
   cout << "s\tsort partitions\n";
   cout << "t\tchange a partition's type code\n";
   cout << "v\tverify disk\n";
   cout << "w\twrite table to disk and exit\n";
   cout << "x\textra functionality (experts only)\n";
   cout << "?\tprint this menu\n";
} // ShowCommands()

// Accept a recovery & transformation menu command. Returns only when the user
// issues an exit command, such as 'w' or 'q'.
void RecoveryMenu(string filename, GPTDataTextUI* theGPT) {
   char command, line[255], buFile[255];
   char* junk;
   uint32_t temp1, numParts;
   int goOn = 1;

   do {
      cout << "\nRecovery/transformation command (? for help): ";
      junk = fgets(line, 255, stdin);
      sscanf(line, "%c", &command);
      switch (command) {
         case '\n':
            break;
         case 'b': case 'B':
            theGPT->RebuildMainHeader();
            break;
         case 'c': case 'C':
            cout << "Warning! This will probably do weird things if you've converted an MBR to\n"
                 << "GPT form and haven't yet saved the GPT! Proceed? ";
            if (GetYN() == 'Y')
               theGPT->LoadSecondTableAsMain();
            break;
         case 'd': case 'D':
            theGPT->RebuildSecondHeader();
            break;
         case 'e': case 'E':
            cout << "Warning! This will probably do weird things if you've converted an MBR to\n"
                 << "GPT form and haven't yet saved the GPT! Proceed? ";
            if (GetYN() == 'Y')
               theGPT->LoadMainTable();
            break;
         case 'f': case 'F':
            cout << "Warning! This will destroy the currently defined partitions! Proceed? ";
            if (GetYN() == 'Y') {
               if (theGPT->LoadMBR(filename) == 1) { // successful load
                  theGPT->XFormPartitions();
               } else {
                  cout << "Problem loading MBR! GPT is untouched; regenerating protective MBR!\n";
                  theGPT->MakeProtectiveMBR();
               } // if/else
            } // if
            break;
         case 'g': case 'G':
            numParts = theGPT->GetNumParts();
            temp1 = theGPT->XFormToMBR();
            if (temp1 > 0) {
               cout << "\nConverted " << temp1 << " partitions. Finalize and exit? ";
               if (GetYN() == 'Y') {
                  if ((theGPT->DestroyGPT() > 0) && (theGPT->SaveMBR()))
                     goOn = 0;
               } else {
                  theGPT->MakeProtectiveMBR();
                  theGPT->SetGPTSize(numParts);
                  cout << "Note: New protective MBR created\n\n";
               } // if/else
            } // if
            break;
         case 'h': case 'H':
            theGPT->MakeHybrid();
            break;
         case 'i': case 'I':
            theGPT->ShowDetails();
            break;
         case 'l': case 'L':
            cout << "Enter backup filename to load: ";
            junk = fgets(line, 255, stdin);
            sscanf(line, "%s", (char*) &buFile);
            theGPT->LoadGPTBackup(buFile);
            break;
         case 'm': case 'M':
            MainMenu(filename, theGPT);
            goOn = 0;
            break;
         case 'o': case 'O':
            theGPT->DisplayMBRData();
            break;
         case 'p': case 'P':
            theGPT->DisplayGPTData();
            break;
         case 'q': case 'Q':
            goOn = 0;
            break;
         case 't': case 'T':
            theGPT->XFormDisklabel();
            break;
         case 'v': case 'V':
            theGPT->Verify();
            break;
         case 'w': case 'W':
            if (theGPT->SaveGPTData() == 1) {
               goOn = 0;
            } // if
            break;
         case 'x': case 'X':
            ExpertsMenu(filename, theGPT);
            goOn = 0;
            break;
         default:
            ShowRecoveryCommands();
            break;
      } // switch
   } while (goOn);
} // RecoveryMenu()

void ShowRecoveryCommands(void) {
   cout << "b\tuse backup GPT header (rebuilding main)\n";
   cout << "c\tload backup partition table from disk (rebuilding main)\n";
   cout << "d\tuse main GPT header (rebuilding backup)\n";
   cout << "e\tload main partition table from disk (rebuilding backup)\n";
   cout << "f\tload MBR and build fresh GPT from it\n";
   cout << "g\tconvert GPT into MBR and exit\n";
   cout << "h\tmake hybrid MBR\n";
   cout << "i\tshow detailed information on a partition\n";
   cout << "l\tload partition data from a backup file\n";
   cout << "m\treturn to main menu\n";
   cout << "o\tprint protective MBR data\n";
   cout << "p\tprint the partition table\n";
   cout << "q\tquit without saving changes\n";
   cout << "t\ttransform BSD disklabel partition\n";
   cout << "v\tverify disk\n";
   cout << "w\twrite table to disk and exit\n";
   cout << "x\textra functionality (experts only)\n";
   cout << "?\tprint this menu\n";
} // ShowRecoveryCommands()

// Accept an experts' menu command. Returns only after the user
// selects an exit command, such as 'w' or 'q'.
void ExpertsMenu(string filename, GPTDataTextUI* theGPT) {
   char command, line[255];
   char* junk;
   uint32_t pn, temp1, temp2;
   int goOn = 1;
   GUIDData aGUID;
   ostringstream prompt;

   do {
      cout << "\nExpert command (? for help): ";
      junk = fgets(line, 255, stdin);
      sscanf(line, "%c", &command);
      switch (command) {
         case '\n':
            break;
         case 'a': case 'A':
            if (theGPT->GetPartRange(&temp1, &temp2) > 0)
               theGPT->SetAttributes(theGPT->GetPartNum());
           else
               cout << "No partitions\n";
            break;
         case 'c': case 'C':
            if (theGPT->GetPartRange(&temp1, &temp2) > 0) {
               pn = theGPT->GetPartNum();
               cout << "Enter the partition's new unique GUID:\n";
               theGPT->SetPartitionGUID(pn, aGUID.GetGUIDFromUser());
            } else cout << "No partitions\n";
            break;
         case 'd': case 'D':
            cout << "Partitions will begin on " << theGPT->GetAlignment()
                 << "-sector boundaries.\n";
            break;
         case 'e': case 'E':
            cout << "Relocating backup data structures to the end of the disk\n";
            theGPT->MoveSecondHeaderToEnd();
            break;
         case 'g': case 'G':
            cout << "Enter the disk's unique GUID:\n";
            theGPT->SetDiskGUID(aGUID.GetGUIDFromUser());
            break;
         case 'i': case 'I':
            theGPT->ShowDetails();
            break;
         case 'l': case 'L':
            prompt.seekp(0);
            prompt << "Enter the sector alignment value (1-" << MAX_ALIGNMENT << ", default = "
                   << DEFAULT_ALIGNMENT << "): ";
            temp1 = GetNumber(1, MAX_ALIGNMENT, DEFAULT_ALIGNMENT, prompt.str());
            theGPT->SetAlignment(temp1);
            break;
         case 'm': case 'M':
            MainMenu(filename, theGPT);
            goOn = 0;
            break;
         case 'n': case 'N':
            theGPT->MakeProtectiveMBR();
            break;
         case 'o': case 'O':
            theGPT->DisplayMBRData();
            break;
         case 'p': case 'P':
            theGPT->DisplayGPTData();
            break;
         case 'q': case 'Q':
	         goOn = 0;
	         break;
         case 'r': case 'R':
            RecoveryMenu(filename, theGPT);
            goOn = 0;
            break;
         case 's': case 'S':
            theGPT->ResizePartitionTable();
            break;
	      case 't': case 'T':
	         theGPT->SwapPartitions();
	         break;
         case 'v': case 'V':
            theGPT->Verify();
            break;
         case 'w': case 'W':
            if (theGPT->SaveGPTData() == 1) {
               goOn = 0;
            } // if
            break;
         case 'z': case 'Z':
            if (theGPT->DestroyGPTwPrompt() == 1) {
               goOn = 0;
            }
            break;
         default:
            ShowExpertCommands();
            break;
      } // switch
   } while (goOn);
} // ExpertsMenu()

void ShowExpertCommands(void) {
   cout << "a\tset attributes\n";
   cout << "c\tchange partition GUID\n";
   cout << "d\tdisplay the sector alignment value\n";
   cout << "e\trelocate backup data structures to the end of the disk\n";
   cout << "g\tchange disk GUID\n";
   cout << "i\tshow detailed information on a partition\n";
   cout << "l\tset the sector alignment value\n";
   cout << "m\treturn to main menu\n";
   cout << "n\tcreate a new protective MBR\n";
   cout << "o\tprint protective MBR data\n";
   cout << "p\tprint the partition table\n";
   cout << "q\tquit without saving changes\n";
   cout << "r\trecovery and transformation options (experts only)\n";
   cout << "s\tresize partition table\n";
   cout << "t\ttranspose two partition table entries\n";
   cout << "v\tverify disk\n";
   cout << "w\twrite table to disk and exit\n";
   cout << "z\tzap (destroy) GPT data structures and exit\n";
   cout << "?\tprint this menu\n";
} // ShowExpertCommands()

// On Windows, display a warning and ask whether to continue. If the user elects
// not to continue, exit immediately.
void WinWarning(void) {
#ifdef _WIN32
   cout << "\a************************************************************************\n"
   << "Most versions of Windows cannot boot from a GPT disk, and most varieties\n"
   << "prior to Vista cannot read GPT disks. Therefore, you should exit now\n"
   << "unless you understand the implications of converting MBR to GPT, editing\n"
   << "an existing GPT disk, or creating a new GPT disk layout!\n"
   << "************************************************************************\n\n";
   cout << "Are you SURE you want to continue? ";
   if (GetYN() != 'Y')
      exit(0);
#endif
} // WinWarning()
