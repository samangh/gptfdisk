// sgdisk.cc
// Command-line-based version of gdisk. This program is named after sfdisk,
// and it can serve a similar role (easily scripted, etc.), but it's used
// strictly via command-line arguments, and it doesn't bear much resemblance
// to sfdisk in actual use.
//
// by Rod Smith, project began February 2009; sgdisk begun January 2010.

/* This program is copyright (c) 2009-2011 by Roderick W. Smith. It is distributed
  under the terms of the GNU GPL version 2, as detailed in the COPYING file. */

#include "gptcl.h"

using namespace std;

#define MAX_OPTIONS 50

int main(int argc, char *argv[]) {
   GPTDataCL theGPT;
   return theGPT.DoOptions(argc, argv);
} // main
