/* This program is copyright (c) 2009 by Roderick W. Smith. It is distributed
  under the terms of the GNU GPL version 2, as detailed in the COPYING file. */

#include <stdint.h>
#include <stdlib.h>
#include <string>
#include "support.h"
#include "guid.h"

#ifndef __PARTITION_TYPES
#define __PARTITION_TYPES

using namespace std;

// A partition type
struct AType {
   // I'm using a custom 16-bit extension of the original MBR 8-bit
   // type codes, so as to permit disambiguation and use of new
   // codes required by GPT
   uint16_t MBRType;
   GUIDData GUIDType;
   string name;
   int display; // 1 to show to users as available type, 0 not to
   AType* next;
}; // struct AType

class PartType : public GUIDData {
protected:
   static int numInstances;
   static AType* allTypes; // Linked list holding all the data
   static AType* lastType; // Pointer to last entry in the list
   void AddAllTypes(void);
public:
   PartType(void);
   PartType(const PartType & orig);
   PartType(const GUIDData & orig);
   ~PartType(void);

   // Set up type information
   int AddType(uint16_t mbrType, const char * guidData, const char * name, int toDisplay = 1);

   // Assignment operators based on base class....
   GUIDData & operator=(const GUIDData & orig) {return GUIDData::operator=(orig);}
   GUIDData & operator=(const string & orig) {return GUIDData::operator=(orig);}
   GUIDData & operator=(const char * orig) {return GUIDData::operator=(orig);}

   // New data assignment
   PartType & operator=(uint16_t ID); // Use MBR type code times 0x0100 to assign GUID

   // Retrieve transformed GUID data based on type code matches
   string TypeName(void);
   uint16_t GetHexType();

   // Information relating to all type data
   void ShowAllTypes(void);
   int Valid(uint16_t);
};

#endif
