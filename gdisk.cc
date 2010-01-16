// gdisk.cc
// Program modelled after Linux fdisk, but it manipulates GPT partitions
// rather than MBR partitions.
//
// by Rod Smith, project began February 2009

/* This program is copyright (c) 2009, 2010 by Roderick W. Smith. It is distributed
  under the terms of the GNU GPL version 2, as detailed in the COPYING file. */

//#include <iostream>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include "mbr.h"
#include "gpt.h"
#include "support.h"

// Function prototypes....
void MainMenu(char* filename, struct GPTData* theGPT);
void ShowCommands(void);
void ExpertsMenu(char* filename, struct GPTData* theGPT);
void ShowExpertCommands(void);
void RecoveryMenu(char* filename, struct GPTData* theGPT);
void ShowRecoveryCommands(void);

int main(int argc, char* argv[]) {
   GPTData theGPT;
   int doMore = 1;
   char* device = NULL;

   printf("GPT fdisk (gdisk) version %s\n\n", GPTFDISK_VERSION);

    if (argc == 2) { // basic usage
      if (SizesOK()) {
         doMore = theGPT.LoadPartitions(argv[1]);
         if (doMore) {
            MainMenu(argv[1], &theGPT);
         } // if (doMore)
      } // if (SizesOK())
   } else if (argc == 3) { // usage with "-l" option
      if (SizesOK()) {
         if (strcmp(argv[1], "-l") == 0) {
            device = argv[2];
         } else if (strcmp(argv[2], "-l") == 0) {
            device = argv[1];
         } else { // 3 arguments, but none is "-l"
            fprintf(stderr, "Usage: %s [-l] device_file\n", argv[0]);
         } // if/elseif/else
         if (device != NULL) {
            theGPT.JustLooking();
            doMore = theGPT.LoadPartitions(device);
            if (doMore) theGPT.DisplayGPTData();
         } // if
      } // if
   } else {
      fprintf(stderr, "Usage: %s [-l] device_file\n", argv[0]);
   } // if/else
} // main

// Accept a command and execute it. Returns only when the user
// wants to exit (such as after a 'w' or 'q' command).
void MainMenu(char* filename, struct GPTData* theGPT) {
   char command, line[255], buFile[255];
   char* junk;
   int goOn = 1;
   PartTypes typeHelper;
   uint32_t temp1, temp2;

   do {
      printf("\nCommand (? for help): ");
      junk = fgets(line, 255, stdin);
      sscanf(line, "%c", &command);
      switch (command) {
         case 'b': case 'B':
            printf("Enter backup filename to save: ");
            junk = fgets(line, 255, stdin);
            sscanf(line, "%s", (char*) &buFile);
            theGPT->SaveGPTBackup(buFile);
            break;
         case 'c': case 'C':
            if (theGPT->GetPartRange(&temp1, &temp2) > 0)
               theGPT->SetName(theGPT->GetPartNum());
            else
               printf("No partitions\n");
            break;
         case 'd': case 'D':
            theGPT->DeletePartition();
            break;
         case 'i': case 'I':
            theGPT->ShowDetails();
            break;
         case 'l': case 'L':
            typeHelper.ShowTypes();
            break;
         case 'n': case 'N':
            theGPT->CreatePartition();
            break;
         case 'o': case 'O':
            printf("This option deletes all partitions and creates a new "
                  "protective MBR.\nProceed? ");
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
            printf("You may need to edit /etc/fstab and/or your boot loader configuration!\n");
            break;
         case 't': case 'T':
            theGPT->ChangePartType();
            break;
         case 'v': case 'V':
            if (theGPT->Verify() > 0) { // problems found
               printf("You may be able to correct the problems by using options on the experts\n"
                     "menu (press 'x' at the command prompt). Good luck!\n");
            } // if
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
   printf("b\tback up GPT data to a file\n");
   printf("c\tchange a partition's name\n");
   printf("d\tdelete a partition\n");
   printf("i\tshow detailed information on a partition\n");
   printf("l\tlist known partition types\n");
   printf("n\tadd a new partition\n");
   printf("o\tcreate a new empty GUID partition table (GPT)\n");
   printf("p\tprint the partition table\n");
   printf("q\tquit without saving changes\n");
   printf("r\trecovery and transformation options (experts only)\n");
   printf("s\tsort partitions\n");
   printf("t\tchange a partition's type code\n");
   printf("v\tverify disk\n");
   printf("w\twrite table to disk and exit\n");
   printf("x\textra functionality (experts only)\n");
   printf("?\tprint this menu\n");
} // ShowCommands()

// Accept a recovery & transformation menu command. Returns only when the user
// issues an exit command, such as 'w' or 'q'.
void RecoveryMenu(char* filename, struct GPTData* theGPT) {
   char command, line[255], buFile[255];
   char* junk;
   PartTypes typeHelper;
   uint32_t temp1;
   int goOn = 1;

   do {
      printf("\nRecovery/transformation command (? for help): ");
      junk = fgets(line, 255, stdin);
      sscanf(line, "%c", &command);
      switch (command) {
         case 'b': case 'B':
            theGPT->RebuildMainHeader();
            break;
         case 'c': case 'C':
            printf("Warning! This will probably do weird things if you've converted an MBR to\n"
                  "GPT form and haven't yet saved the GPT! Proceed? ");
            if (GetYN() == 'Y')
               theGPT->LoadSecondTableAsMain();
            break;
         case 'd': case 'D':
            theGPT->RebuildSecondHeader();
            break;
         case 'e': case 'E':
            printf("Warning! This will probably do weird things if you've converted an MBR to\n"
                  "GPT form and haven't yet saved the GPT! Proceed? ");
            if (GetYN() == 'Y')
               theGPT->LoadMainTable();
            break;
         case 'f': case 'F':
            printf("Warning! This will destroy the currently defined partitions! Proceed? ");
            if (GetYN() == 'Y') {
               if (theGPT->LoadMBR(filename) == 1) { // successful load
                  theGPT->XFormPartitions();
               } else {
                  printf("Problem loading MBR! GPT is untouched; regenerating protective MBR!\n");
                  theGPT->MakeProtectiveMBR();
               } // if/else
            } // if
            break;
         case 'g': case 'G':
            temp1 = theGPT->XFormToMBR();
            if (temp1 > 0) {
               printf("Converted %d partitions. Finalize and exit? ", temp1);
               if (GetYN() == 'Y') {
                  if (theGPT->DestroyGPT(0) > 0)
                     goOn = 0;
               } else {
                  theGPT->MakeProtectiveMBR();
                  printf("Note: New protective MBR created.\n");
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
            printf("Enter backup filename to load: ");
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
   printf("b\tuse backup GPT header (rebuilding main)\n");
   printf("c\tload backup partition table from disk (rebuilding main)\n");
   printf("d\tuse main GPT header (rebuilding backup)\n");
   printf("e\tload main partition table from disk (rebuilding backup)\n");
   printf("f\tload MBR and build fresh GPT from it\n");
   printf("g\tconvert GPT into MBR and exit\n");
   printf("h\tmake hybrid MBR\n");
   printf("i\tshow detailed information on a partition\n");
   printf("l\tload partition data from a backup file\n");
   printf("m\treturn to main menu\n");
   printf("o\tprint protective MBR data\n");
   printf("p\tprint the partition table\n");
   printf("q\tquit without saving changes\n");
   printf("t\ttransform BSD disklabel partition\n");
   printf("v\tverify disk\n");
   printf("w\twrite table to disk and exit\n");
   printf("x\textra functionality (experts only)\n");
   printf("?\tprint this menu\n");
} // ShowRecoveryCommands()

// Accept an experts' menu command. Returns only after the user
// selects an exit command, such as 'w' or 'q'.
void ExpertsMenu(char* filename, struct GPTData* theGPT) {
   char command, line[255];
   char* junk;
   PartTypes typeHelper;
   uint32_t pn;
   uint32_t temp1, temp2;
   int goOn = 1;

   do {
      printf("\nExpert command (? for help): ");
      junk = fgets(line, 255, stdin);
      sscanf(line, "%c", &command);
      switch (command) {
         case 'a': case 'A':
            if (theGPT->GetPartRange(&temp1, &temp2) > 0)
               theGPT->SetAttributes(theGPT->GetPartNum());
           else
               printf("No partitions\n");
            break;
         case 'c': case 'C':
            if (theGPT->GetPartRange(&temp1, &temp2) > 0) {
               pn = theGPT->GetPartNum();
               printf("Enter the partition's new unique GUID:\n");
               theGPT->SetPartitionGUID(pn, GetGUID());
            } else printf("No partitions\n");
            break;
         case 'd': case 'D':
            printf("Partitions will begin on %d-sector boundaries.\n",
                   theGPT->GetAlignment());
            break;
         case 'e': case 'E':
            printf("Relocating backup data structures to the end of the disk\n");
            theGPT->MoveSecondHeaderToEnd();
            break;
         case 'g': case 'G':
            printf("Enter the disk's unique GUID:\n");
            theGPT->SetDiskGUID(GetGUID());
            break;
         case 'i': case 'I':
            theGPT->ShowDetails();
            break;
         case 'l': case 'L':
            temp1 = GetNumber(1, 128, 8, "Enter the sector alignment value (1-128, default = 8): ");
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
         case 'v': case 'V':
            theGPT->Verify();
            break;
         case 'w': case 'W':
            if (theGPT->SaveGPTData() == 1) {
               goOn = 0;
            } // if
            break;
         case 'z': case 'Z':
            if (theGPT->DestroyGPT() == 1) {
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
   printf("a\tset attributes\n");
   printf("c\tchange partition GUID\n");
   printf("d\tdisplay the sector alignment value\n");
   printf("e\trelocate backup data structures to the end of the disk\n");
   printf("g\tchange disk GUID\n");
   printf("i\tshow detailed information on a partition\n");
   printf("l\tset the sector alignment value\n");
   printf("m\treturn to main menu\n");
   printf("n\tcreate a new protective MBR\n");
   printf("o\tprint protective MBR data\n");
   printf("p\tprint the partition table\n");
   printf("q\tquit without saving changes\n");
   printf("r\trecovery and transformation options (experts only)\n");
   printf("s\tresize partition table\n");
   printf("v\tverify disk\n");
   printf("w\twrite table to disk and exit\n");
   printf("z\tzap (destroy) GPT data structures and exit\n");
   printf("?\tprint this menu\n");
} // ShowExpertCommands()
