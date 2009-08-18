// attributes.cc
// Class to manage partition attribute codes. These are binary bit fields,
// of which only three are currently (2/2009) documented on Wikipedia.

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include <string.h>
#include <stdint.h>
#include <stdio.h>
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
      strcpy(atNames[i], temp);
   } // for

   // Now reset those names that are defined....
   strcpy(atNames[0], "system partition");
   strcpy(atNames[60], "read-only");
   strcpy(atNames[62], "hidden");
   strcpy(atNames[63], "do not automount");
} // Attributes constructor

// Destructor.
Attributes::~Attributes(void) {
} // Attributes destructor

// Display current attributes to user
void Attributes::DisplayAttributes(void) {
   int i;

   printf("Attribute value is %llX. Set fields are:\n",
          (unsigned long long) attributes);
   for (i = 0; i < NUM_ATR; i++) {
      if (((attributes >> i) % 2) == 1) { // bit is set
/*         if (strncmp("Undefined", atNames[i], 9) != 0)
            printf("%s\n", atNames[i]); */
         if (strncmp("Undefined", atNames[NUM_ATR - i - 1], 9) != 0)
            printf("%s\n", atNames[NUM_ATR - i - 1]);
      } // if
   } // for
} // Attributes::DisplayAttributes()

// Prompt user for attribute changes
void Attributes::ChangeAttributes(void) {
   int response, i;
   uint64_t bitValue;

   printf("Known attributes are:\n");
   for (i = 0; i < NUM_ATR; i++) {
      if (strncmp("Undefined", atNames[i], 9) != 0)
         printf("%d - %s\n", i, atNames[i]);
   } // for

   do {
      response = GetNumber(0, 64, -1, "Toggle which attribute field (0-63, 64 to exit): ");
      if (response != 64) {
         bitValue = PowerOf2(NUM_ATR - response - 1); // Find the integer value of the bit
//         bitValue = PowerOf2(response); // Find the integer value of the bit
         if ((bitValue & attributes) == bitValue) { // bit is set
            attributes -= bitValue; // so unset it
	    printf("Have disabled the '%s' attribute.\n", atNames[response]);
         } else { // bit is not set
            attributes += bitValue; // so set it
	    printf("Have enabled the '%s' attribute.\n", atNames[response]);
         } // if/else
      } // if
   } while (response != 64);
} // Attributes::ChangeAttributes()

