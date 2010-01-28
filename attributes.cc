// attributes.cc
// Class to manage partition attribute codes. These are binary bit fields,
// of which only three are currently (2/2009) documented on Wikipedia.

/* This program is copyright (c) 2009 by Roderick W. Smith. It is distributed
  under the terms of the GNU GPL version 2, as detailed in the COPYING file. */

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include "attributes.h"

using namespace std;

// Constructor. Its main task is to initialize the attribute name
// data.
Attributes::Attributes(void) {
   int i;
   char temp[ATR_NAME_SIZE];

   // Most bits are undefined, so start by giving them an
   // appropriate name
   for (i = 1; i < NUM_ATR; i++) {
      sprintf(temp, "Undefined bit #%d", i);
      atNames[i] = temp;
   } // for

   // Now reset those names that are defined....
   atNames[0] = "system partition";
   atNames[60] = "read-only";
   atNames[62] = "hidden";
   atNames[63] = "do not automount";
} // Attributes constructor

// Destructor.
Attributes::~Attributes(void) {
} // Attributes destructor

// Display current attributes to user
void Attributes::DisplayAttributes(void) {
   int i;

   cout << "Attribute value is ";
   cout.setf(ios::uppercase);
   cout.fill('0');
   cout.width(16);
   cout << hex << attributes << dec << ". Set fields are:\n";
   for (i = 0; i < NUM_ATR; i++) {
      if (((attributes >> i) % 2) == 1) { // bit is set
         if (atNames[NUM_ATR - i - 1].substr(0, 9) != "Undefined")
            cout << atNames[NUM_ATR - i - 1] << "\n";
      } // if
   } // for
   cout.fill(' ');
} // Attributes::DisplayAttributes()

// Prompt user for attribute changes
void Attributes::ChangeAttributes(void) {
   int response, i;
   uint64_t bitValue;

   cout << "Known attributes are:\n";
   for (i = 0; i < NUM_ATR; i++) {
      if (atNames[i].substr(0, 9) != "Undefined")
         cout << i << " - " << atNames[i] << "\n";
   } // for

   do {
      response = GetNumber(0, 64, -1, (string) "Toggle which attribute field (0-63, 64 to exit): ");
      if (response != 64) {
         bitValue = PowerOf2(NUM_ATR - response - 1); // Find the integer value of the bit
         if ((bitValue & attributes) == bitValue) { // bit is set
            attributes -= bitValue; // so unset it
	    cout << "Have disabled the '" << atNames[response] << "' attribute.\n";
         } else { // bit is not set
            attributes += bitValue; // so set it
            cout << "Have enabled the '" << atNames[response] << "' attribute.\n";
         } // if/else
      } // if
   } while (response != 64);
} // Attributes::ChangeAttributes()

