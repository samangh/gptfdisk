/*
    Copyright (C) 2011  <Roderick W. Smith>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*/

/* This class implements an interactive curses-based interface atop the
   GPTData class */

#include <string>
#include "gptcurses.h"

using namespace std;

#define MAX_OPTIONS 50

int main(int argc, char *argv[]) {
   string device = "";

   if (!SizesOK())
      exit(1);

   switch (argc) {
      case 1:
         cout << "Type device filename, or press <Enter> to exit: ";
         device = ReadString();
         if (device.length() == 0)
            exit(0);
         break;
      case 2: // basic usage
         device = argv[1];
         break;
      default:
         cerr << "Usage: " << argv[0] << " device_file\n";
         exit(1);
         break;
   } // switch

   GPTDataCurses theGPT;

   if (theGPT.LoadPartitions(device)) {
      if (theGPT.GetState() != use_gpt) {
         Report("Warning! Non-GPT or damaged disk detected! This program will attempt to\n"
                "convert to GPT form or repair damage to GPT data structures, but may not\n"
                "succeed. Use gdisk or another disk repair tool if you have a damaged GPT\n"
                "disk.");
      } // if
      theGPT.MainMenu();
   } else {
      Report("Could not load partitions from '" + device + "'! Aborting!");
   } // if/else
} // main
