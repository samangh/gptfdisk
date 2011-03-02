/*
    gptpartnotes.h -- Class that takes notes on GPT partitions for purpose of MBR conversion
    Copyright (C) 2010-2011 Roderick W. Smith

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

#ifndef __GPTPARTNOTES_H
#define __GPTPARTNOTES_H

#include "gpt.h"
#include "gptpart.h"
#include "partnotes.h"

using namespace std;

class GptPartNotes : public PartNotes {
   protected:
      GPTPart *gptParts;
   public:
      GptPartNotes();
      GptPartNotes(GPTPart *parts, GPTData *gpt, int num, int blockSize);
//      ~GptPartNotes();

      // Add partition notes (little or no error checking)
      int PassPartitions(GPTPart *parts, GPTData *gpt, int num, int blockSize);

      // Retrieve data or metadata
      int IsSorted(void);

      // Interact with users, possibly changing data with error handling
      void ShowSummary(void);
      int ChangeType(int partNum, int newType);
};

#endif // __GPTPARTNOTES_H
