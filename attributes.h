/* This program is copyright (c) 2009-2011 by Roderick W. Smith. It is distributed
  under the terms of the GNU GPL version 2, as detailed in the COPYING file. */

#ifndef __GPT_ATTRIBUTES
#define __GPT_ATTRIBUTES

#include <stdint.h>
#include <string>

#define NUM_ATR 64 /* # of attributes -- 64, since it's a 64-bit field */
#define ATR_NAME_SIZE 25 /* maximum size of attribute names */

class Attributes {
protected:
   static std::string atNames[NUM_ATR];
   static int numAttrs;
   void Setup(void);
   uint64_t attributes;

public:
   Attributes(void);
   Attributes(const uint64_t a);
   ~Attributes(void);
   void operator=(uint64_t a) {attributes = a;}

   uint64_t GetAttributes(void) const {return attributes;}
   void DisplayAttributes(void);
   void ShowAttributes(const uint32_t partNum);

   void ChangeAttributes(void);
   bool OperateOnAttributes(const uint32_t partNum, const std::string& attributeOperator, const std::string& attributeBits);

   static const std::string& GetAttributeName(const uint32_t bitNum) {return atNames [bitNum];}
   static void ListAttributes(void);
}; // class Attributes

std::ostream & operator<<(std::ostream & os, const Attributes & data);

#endif
