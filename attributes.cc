// attributes.cc
// Class to manage partition attribute codes. These are binary bit fields,
// of which only three are currently (2/2009) documented on Wikipedia.

/* This program is copyright (c) 2009 by Roderick W. Smith. It is distributed
  under the terms of the GNU GPL version 2, as detailed in the COPYING file. */

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include <sstream>

#include "attributes.h"
#include "support.h"

using namespace std;

string Attributes::atNames[NUM_ATR];
Attributes::staticInit Attributes::staticInitializer;

Attributes::staticInit::staticInit (void) {
   ostringstream temp;
 
   // Most bits are undefined, so start by giving them an
   // appropriate name
   for (int i = 0; i < NUM_ATR; i++) {
      temp.str("");
      temp << "Undefined bit #" << i;
      Attributes::atNames[i] = temp.str();
   } // for

   // Now reset those names that are defined....
   Attributes::atNames[0] = "system partition"; // required for computer to operate
   Attributes::atNames[1] = "hide from EFI";
   Attributes::atNames[2] = "legacy BIOS bootable";
   Attributes::atNames[60] = "read-only";
   Attributes::atNames[62] = "hidden";
   Attributes::atNames[63] = "do not automount";
}  // Attributes::staticInit::staticInit

// Destructor.
Attributes::~Attributes(void) {
} // Attributes destructor

// Display current attributes to user
void Attributes::DisplayAttributes(void) {
   uint32_t i;
   int numSet = 0;

   cout << "Attribute value is ";
   cout.setf(ios::uppercase);
   cout.fill('0');
   cout.width(16);
   cout << hex << attributes << dec << ". Set fields are:\n";
   for (i = 0; i < NUM_ATR; i++) {
      if ((UINT64_C(1) << i) & attributes) {
         cout << i << " (" << Attributes::GetAttributeName(i) << ")" << "\n";
         numSet++;
      } // if
   } // for
   cout.fill(' ');
   if (numSet == 0)
      cout << "  No fields set\n";
   cout << "\n";
} // Attributes::DisplayAttributes()

// Prompt user for attribute changes
void Attributes::ChangeAttributes(void) {
   int response;
   uint64_t bitValue;

   cout << "Known attributes are:\n";
   ListAttributes();
   cout << "\n";

   do {
      DisplayAttributes();
      response = GetNumber(0, NUM_ATR, -1, "Toggle which attribute field (0-63, 64 to exit): ");
      if (response != 64) {
         bitValue = UINT64_C(1) << response; // Find the integer value of the bit
         if (bitValue & attributes) { // bit is set
            attributes &= ~bitValue; // so unset it
	         cout << "Have disabled the '" << atNames[response] << "' attribute.\n";
         } else { // bit is not set
            attributes |= bitValue; // so set it
            cout << "Have enabled the '" << atNames[response] << "' attribute.\n";
         } // if/else
      } // if
   } while (response != 64);
} // Attributes::ChangeAttributes()

// Display all defined attributes on the screen (omits undefined bits).
void Attributes::ListAttributes(void) {
   uint32_t bitNum;
   string tempAttr;

   for (bitNum = 0; bitNum < NUM_ATR; bitNum++) {
      tempAttr = GetAttributeName(bitNum);
      if (tempAttr.substr(0, 15) != "Undefined bit #" )
         cout << bitNum << ": " << Attributes::GetAttributeName(bitNum) << "\n";
   } // for
} // Attributes::ListAttributes

void Attributes::ShowAttributes(const uint32_t partNum) {
   uint32_t bitNum;
   bool bitset;

   for (bitNum = 0; bitNum < 64; bitNum++) {
      bitset = (UINT64_C(1) << bitNum) & attributes;
      if (bitset) {
         cout << partNum+1 << ":" << bitNum << ":" << bitset
              << " (" << Attributes::GetAttributeName(bitNum) << ")" << endl;
      } // if
   } // for
} // Attributes::ShowAttributes

// multifaceted attributes access
// returns true upon success, false upon failure
bool Attributes::OperateOnAttributes(const uint32_t partNum, const string& attributeOperator, const string& attributeBits) {

   // attribute access opcode
   typedef enum {
      ao_or, ao_nand, ao_xor, ao_assignall,  // operate on all attributes (bitmask)
      ao_unknown, // must be after bitmask operators and before bitnum operators
      ao_set, ao_clear, ao_toggle, ao_get    // operate on a single attribute (bitnum)
   } attribute_opcode_t; // typedef enum

   // translate attribute operator into an attribute opcode
   attribute_opcode_t attributeOpcode = ao_unknown; { // opcode is not known yet
      if      (attributeOperator == "or")      attributeOpcode = ao_or;
      else if (attributeOperator == "nand")    attributeOpcode = ao_nand;
      else if (attributeOperator == "xor")     attributeOpcode = ao_xor;
      else if (attributeOperator == "=")       attributeOpcode = ao_assignall;
      else if (attributeOperator == "set")     attributeOpcode = ao_set;
      else if (attributeOperator == "clear")   attributeOpcode = ao_clear;
      else if (attributeOperator == "toggle")  attributeOpcode = ao_toggle;
      else if (attributeOperator == "get")     attributeOpcode = ao_get;
      else {
         cerr << "Unknown attributes operator: " << attributeOperator << endl;
         return false;
      } // else
   } // attributeOpcode

   // get bit mask if operating on entire attribute set
   uint64_t attributeBitMask; { if (attributeOpcode < ao_unknown) {
      if (1 != sscanf (attributeBits.c_str(), "%qx", (long long unsigned int*) &attributeBitMask)) {
         cerr << "Could not convert hex attribute mask" << endl;
         return false;
      } // if
   }} // attributeBitMask, if

   // get bit number and calculate bit mask if operating on a single attribute
   int bitNum; { if (attributeOpcode > ao_unknown) {
      if (1 != sscanf (attributeBits.c_str(), "%d", &bitNum)) {
         cerr << "Could not convert bit number" << endl;
         return false;
      } // if
      const uint64_t one = 1;
      attributeBitMask = one << bitNum;
   }} // bitNum, if

   switch (attributeOpcode) {
      // assign all attributes at once
      case ao_assignall:  attributes = attributeBitMask;    break;

      // set individual attribute(s)
      case ao_set:
      case ao_or:         attributes |= attributeBitMask;   break;

      // clear individual attribute(s)
      case ao_clear:
      case ao_nand:       attributes &= ~attributeBitMask;  break;

      // toggle individual attribute(s)
      case ao_toggle:
      case ao_xor:        attributes ^= attributeBitMask;   break;

      // display a single attribute
      case ao_get: {
         cout << partNum+1 << ":" << bitNum << ":" <<
              bool (attributeBitMask & attributes) << endl;
         break;
      } // case ao_get

      default: break; // will never get here
   } // switch

   return true;
} // Attributes::OperateOnAttributes()
