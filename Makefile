# Makefile for GPT fdisk

# This program is licensed under the terms of the GNU GPL, version 2,
# or (at your option) any later version.
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

# This is a consolidated Makefile for Linux, FreeBSD, Solaris (untested),
# macOS, and Windows (x86_64 and i686).

# Builds for host OS by default; pass TARGET={target_os} to cross-compile,
# where {target_os} is one of linux, freebsd, solaris, macos, win32, or win64.
# Appropriate cross-compiler support must be installed, of course, and build
# options below may need to be changed.

# DETECTED_OS is used both to set certain options for the build
# environment and to determine the default TARGET if it's not
# otherwise specified.
DETECTED_OS := $(shell uname -s)

ifeq ($(origin TARGET),undefined)
  $(info TARGET is not set; trying to determine target based on host OS....)
  $(info Detected OS is $(DETECTED_OS))
  ifeq ($(DETECTED_OS),Linux)
    # Note: TARGET is set to "linux", but this is never tested, since it's
    # the default.
    TARGET=linux
  else ifeq ($(DETECTED_OS),Darwin)
    TARGET=macos
  else ifeq ($(DETECTED_OS),MINGW64_NT-10.0-19042)
    # Works for my MSYS2 installation, but seems awfully version-specific
    # Also, uname may not exist in some Windows environments.
    TARGET=windows
  else ifeq ($(DETECTED_OS),FreeBSD)
    TARGET=freebsd
  else ifeq ($(DETECTED_OS),SunOS)
    TARGET=solaris
  endif
endif

# A second way to detect Windows....
ifeq ($(origin TARGET),undefined)
  ifeq ($(OS),Windows_NT)
    TARGET=windows
  endif
endif

# For Windows, we need to know the bit depth, too
ifeq ($(TARGET),windows)
  ARCH=$(shell uname -m)
  $(info ARCH is $(ARCH))
  ifeq ($(ARCH),x86_64)
    TARGET=win64
  else ifeq ($(ARCH),i686)
    TARGET=win32
  else
    # In theory, there could be ARM versions, but we aren't set up for them yet;
    # also, default to win64 in case uname doesn't exist on the system
    TARGET=win64
  endif
endif

$(info Build target is $(TARGET))

# Default/Linux settings....
#CXXFLAGS+=-O2 -Wall -D_FILE_OFFSET_BITS=64 -D USE_UTF16
STRIP?=strip
CXXFLAGS+=-O2 -Wall -D_FILE_OFFSET_BITS=64
LDFLAGS+=
LDLIBS+=-luuid #-licuio -licuuc
FATBINFLAGS=
THINBINFLAGS=
SGDISK_LDLIBS=-lpopt
CGDISK_LDLIBS=-lncursesw
LIB_NAMES=crc32 support guid gptpart mbrpart basicmbr mbr gpt bsd parttypes attributes diskio diskio-unix
MBR_LIBS=support diskio diskio-unix basicmbr mbrpart
ALL=gdisk cgdisk sgdisk fixparts
FN_EXTENSION=

# Settings for non-Linux OSes....
ifeq ($(TARGET),win64)
  CXX=x86_64-w64-mingw32-g++
  ifeq ($(DETECTED_OS),Linux)
    STRIP=x86_64-w64-mingw32-strip
  else
    STRIP=strip
  endif
  CXXFLAGS=-O2 -Wall -D_FILE_OFFSET_BITS=64 -static -static-libgcc -static-libstdc++
  #CXXFLAGS=-O2 -Wall -D_FILE_OFFSET_BITS=64 -I /usr/local/include -I/opt/local/include -g
  LDFLAGS+=-static -static-libgcc -static-libstdc++
  LDLIBS+=-lrpcrt4
  SGDISK_LDLIBS=-lpopt -lintl -liconv
  LIB_NAMES=guid gptpart bsd parttypes attributes crc32 mbrpart basicmbr mbr gpt support diskio diskio-windows
  MBR_LIBS=support diskio diskio-windows basicmbr mbrpart
  FN_EXTENSION=64.exe
  ifeq ($(DETECTED_OS),Linux)
    # Omit cgdisk when building under Linux for Windows because it doesn't
    # work on my system
    ALL=gdisk sgdisk fixparts
  endif
else ifeq ($(TARGET),win32)
  CXX=i686-w64-mingw32-g++
  ifeq ($(DETECTED_OS),Linux)
    STRIP=i686-w64-mingw32-strip
  else
    STRIP=strip
  endif
  CXXFLAGS=-O2 -Wall -D_FILE_OFFSET_BITS=64 -static -static-libgcc -static-libstdc++
  #CXXFLAGS=-O2 -Wall -D_FILE_OFFSET_BITS=64 -I /usr/local/include -I/opt/local/include
  LDFLAGS+=-static -static-libgcc -static-libstdc++
  LDLIBS+=-lrpcrt4
  SGDISK_LDLIBS=-lpopt -lintl -liconv
  LIB_NAMES=guid gptpart bsd parttypes attributes crc32 mbrpart basicmbr mbr gpt support diskio diskio-windows
  MBR_LIBS=support diskio diskio-windows basicmbr mbrpart
  FN_EXTENSION=32.exe
  ifeq ($(DETECTED_OS),Linux)
    # Omit cgdisk when building for Windows under Linux because it doesn't
    # work on my system
    ALL=gdisk sgdisk fixparts
  endif
else ifeq ($(TARGET),freebsd)
  CXX=clang++
  CXXFLAGS+=-O2 -Wall -D_FILE_OFFSET_BITS=64 -I /usr/local/include
  LDFLAGS+=-L/usr/local/lib
  LDLIBS+=-luuid #-licuio
else ifeq ($(TARGET),macos)
  FATBINFLAGS=-arch x86_64 -arch arm64 -mmacosx-version-min=10.9
  THINBINFLAGS=-arch x86_64 -mmacosx-version-min=10.9
  CXXFLAGS=$(FATBINFLAGS) -O2 -Wall -D_FILE_OFFSET_BITS=64 -stdlib=libc++ -I/opt/local/include -I /usr/local/include -I/opt/local/include
  LDLIBS= #-licucore
  CGDISK_LDLIBS=/usr/local/Cellar/ncurses/6.2/lib/libncurses.dylib
else ifeq ($(TARGET),solaris)
  CXXFLAGS+=-Wall -D_FILE_OFFSET_BITS=64 -I/usr/include/ncurses
  LDFLAGS+=-L/lib -licuio -licuuc -luuid
endif

# More default settings, for all OSes....
LIB_OBJS=$(LIB_NAMES:=.o)
MBR_LIB_OBJS=$(MBR_LIBS:=.o)
LIB_HEADERS=$(LIB_NAMES:=.h)
DEPEND= makedepend $(CXXFLAGS)
ALL_EXE=$(ALL:=$(FN_EXTENSION))

all:	$(ALL)

gdisk:	$(LIB_OBJS) gdisk.o gpttext.o
	$(CXX) $(LIB_OBJS) gdisk.o gpttext.o $(LDFLAGS) $(LDLIBS) $(FATBINFLAGS) -o gdisk$(FN_EXTENSION)

cgdisk: $(LIB_OBJS) cgdisk.o gptcurses.o
	$(CXX) $(LIB_OBJS) cgdisk.o gptcurses.o $(LDFLAGS) $(LDLIBS) $(CGDISK_LDLIBS) -o cgdisk$(FN_EXTENSION)

sgdisk: $(LIB_OBJS) sgdisk.o gptcl.o
	$(CXX) $(LIB_OBJS) sgdisk.o gptcl.o $(LDFLAGS) $(LDLIBS) $(SGDISK_LDLIBS) $(THINBINFLAGS) -o sgdisk$(FN_EXTENSION)

fixparts: $(MBR_LIB_OBJS) fixparts.o
	$(CXX) $(MBR_LIB_OBJS) fixparts.o $(LDFLAGS) $(FATBINFLAGS) -o fixparts$(FN_EXTENSION)

test:
	./gdisk_test.sh

lint:	#no pre-reqs
	lint $(SRCS)

clean:	#no pre-reqs
	rm -f core *.o *~ $(ALL_EXE)

strip:	#no pre-reqs
	$(STRIP) $(ALL_EXE)

# what are the source dependencies
depend: $(SRCS)
	$(DEPEND) $(SRCS)

$(OBJS):
	$(CRITICAL_CXX_FLAGS) 

# makedepend dependencies below -- type "makedepend *.cc" to regenerate....
# DO NOT DELETE
