// gdisk.cc
// Program modelled after Linux fdisk, but it manipulates GPT partitions
// rather than MBR partitions.
//
// by Rod Smith, February 2009

//#include <iostream>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include "mbr.h"
#include "gpt.h"
#include "support.h"

// Function prototypes....
// int ReadPartitions(char* filename, struct GPTData* theGPT);
int DoCommand(char* filename, struct GPTData* theGPT);
void ShowCommands(void);
void ShowExpertCommands(void);
int ExpertsMenu(char* filename, struct GPTData* theGPT);

int main(int argc, char* argv[]) {
   GPTData theGPT;
   int doMore = 1;
   char* device = NULL;

   printf("GPT fdisk (gdisk) version 0.3.1\n\n");

    if (argc == 2) { // basic usage
      if (SizesOK()) {
         doMore = theGPT.LoadPartitions(argv[1]);
         while (doMore) {
            doMore = DoCommand(argv[1], &theGPT);
         } // while
      } // if
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
            doMore = theGPT.LoadPartitions(device);
            if (doMore) theGPT.DisplayGPTData();
         } // if
      } // if
   } else {
      fprintf(stderr, "Usage: %s [-l] device_file\n", argv[0]);
   } // if/else
} // main

// Accept a command and execute it. Returns 0 if the command includes
// an exit condition (such as a q or w command), 1 if more commands
// should be processed.
int DoCommand(char* filename, struct GPTData* theGPT) {
   char command, line[255];
   int retval = 1;
   PartTypes typeHelper;
   uint32_t temp1, temp2;

   printf("\nCommand (m for help): ");
   fgets(line, 255, stdin);
   sscanf(line, "%c", &command);
   switch (command) {
/*      case 'b': case 'B':
         GetGUID();
	 break; */
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
         theGPT->ClearGPTData();
//         theGPT->protectiveMBR.MakeProtectiveMBR();
//         theGPT->BlankPartitions();
         break;
      case 'p': case 'P':
         theGPT->DisplayGPTData();
	 break;
      case 'q': case 'Q':
         retval = 0;
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
            retval = 0;
         break;
      case 'x': case 'X':
         retval = ExpertsMenu(filename, theGPT);
         break;
      default:
         ShowCommands();
         break;
   } // switch
   return (retval);
} // DoCommand()

void ShowCommands(void) {
   printf("c\tchange a partition's name\n");
   printf("d\tdelete a partition\n");
   printf("i\tshow detailed information on a partition\n");
   printf("l\tlist available partition types\n");
   printf("m\tprint this menu\n");
   printf("n\tadd a new partition\n");
   printf("o\tcreate a new empty GUID partition table (GPT)\n");
   printf("p\tprint the partition table\n");
   printf("q\tquit without saving changes\n");
   printf("s\tsort partitions\n");
   printf("t\tchange a partition's type code\n");
   printf("v\tverify disk\n");
   printf("w\twrite table to disk and exit\n");
   printf("x\textra functionality (experts only)\n");
} // ShowCommands()

// Accept a command and execute it. Returns 0 if the command includes
// an exit condition (such as a q or w command), 1 if more commands
// should be processed.
int ExpertsMenu(char* filename, struct GPTData* theGPT) {
   char command, line[255], buFile[255];
   int retval = 1;
   PartTypes typeHelper;
   uint32_t pn;
   uint32_t temp1, temp2;
   int goOn = 1;

   do {
      printf("\nExpert command (m for help): ");
      fgets(line, 255, stdin);
      sscanf(line, "%c", &command);
      switch (command) {
         case 'a': case 'A':
            if (theGPT->GetPartRange(&temp1, &temp2) > 0)
               theGPT->SetAttributes(theGPT->GetPartNum());
           else
               printf("No partitions\n");
            break;
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
            if (theGPT->GetPartRange(&temp1, &temp2) > 0) {
               pn = theGPT->GetPartNum();
               printf("Enter the partition's new unique GUID:\n");
               theGPT->SetPartitionGUID(pn, GetGUID());
            } else printf("No partitions\n");
            break;
         case 'g': case 'G':
            printf("Enter the disk's unique GUID:\n");
            theGPT->SetDiskGUID(GetGUID());
            break;
/*         case 'h': case 'H':
            theGPT->MakeHybrid();
            break; */
         case 'i': case 'I':
            theGPT->ShowDetails();
            break;
         case 'k': case 'K':
            printf("Enter backup filename to save: ");
            fgets(line, 255, stdin);
            sscanf(line, "%s", &buFile);
	    theGPT->SaveGPTBackup(buFile);
            break;
         case 'l': case 'L':
            printf("Enter backup filename to load: ");
            fgets(line, 255, stdin);
            sscanf(line, "%s", &buFile);
	    theGPT->LoadGPTBackup(buFile);
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
            retval = 0;
	    goOn = 0;
	    break;
         case 'r': case 'R':
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
               retval = 0;
               goOn = 0;
            } // if
            break;
         default:
            ShowExpertCommands();
            break;
      } // switch
   } while (goOn);
   return (retval);
} // ExpertsMenu()

void ShowExpertCommands(void) {
   printf("a\tset attributes\n");
   printf("b\tuse backup GPT header (rebuilding main)\n");
   printf("c\tload backup partition table from disk (rebuilding main)\n");
   printf("d\tuse main GPT header (rebuilding backup)\n");
   printf("e\tload main partition table from disk (rebuilding backup)\n");
   printf("f\tchange partition GUID\n");
   printf("g\tchange disk GUID\n");
//   printf("h\tmake hybrid MBR\n");
   printf("i\tshow detailed information on a partition\n");
   printf("k\tsave partition data to a backup file\n");
   printf("l\tload partition data from a backup file\n");
   printf("m\tprint this menu\n");
   printf("n\tcreate a new protective MBR\n");
   printf("o\tprint protective MBR data\n");
   printf("p\tprint the partition table\n");
   printf("q\tquit without saving changes\n");
   printf("r\treturn to main menu\n");
   printf("s\tresize partition table\n");
   printf("v\tverify disk\n");
   printf("w\twrite table to disk and exit\n");
} // ShowExpertCommands()
