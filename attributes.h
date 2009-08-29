/* This program is copyright (c) 2009 by Roderick W. Smith. It is distributed
  under the terms of the GNU GPL version 2, as detailed in the COPYING file. */

#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string>
#include "support.h"

#ifndef __GPT_ATTRIBUTES
#define __GPT_ATTRIBUTES

#define NUM_ATR 64 /* # of attributes -- 64, since it's a 64-bit field */
#define ATR_NAME_SIZE 25 /* maximum size of attribute names */

using namespace std;

class Attributes {
protected:
   uint64_t attributes;
   char atNames[NUM_ATR][ATR_NAME_SIZE];
public:
   Attributes(void);
   ~Attributes(void);
   void SetAttributes(uint64_t a) {attributes = a;}
   uint64_t GetAttributes(void) {return attributes;}
   void DisplayAttributes(void);
   void ChangeAttributes(void);
}; // class Attributes

#endif
