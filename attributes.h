/* This program is copyright (c) 2009 by Roderick W. Smith. It is distributed
  under the terms of the GNU GPL version 2, as detailed in the COPYING file. */

#include <stdint.h>
#include <string>

#ifndef __GPT_ATTRIBUTES
#define __GPT_ATTRIBUTES

#define NUM_ATR 64 /* # of attributes -- 64, since it's a 64-bit field */
#define ATR_NAME_SIZE 25 /* maximum size of attribute names */

using namespace std;

class Attributes {

private:
   class staticInit {public: staticInit (void);};
   static string atNames[NUM_ATR];
   static Attributes::staticInit staticInitializer;

protected:
   uint64_t attributes;

public:
   Attributes(const uint64_t a = 0) {SetAttributes (a);}
   ~Attributes(void);
   void SetAttributes(const uint64_t a) {attributes = a;}
   uint64_t GetAttributes(void) {return attributes;}
   void DisplayAttributes(void);
   void ChangeAttributes(void);
   void ShowAttributes(const uint32_t partNum);
   bool OperateOnAttributes(const uint32_t partNum, const string& attributeOperator, const string& attributeBits);

   static const string& GetAttributeName(const uint32_t bitNum) {return atNames [bitNum];}
   static void ListAttributes(void);
}; // class Attributes

#endif
