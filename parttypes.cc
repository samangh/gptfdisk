// parttypes.cc
// Class to manage partition type codes -- a slight variant on MBR type
// codes, GUID type codes, and associated names.

/* This program is copyright (c) 2009-2011 by Roderick W. Smith. It is distributed
  under the terms of the GNU GPL version 2, as detailed in the COPYING file. */

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include "parttypes.h"

using namespace std;

int PartType::numInstances = 0;
AType* PartType::allTypes = NULL;
AType* PartType::lastType = NULL;

// Constructor. Its main task is to initialize the data list, but only
// if this is the first instance, since it's a static linked list.
// Partition type codes are MBR type codes multiplied by 0x0100, with
// additional related codes taking on following numbers. For instance,
// the FreeBSD disklabel code in MBR is 0xa5; here, it's 0xa500, with
// additional FreeBSD codes being 0xa501, 0xa502, and so on. This gives
// related codes similar numbers and (given appropriate entry positions
// in the linked list) keeps them together in the listings generated
// by typing "L" at the main gdisk menu.
PartType::PartType(void) : GUIDData() {
   numInstances++;
   if (numInstances == 1) {
      AddAllTypes();
   } // if
} // default constructor

PartType::PartType(const PartType & orig) : GUIDData(orig) {
   numInstances++;
   if (numInstances == 1) { // should never happen; just being paranoid
      AddAllTypes();
   } // if
} // PartType copy constructor

PartType::PartType(const GUIDData & orig) : GUIDData(orig) {
   numInstances++;
   if (numInstances == 1) {
      AddAllTypes();
   } // if
} // PartType copy constructor

PartType::~PartType(void) {
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

// Add all partition type codes to the internal linked-list structure.
// Used by constructors.
// See http://www.win.tue.nl/~aeb/partitions/partition_types-1.html
// for a list of MBR partition type codes.
void PartType::AddAllTypes(void) {
   // Start with the "unused entry," which should normally appear only
   // on empty partition table entries....
   AddType(0x0000, "00000000-0000-0000-0000-000000000000", "Unused entry", 0);

   // DOS/Windows partition types, which confusingly Linux also uses in GPT
   AddType(0x0100, "EBD0A0A2-B9E5-4433-87C0-68B6B72699C7", "Microsoft basic data", 0); // FAT-12
   AddType(0x0400, "EBD0A0A2-B9E5-4433-87C0-68B6B72699C7", "Microsoft basic data", 0); // FAT-16 < 32M
   AddType(0x0600, "EBD0A0A2-B9E5-4433-87C0-68B6B72699C7", "Microsoft basic data", 0); // FAT-16
   AddType(0x0700, "EBD0A0A2-B9E5-4433-87C0-68B6B72699C7", "Microsoft basic data", 1); // NTFS (or HPFS)
   AddType(0x0b00, "EBD0A0A2-B9E5-4433-87C0-68B6B72699C7", "Microsoft basic data", 0); // FAT-32
   AddType(0x0c00, "EBD0A0A2-B9E5-4433-87C0-68B6B72699C7", "Microsoft basic data", 0); // FAT-32 LBA
   AddType(0x0c01, "E3C9E316-0B5C-4DB8-817D-F92DF00215AE", "Microsoft reserved");
   AddType(0x0e00, "EBD0A0A2-B9E5-4433-87C0-68B6B72699C7", "Microsoft basic data", 0); // FAT-16 LBA
   AddType(0x1100, "EBD0A0A2-B9E5-4433-87C0-68B6B72699C7", "Microsoft basic data", 0); // Hidden FAT-12
   AddType(0x1400, "EBD0A0A2-B9E5-4433-87C0-68B6B72699C7", "Microsoft basic data", 0); // Hidden FAT-16 < 32M
   AddType(0x1600, "EBD0A0A2-B9E5-4433-87C0-68B6B72699C7", "Microsoft basic data", 0); // Hidden FAT-16
   AddType(0x1700, "EBD0A0A2-B9E5-4433-87C0-68B6B72699C7", "Microsoft basic data", 0); // Hidden NTFS (or HPFS)
   AddType(0x1b00, "EBD0A0A2-B9E5-4433-87C0-68B6B72699C7", "Microsoft basic data", 0); // Hidden FAT-32
   AddType(0x1c00, "EBD0A0A2-B9E5-4433-87C0-68B6B72699C7", "Microsoft basic data", 0); // Hidden FAT-32 LBA
   AddType(0x1e00, "EBD0A0A2-B9E5-4433-87C0-68B6B72699C7", "Microsoft basic data", 0); // Hidden FAT-16 LBA
   AddType(0x2700, "DE94BBA4-06D1-4D40-A16A-BFD50179D6AC", "Windows RE");
   AddType(0x4200, "AF9B60A0-1431-4F62-BC68-3311714A69AD", "Windows LDM data"); // Logical disk manager
   AddType(0x4201, "5808C8AA-7E8F-42E0-85D2-E1E90434CFB3", "Windows LDM metadata"); // Logical disk manager

   // An oddball IBM filesystem....
   AddType(0x7501, "37AFFC90-EF7D-4E96-91C3-2D7AE055B174", "IBM GPFS"); // General Parallel File System (GPFS)

   // ChromeOS-specific partition types...
   // Values taken from vboot_reference/firmware/lib/cgptlib/include/gpt.h in
   // ChromeOS source code, retrieved 12/23/2010. They're also at
   // http://www.chromium.org/chromium-os/chromiumos-design-docs/disk-format.
   // These have no MBR equivalents, AFAIK, so I'm using 0x7Fxx values, since they're close
   // to the Linux values.
   AddType(0x7f00, "FE3A2A5D-4F32-41A7-B725-ACCC3285A309", "ChromeOS kernel");
   AddType(0x7f01, "3CB8E202-3B7E-47DD-8A3C-7FF2A13CFCEC", "ChromeOS root");
   AddType(0x7f02, "2E0A753D-9E48-43B0-8337-B15192CB1B5E", "ChromeOS reserved");

   // Linux-specific partition types....
   AddType(0x8200, "0657FD6D-A4AB-43C4-84E5-0933C84B4F4F", "Linux swap"); // Linux swap (or Solaris)
   AddType(0x8300, "0FC63DAF-8483-4772-8E79-3D69D8477DE4", "Linux filesystem"); // Linux native
   AddType(0x8301, "8DA63339-0007-60C0-C436-083AC8230908", "Linux reserved");
   AddType(0x8e00, "E6D6D379-F507-44C2-A23C-238F2A3DF928", "Linux LVM");

   // FreeBSD partition types....
   // Note: Rather than extract FreeBSD disklabel data, convert FreeBSD
   // partitions in-place, and let FreeBSD sort out the details....
   AddType(0xa500, "516E7CB4-6ECF-11D6-8FF8-00022D09712B", "FreeBSD disklabel");
   AddType(0xa501, "83BD6B9D-7F41-11DC-BE0B-001560B84F0F", "FreeBSD boot");
   AddType(0xa502, "516E7CB5-6ECF-11D6-8FF8-00022D09712B", "FreeBSD swap");
   AddType(0xa503, "516E7CB6-6ECF-11D6-8FF8-00022D09712B", "FreeBSD UFS");
   AddType(0xa504, "516E7CBA-6ECF-11D6-8FF8-00022D09712B", "FreeBSD ZFS");
   AddType(0xa505, "516E7CB8-6ECF-11D6-8FF8-00022D09712B", "FreeBSD Vinum/RAID");

   // Midnight BSD partition types....
   AddType(0xa580, "85D5E45A-237C-11E1-B4B3-E89A8F7FC3A7", "Midnight BSD data");
   AddType(0xa581, "85D5E45E-237C-11E1-B4B3-E89A8F7FC3A7", "Midnight BSD boot");
   AddType(0xa582, "85D5E45B-237C-11E1-B4B3-E89A8F7FC3A7", "Midnight BSD swap");
   AddType(0xa583, "0394Ef8B-237E-11E1-B4B3-E89A8F7FC3A7", "Midnight BSD UFS");
   AddType(0xa584, "85D5E45D-237C-11E1-B4B3-E89A8F7FC3A7", "Midnight BSD ZFS");
   AddType(0xa585, "85D5E45C-237C-11E1-B4B3-E89A8F7FC3A7", "Midnight BSD Vinum");

   // A MacOS partition type, separated from others by NetBSD partition types...
   AddType(0xa800, "55465300-0000-11AA-AA11-00306543ECAC", "Apple UFS"); // Mac OS X

   // NetBSD partition types. Note that the main entry sets it up as a
   // FreeBSD disklabel. I'm not 100% certain this is the correct behavior.
   AddType(0xa900, "516E7CB4-6ECF-11D6-8FF8-00022D09712B", "FreeBSD disklabel", 0); // NetBSD disklabel
   AddType(0xa901, "49F48D32-B10E-11DC-B99B-0019D1879648", "NetBSD swap");
   AddType(0xa902, "49F48D5A-B10E-11DC-B99B-0019D1879648", "NetBSD FFS");
   AddType(0xa903, "49F48D82-B10E-11DC-B99B-0019D1879648", "NetBSD LFS");
   AddType(0xa904, "2DB519C4-B10F-11DC-B99B-0019D1879648", "NetBSD concatenated");
   AddType(0xa905, "2DB519EC-B10F-11DC-B99B-0019D1879648", "NetBSD encrypted");
   AddType(0xa906, "49F48DAA-B10E-11DC-B99B-0019D1879648", "NetBSD RAID");

   // Mac OS partition types (See also 0xa800, above)....
   AddType(0xab00, "426F6F74-0000-11AA-AA11-00306543ECAC", "Apple boot");
   AddType(0xaf00, "48465300-0000-11AA-AA11-00306543ECAC", "Apple HFS/HFS+");
   AddType(0xaf01, "52414944-0000-11AA-AA11-00306543ECAC", "Apple RAID");
   AddType(0xaf02, "52414944-5F4F-11AA-AA11-00306543ECAC", "Apple RAID offline");
   AddType(0xaf03, "4C616265-6C00-11AA-AA11-00306543ECAC", "Apple label");
   AddType(0xaf04, "5265636F-7665-11AA-AA11-00306543ECAC", "AppleTV recovery");
   AddType(0xaf05, "53746F72-6167-11AA-AA11-00306543ECAC", "Apple Core Storage");

   // Solaris partition types (one of which is shared with MacOS)
   AddType(0xbe00, "6A82CB45-1DD2-11B2-99A6-080020736631", "Solaris boot");
   AddType(0xbf00, "6A85CF4D-1DD2-11B2-99A6-080020736631", "Solaris root");
   AddType(0xbf01, "6A898CC3-1DD2-11B2-99A6-080020736631", "Solaris /usr & Mac ZFS"); // Solaris/MacOS
   AddType(0xbf02, "6A87C46F-1DD2-11B2-99A6-080020736631", "Solaris swap");
   AddType(0xbf03, "6A8B642B-1DD2-11B2-99A6-080020736631", "Solaris backup");
   AddType(0xbf04, "6A8EF2E9-1DD2-11B2-99A6-080020736631", "Solaris /var");
   AddType(0xbf05, "6A90BA39-1DD2-11B2-99A6-080020736631", "Solaris /home");
   AddType(0xbf06, "6A9283A5-1DD2-11B2-99A6-080020736631", "Solaris alternate sector");
   AddType(0xbf07, "6A945A3B-1DD2-11B2-99A6-080020736631", "Solaris Reserved 1");
   AddType(0xbf08, "6A9630D1-1DD2-11B2-99A6-080020736631", "Solaris Reserved 2");
   AddType(0xbf09, "6A980767-1DD2-11B2-99A6-080020736631", "Solaris Reserved 3");
   AddType(0xbf0a, "6A96237F-1DD2-11B2-99A6-080020736631", "Solaris Reserved 4");
   AddType(0xbf0b, "6A8D2AC7-1DD2-11B2-99A6-080020736631", "Solaris Reserved 5");

   // I can find no MBR equivalents for these, but they're on the
   // Wikipedia page for GPT, so here we go....
   AddType(0xc001, "75894C1E-3AEB-11D3-B7C1-7B03A0000000", "HP-UX data");
   AddType(0xc002, "E2A1E728-32E3-11D6-A682-7B03A0000000", "HP-UX service");

   // EFI system and related partitions
   AddType(0xef00, "C12A7328-F81F-11D2-BA4B-00A0C93EC93B", "EFI System"); // Parted identifies these as having the "boot flag" set
   AddType(0xef01, "024DEE41-33E7-11D3-9D69-0008C781F39F", "MBR partition scheme"); // Used to nest MBR in GPT
   AddType(0xef02, "21686148-6449-6E6F-744E-656564454649", "BIOS boot partition"); // Boot loader

   // A straggler Linux partition type....
   AddType(0xfd00, "A19D880F-05FC-4D3B-A006-743F0F84911E", "Linux RAID");

   // Note: DO NOT use the 0xffff code; that's reserved to indicate an
   // unknown GUID type code.
} // PartType::AddAllTypes()

// Add a single type to the linked list of types. Returns 1 if operation
// succeeds, 0 otherwise.
int PartType::AddType(uint16_t mbrType, const char * guidData, const char * name,
                      int toDisplay) {
   AType* tempType;
   int allOK = 1;

   tempType = new AType;
   if (tempType != NULL) {
      tempType->MBRType = mbrType;
      tempType->GUIDType = guidData;
      tempType->name = name;
      tempType->display = toDisplay;
      tempType->next = NULL;
      if (allTypes == NULL) { // first entry
         allTypes = tempType;
      } else {
         lastType->next = tempType;
      } // if/else
      lastType = tempType;
   } else {
      cerr << "Unable to allocate memory in PartType::AddType()! Partition type list will\n";
      cerr << "be incomplete!\n";
      allOK = 0;
   } // if/else
   return allOK;
} // GUID::AddType(const char* variant)

// Assignment operator by string. If the original string is short,
// interpret it as a gdisk hex code; if it's longer, interpret it as
// a direct entry of a GUID value. If a short string isn't a hex
// number, do nothing.
PartType & PartType::operator=(const string & orig) {
   uint32_t hexCode;

   if (orig.length() < 32) {
      if (IsHex(orig)) {
         sscanf(orig.c_str(), "%x", &hexCode);
         *this = hexCode;
      }
   } else {
      GUIDData::operator=(orig);
   } // if/else hexCode or GUID
   return *this;
} // PartType::operator=(const char * orig)

// Assignment from C-style string; rely on C++ casting....
PartType & PartType::operator=(const char * orig) {
   return operator=((string) orig);
} // PartType::operator=(const char * orig)

// Assign a GUID based on my custom 2-byte (16-bit) MBR hex ID variant
PartType & PartType::operator=(uint16_t ID) {
   AType* theItem = allTypes;
   int found = 0;

   // Now search the type list for a match to the ID....
   while ((theItem != NULL) && (!found)) {
      if (theItem->MBRType == ID)  {
         GUIDData::operator=(theItem->GUIDType);
         found = 1;
      } else {
         theItem = theItem->next;
      } // if/else
   } // while
   if (!found) {
      // Assign a default value....
      operator=(DEFAULT_TYPE);
      cout.setf(ios::uppercase);
      cout.fill('0');
      cout << "Exact type match not found for type code ";
      cout.width(4);
      cout << hex << ID << "; assigning type code for\n'" << TypeName() << "'\n" << dec;
      cout.fill(' ');
   } // if (!found)
   return *this;
} // PartType::operator=(uint16_t ID)

// Return the English description of the partition type (e.g., "Linux filesystem")
string PartType::TypeName(void) const {
   AType* theItem = allTypes;
   int found = 0;
   string typeName;

   while ((theItem != NULL) && (!found)) {
      if (theItem->GUIDType == *this) { // found it!
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
} // PartType::TypeName()

// Return the Unicode description of the partition type (e.g., "Linux filesystem")
UnicodeString PartType::UTypeName(void) const {
   AType* theItem = allTypes;
   int found = 0;
   UnicodeString typeName;

   while ((theItem != NULL) && (!found)) {
      if (theItem->GUIDType == *this) { // found it!
         typeName = theItem->name.c_str();
         found = 1;
      } else {
         theItem = theItem->next;
      } // if/else
   } // while
   if (!found) {
      typeName = "Unknown";
   } // if (!found)
   return typeName;
} // PartType::TypeName()

// Return the custom GPT fdisk 2-byte (16-bit) hex code for this GUID partition type
// Note that this function ignores entries for which the display variable
// is set to 0. This enables control of which values get returned when
// there are multiple possibilities, but opens the algorithm up to the
// potential for problems should the data in the list be bad.
uint16_t PartType::GetHexType() const {
   AType* theItem = allTypes;
   int found = 0;
   uint16_t theID = 0xFFFF;

   while ((theItem != NULL) && (!found)) {
      if ((theItem->GUIDType == *this) && (theItem->display == 1)) { // found it!
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
} // PartType::GetHex()

// Displays the available types and my extended MBR codes for same....
// Note: This function assumes an 80-column display. On wider displays,
// it stops at under 80 columns; on narrower displays, lines will wrap
// in an ugly way.
void PartType::ShowAllTypes(void) const {
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
         for (i = 0; i < (20 - (thisType->name.substr(0, 20).length())); i++)
            cout << " ";
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
} // PartType::ShowTypes()

// Returns 1 if code is a valid extended MBR code, 0 if it's not
int PartType::Valid(uint16_t code) const {
   AType* thisType = allTypes;
   int found = 0;

   while ((thisType != NULL) && (!found)) {
      if (thisType->MBRType == code) {
         found = 1;
      } // if
      thisType = thisType->next;
   } // while
   return found;
} // PartType::Valid()
