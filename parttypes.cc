// parttypes.cc
// Class to manage partition type codes -- a slight variant on MBR type
// codes, GUID type codes, and associated names.

/* This program is copyright (c) 2009 by Roderick W. Smith. It is distributed
  under the terms of the GNU GPL version 2, as detailed in the COPYING file. */

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include "parttypes.h"

using namespace std;

int PartTypes::numInstances = 0;
AType* PartTypes::allTypes = NULL;
AType* PartTypes::lastType = NULL;

// Constructor. Its main task is to initialize the data list, but only
// if this is the first instance, since it's a static linked list.
// Partition type codes are MBR type codes multiplied by 0x0100, with
// additional related codes taking on following numbers. For instance,
// the FreeBSD disklabel code in MBR is 0xa5; here, it's 0xa500, with
// additional FreeBSD codes being 0xa501, 0xa502, and so on. This gives
// related codes similar numbers and (given appropriate entry positions
// in the linked list) keeps them together in the listings generated
// by typing "L" at the main gdisk menu.
// See http://www.win.tue.nl/~aeb/partitions/partition_types-1.html
// for a list of MBR partition type codes.
PartTypes::PartTypes(void) {

   numInstances++;
   if (numInstances == 1) {

      // Start with the "unused entry," which should normally appear only
      // on empty partition table entries....
      AddType(0x0000, UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000),
              "Unused entry", 0);

      // DOS/Windows partition types, which confusingly Linux also uses in GPT
      AddType(0x0100, UINT64_C(0x4433B9E5EBD0A0A2), UINT64_C(0xC79926B7B668C087),
              "Linux/Windows data", 0); // FAT-12
      AddType(0x0400, UINT64_C(0x4433B9E5EBD0A0A2), UINT64_C(0xC79926B7B668C087),
              "Linux/Windows data", 0); // FAT-16 < 32M
      AddType(0x0600, UINT64_C(0x4433B9E5EBD0A0A2), UINT64_C(0xC79926B7B668C087),
              "Linux/Windows data", 0); // FAT-16
      AddType(0x0700, UINT64_C(0x4433B9E5EBD0A0A2), UINT64_C(0xC79926B7B668C087),
              "Linux/Windows data", 1); // NTFS (or could be HPFS)
      AddType(0x0b00, UINT64_C(0x4433B9E5EBD0A0A2), UINT64_C(0xC79926B7B668C087),
              "Linux/Windows data", 0); // FAT-32
      AddType(0x0c00, UINT64_C(0x4433B9E5EBD0A0A2), UINT64_C(0xC79926B7B668C087),
              "Linux/Windows data", 0); // FAT-32 LBA
      AddType(0x0c01, UINT64_C(0x4DB80B5CE3C9E316), UINT64_C(0xAE1502F02DF97D81),
              "Microsoft Reserved"); // Microsoft reserved
      AddType(0x0e00, UINT64_C(0x4433B9E5EBD0A0A2), UINT64_C(0xC79926B7B668C087),
              "Linux/Windows data", 0); // FAT-16 LBA
      AddType(0x1100, UINT64_C(0x4433B9E5EBD0A0A2), UINT64_C(0xC79926B7B668C087),
              "Linux/Windows data", 0); // Hidden FAT-12
      AddType(0x1400, UINT64_C(0x4433B9E5EBD0A0A2), UINT64_C(0xC79926B7B668C087),
              "Linux/Windows data", 0); // Hidden FAT-16 < 32M
      AddType(0x1600, UINT64_C(0x4433B9E5EBD0A0A2), UINT64_C(0xC79926B7B668C087),
              "Linux/Windows data", 0); // Hidden FAT-16
      AddType(0x1700, UINT64_C(0x4433B9E5EBD0A0A2), UINT64_C(0xC79926B7B668C087),
              "Linux/Windows data", 0); // Hidden NTFS (or could be HPFS)
      AddType(0x1b00, UINT64_C(0x4433B9E5EBD0A0A2), UINT64_C(0xC79926B7B668C087),
              "Linux/Windows data", 0); // Hidden FAT-32
      AddType(0x1c00, UINT64_C(0x4433B9E5EBD0A0A2), UINT64_C(0xC79926B7B668C087),
              "Linux/Windows data", 0); // Hidden FAT-32 LBA
      AddType(0x1e00, UINT64_C(0x4433B9E5EBD0A0A2), UINT64_C(0xC79926B7B668C087),
              "Linux/Windows data", 0); // Hidden FAT-16 LBA
      AddType(0x2700, UINT64_C(0x4D4006D1DE94BBA4), UINT64_C(0xACD67901D5BF6AA1),
              "Windows RE"); // Windows RE
      AddType(0x4200, UINT64_C(0x4F621431Af9B60A0), UINT64_C(0xAD694A71113368BC),
              "Windows LDM data"); // Logical disk manager
      AddType(0x4201, UINT64_C(0x42E07E8F5808C8AA), UINT64_C(0xB3CF3404E9E1D285),
              "Windows LDM metadata"); // Logical disk manager

      // Linux-specific partition types....
      AddType(0x8200, UINT64_C(0x43C4A4AB0657FD6D), UINT64_C(0x4F4F4BC83309E584),
              "Linux swap"); // Linux swap (or could be Solaris)
      AddType(0x8300, UINT64_C(0x4433B9E5EBD0A0A2), UINT64_C(0xC79926B7B668C087),
              "Linux/Windows data", 0); // Linux native
      AddType(0x8301, UINT64_C(0x60C000078DA63339), UINT64_C(0x080923C83A0836C4),
              "Linux Reserved"); // Linux reserved
      AddType(0x8e00, UINT64_C(0x44C2F507E6D6D379), UINT64_C(0x28F93D2A8F233CA2),
              "Linux LVM"); // Linux LVM

      // FreeBSD partition types....
      // Note: Rather than extract FreeBSD disklabel data, convert FreeBSD
      // partitions in-place, and let FreeBSD sort out the details....
      AddType(0xa500, UINT64_C(0x11D66ECF516E7CB4), UINT64_C(0x2B71092D0200F88F),
              "FreeBSD disklabel"); // FreeBSD disklabel
      AddType(0xa501, UINT64_C(0x11DC7F4183BD6B9D), UINT64_C(0x0F4FB86015000BBE),
              "FreeBSD boot"); // FreeBSD boot
      AddType(0xa502, UINT64_C(0x11D66ECF516E7CB5), UINT64_C(0x2B71092D0200F88F),
              "FreeBSD swap"); // FreeBSD swap
      AddType(0xa503, UINT64_C(0x11D66ECF516E7CB6), UINT64_C(0x2B71092D0200F88F),
              "FreeBSD UFS"); // FreeBSD UFS
      AddType(0xa504, UINT64_C(0x11D66ECF516E7CBA), UINT64_C(0x2B71092D0200F88F),
              "FreeBSD ZFS"); // FreeBSD ZFS
      AddType(0xa505, UINT64_C(0x11D66ECF516E7CB8), UINT64_C(0x2B71092D0200F88F),
              "FreeBSD Vinum/RAID"); // FreeBSD Vinum

      // A MacOS partition type, separated from others by NetBSD partition types...
      AddType(0xa800, UINT64_C(0x11AA000055465300), UINT64_C(0xACEC4365300011AA),
              "Apple UFS"); // MacOS X

      // NetBSD partition types. Note that the main entry sets it up as a
      // FreeBSD disklabel. I'm not 100% certain this is the correct behavior.
      AddType(0xa900, UINT64_C(0x11D66ECF516E7CB4), UINT64_C(0x2B71092D0200F88F),
              "FreeBSD disklabel", 0); // NetBSD disklabel
      AddType(0xa901, UINT64_C(0x11DCB10E49F48D32), UINT64_C(0x489687D119009BB9),
              "NetBSD swap");
      AddType(0xa902, UINT64_C(0x11DCB10E49F48D5A), UINT64_C(0x489687D119009BB9),
              "NetBSD FFS");
      AddType(0xa903, UINT64_C(0x11DCB10E49F48D82), UINT64_C(0x489687D119009BB9),
              "NetBSD LFS");
      AddType(0xa903, UINT64_C(0x11DCB10E49F48DAA), UINT64_C(0x489687D119009BB9),
              "NetBSD RAID");
      AddType(0xa904, UINT64_C(0x11DCB10F2DB519C4), UINT64_C(0x489687D119009BB9),
              "NetBSD concatenated");
      AddType(0xa905, UINT64_C(0x11DCB10F2DB519EC), UINT64_C(0x489687D119009BB9),
              "NetBSD encrypted");

      // MacOS partition types (See also 0xa800, above)....
      AddType(0xab00, UINT64_C(0x11AA0000426F6F74), UINT64_C(0xACEC4365300011AA),
              "Apple boot"); // MacOS X
      AddType(0xaf00, UINT64_C(0x11AA000048465300), UINT64_C(0xACEC4365300011AA),
              "Apple HFS/HFS+"); // MacOS X
      AddType(0xaf01, UINT64_C(0x11AA000052414944), UINT64_C(0xACEC4365300011AA),
              "Apple RAID"); // MacOS X
      AddType(0xaf02, UINT64_C(0x11AA5F4F52414944), UINT64_C(0xACEC4365300011AA),
              "Apple RAID offline"); // MacOS X
      AddType(0xaf03, UINT64_C(0x11AA6C004C616265), UINT64_C(0xACEC4365300011AA),
              "Apple label"); // MacOS X
      AddType(0xaf04, UINT64_C(0x11AA76655265636F), UINT64_C(0xACEC4365300011AA),
              "AppleTV recovery"); // MacOS X

      // Solaris partition types (one of which is shared with MacOS)
      AddType(0xbe00, UINT64_C(0x11B21DD26A82CB45), UINT64_C(0x316673200008A699),
              "Solaris boot"); // Solaris boot
      AddType(0xbf00, UINT64_C(0x11B21DD26a85CF4D), UINT64_C(0x316673200008A699),
              "Solaris root"); // Solaris root
      AddType(0xbf01, UINT64_C(0x11B21DD26A898CC3), UINT64_C(0x316673200008A699),
              "Solaris /usr & Mac ZFS"); // MacOS X & Solaris
      AddType(0xbf02, UINT64_C(0x11B21DD26A87C46F), UINT64_C(0x316673200008A699),
              "Solaris swap");
      AddType(0xbf03, UINT64_C(0x11B21DD26A8B642B), UINT64_C(0x316673200008A699),
              "Solaris backup");
      AddType(0xbf04, UINT64_C(0x11B21DD26A8EF2E9), UINT64_C(0x316673200008A699),
              "Solaris /var");
      AddType(0xbf05, UINT64_C(0x11B21DD26A90BA39), UINT64_C(0x316673200008A699),
              "Solaris /home");
      AddType(0xbf05, UINT64_C(0x11B21DD26A9283A5), UINT64_C(0x316673200008A699),
              "Solaris EFI_ALTSCTR");
      AddType(0xbf06, UINT64_C(0x11B21DD26A945A3B), UINT64_C(0x316673200008A699),
              "Solaris Reserved 1");
      AddType(0xbf07, UINT64_C(0x11B21DD26A9630D1), UINT64_C(0x316673200008A699),
              "Solaris Reserved 2");
      AddType(0xbf08, UINT64_C(0x11B21DD26A980767), UINT64_C(0x316673200008A699),
              "Solaris Reserved 3");
      AddType(0xbf09, UINT64_C(0x11B21DD26A96237F), UINT64_C(0x316673200008A699),
              "Solaris Reserved 4");
      AddType(0xbf0a, UINT64_C(0x11B21DD26A8D2AC7), UINT64_C(0x316673200008A699),
              "Solaris Reserved 5");

      // I can find no MBR equivalents for these, but they're on the
      // Wikipedia page for GPT, so here we go....
      AddType(0xc001, UINT64_C(0x11D33AEB75894C1E), UINT64_C(0x000000A0037BC1B7),
              "HP-UX data");
      AddType(0xc002, UINT64_C(0x11D632E3E2A1E728), UINT64_C(0x000000A0037B82A6),
              "HP-UX service");

      // EFI system and related partitions
      AddType(0xEF00, UINT64_C(0x11d2f81fc12a7328), UINT64_C(0x3bc93ec9a0004bba),
              "EFI System"); // EFI System (parted marks Linux boot
                             // partitions like this)
      AddType(0xEF01, UINT64_C(0x11d333e7024dee41), UINT64_C(0x9FF381C70800699d),
              "MBR partition scheme"); // Used to nest an MBR table on a GPT disk
      AddType(0xEF02, UINT64_C(0x6E6F644921686148), UINT64_C(0x4946456465654E74),
              "BIOS boot partition"); //

      // A straggler Linux partition type....
      AddType(0xfd00, UINT64_C(0x4D3B05FCA19D880F), UINT64_C(0x1E91840F3F7406A0),
              "Linux RAID"); // Linux RAID
   } // if
} // default constructor

PartTypes::~PartTypes(void) {
   AType* tempType;

   numInstances--;
   if (numInstances == 0) {
      while (allTypes != NULL) {
         tempType = allTypes;
         allTypes = allTypes->next;
         delete tempType;
      } // while
   } // if
} // destructor

// Add a single type to the linked list of types. Returns 1 if operation
// succeeds, 0 otherwise.
int PartTypes::AddType(uint16_t mbrType, uint64_t guidData1, uint64_t guidData2,
                       const char * n, int toDisplay) {
   AType* tempType;
   int allOK = 1;

   tempType = new AType;
   if (tempType != NULL) {
      tempType->MBRType = mbrType;
      tempType->GUIDType.data1 = guidData1;
      tempType->GUIDType.data2 = guidData2;
      tempType->name = n;
      tempType->display = toDisplay;
      tempType->next = NULL;
      if (allTypes == NULL) { // first entry
         allTypes = tempType;
      } else {
         lastType->next = tempType;
      } // if/else
      lastType = tempType;
   } else {
      allOK = 0;
   } // if/else
   return allOK;
} // PartTypes::AddType()

// Displays the available types and my extended MBR codes for same....
// Note: This function assumes an 80-column display. On wider displays,
// it stops at under 80 columns; on narrower displays, lines will wrap
// in an ugly way.
void PartTypes::ShowTypes(void) {
   int colCount = 1; // column count
   size_t i;
   AType* thisType = allTypes;

   cout.unsetf(ios::uppercase);
   while (thisType != NULL) {
      if (thisType->display == 1) { // show it
         cout.fill('0');
         cout.width(4);
         cout << hex << thisType->MBRType << " ";
         cout << thisType->name.substr(0, 20);
         for (i = 0; i < (20 - (thisType->name.substr(0, 20).length())); i++) cout << " ";
         if ((colCount % 3) == 0)
            cout << "\n";
         else
            cout << "  ";
         colCount++;
      } // if
      thisType = thisType->next;
   } // while
   cout.fill(' ');
   cout << "\n" << dec;
} // PartTypes::ShowTypes()

// Returns 1 if code is a valid extended MBR code, 0 if it's not
int PartTypes::Valid(uint16_t code) {
   AType* thisType = allTypes;
   int found = 0;

   while ((thisType != NULL) && (!found)) {
      if (thisType->MBRType == code) {
         found = 1;
      } // if
      thisType = thisType->next;
   } // while
   return found;
} // PartTypes::Valid()

// Convert a GUID code to a name.
string PartTypes::GUIDToName(struct GUIDData typeCode) {
   AType* theItem = allTypes;
   int found = 0;
   string typeName;

   while ((theItem != NULL) && (!found)) {
      if ((theItem->GUIDType.data1 == typeCode.data1) &&
          (theItem->GUIDType.data2 == typeCode.data2)) { // found it!
         typeName = theItem->name;
	 found = 1;
      } else {
         theItem = theItem->next;
      } // if/else
   } // while
   if (!found) {
      typeName = "Unknown";
   } // if (!found)
   return typeName;
} // PartTypes::GUIDToName()

// This function takes a variant of the MBR partition type code and
// converts it to a GUID type code
struct GUIDData PartTypes::IDToGUID(uint16_t ID) {
   AType* theItem = allTypes;
   int found = 0;
   struct GUIDData theGUID;

   // Start by assigning a default GUID for the return value. Done
   // "raw" to avoid the necessity for a recursive call and the
   // remote possibility of an infinite recursive loop that this
   // approach would present....
   theGUID.data1 = UINT64_C(0x4433B9E5EBD0A0A2);
   theGUID.data2 = UINT64_C(0xC79926B7B668C087);

   // Now search the type list for a match to the ID....
   while ((theItem != NULL) && (!found)) {
      if (theItem->MBRType == ID)  { // found it!
         theGUID = theItem->GUIDType;
	 found = 1;
      } else {
         theItem = theItem->next;
      } // if/else
   } // while
   if (!found) {
      cout.setf(ios::uppercase);
      cout.fill('0');
      cout << "Exact type match not found for type code ";
      cout.width(4);
      cout << hex << ID << "; assigning type code for\n'Linux/Windows data'\n" << dec;
      cout.fill(' ');
   } // if (!found)
   return theGUID;
} // PartTypes::IDToGUID()

// Convert a GUID to a 16-bit variant of the MBR ID number.
// Note that this function ignores entries for which the display variable
// is set to 0. This enables control of which values get returned when
// there are multiple possibilities, but opens the algorithm up to the
// potential for problems should the data in the list be bad.
uint16_t PartTypes::GUIDToID(struct GUIDData typeCode) {
   AType* theItem = allTypes;
   int found = 0;
   uint16_t theID = 0xFFFF;

   while ((theItem != NULL) && (!found)) {
      if ((theItem->GUIDType.data1 == typeCode.data1) &&
          (theItem->GUIDType.data2 == typeCode.data2) &&
          (theItem->display == 1)) { // found it!
         theID = theItem->MBRType;
	 found = 1;
      } else {
         theItem = theItem->next;
      } // if/else
   } // while
   if (!found) {
      theID = 0xFFFF;
   } // if (!found)
   return theID;
} // PartTypes::GUIDToID()
