//
// C++ Interface: GUIDData
//
// Description: GUIDData class header
// Implements the GUIDData data structure and support methods
//
//
// Author: Rod Smith <rodsmith@rodsbooks.com>, (C) 2010
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef __GUIDDATA_CLASS
#define __GUIDDATA_CLASS

#include <stdint.h>
#include <string>

// Have to play games with uuid_t since it's defined in incompatible ways
// for Unix (libuuid) vs. Windows
#ifdef _WIN32
typedef unsigned char my_uuid_t[16];
#else
#include <uuid/uuid.h>
typedef uuid_t my_uuid_t;
#endif

using namespace std;

// Note: This class's data size is critical. If data elements must be added,
// it will be necessary to modify various GPT classes to compensate.
class GUIDData {
   protected:
      my_uuid_t uuidData;
      string DeleteSpaces(string s);
   public:
      GUIDData(void);
      GUIDData(const GUIDData & orig);
      GUIDData(const char * orig);
      ~GUIDData(void);

      // Data assignment operators....
      GUIDData & operator=(const GUIDData & orig);
      GUIDData & operator=(const string & orig);
      GUIDData & operator=(const char * orig);
      GUIDData & GetGUIDFromUser(void);
      void Zero(void);
      void Randomize(void);

      // Data tests....
      int operator==(const GUIDData & orig);
      int operator!=(const GUIDData & orig);

      // Data retrieval....
      string AsString(void);
}; // class GUIDData

#endif
