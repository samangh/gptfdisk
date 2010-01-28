/* This program is copyright (c) 2009 by Roderick W. Smith. It is distributed
  under the terms of the GNU GPL version 2, as detailed in the COPYING file. */

#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string>
#include "support.h"

#ifndef __PARTITION_TYPES
#define __PARTITION_TYPES

// Set the size of the name string
#define PNAME_SIZE 80

using namespace std;

// A partition type
struct AType {
   // I'm using a custom 16-bit extension of the original MBR 8-bit
   // type codes, so as to permit disambiguation and use of new
   // codes required by GPT
   uint16_t MBRType;
   struct GUIDData GUIDType;
   char name[PNAME_SIZE];
   int display; // 1 to show to users as available type, 0 not to
   AType* next;
}; // struct AType

class PartTypes {
protected:
   static int numInstances;
   static AType* allTypes; // Linked list holding all the data
   static AType* lastType; // Pointer to last entry in the list
public:
   PartTypes(void);
   ~PartTypes(void);
   int AddType(uint16_t mbrType, uint64_t guidData1, uint64_t guidData2,
               const char* name, int toDisplay = 1);
   void ShowTypes(void);
   int Valid(uint16_t);
   string GUIDToName(struct GUIDData typeCode);
   struct GUIDData IDToGUID(uint16_t ID);
   uint16_t GUIDToID(struct GUIDData);
};

#endif
