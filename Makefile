CC=gcc
CXX=g++
CFLAGS+=-D_FILE_OFFSET_BITS=64
CXXFLAGS+=-Wall -D_FILE_OFFSET_BITS=64 -D USE_UTF16
#CXXFLAGS+=-Wall -D_FILE_OFFSET_BITS=64
LDFLAGS+=
LIB_NAMES=crc32 support guid gptpart mbrpart basicmbr mbr gpt bsd parttypes attributes diskio diskio-unix
MBR_LIBS=support diskio diskio-unix basicmbr mbrpart
LIB_OBJS=$(LIB_NAMES:=.o)
MBR_LIB_OBJS=$(MBR_LIBS:=.o)
LIB_HEADERS=$(LIB_NAMES:=.h)
DEPEND= makedepend $(CXXFLAGS)

all:	gdisk sgdisk fixparts

gdisk:	$(LIB_OBJS) gdisk.o gpttext.o
#	$(CXX) $(LIB_OBJS) gdisk.o gpttext.o $(LDFLAGS) -luuid -o gdisk
	$(CXX) $(LIB_OBJS) gdisk.o gpttext.o $(LDFLAGS) -licuio -licuuc -luuid -o gdisk

sgdisk: $(LIB_OBJS) sgdisk.o
#	$(CXX) $(LIB_OBJS) sgdisk.o $(LDFLAGS) -luuid -lpopt -o sgdisk
	$(CXX) $(LIB_OBJS) sgdisk.o $(LDFLAGS) -licuio -licuuc -luuid -lpopt -o sgdisk

fixparts: $(MBR_LIB_OBJS) fixparts.o
	$(CXX) $(MBR_LIB_OBJS) fixparts.o $(LDFLAGS) -o fixparts

lint:	#no pre-reqs
	lint $(SRCS)

clean:	#no pre-reqs
	rm -f core *.o *~ gdisk sgdisk fixparts

# what are the source dependencies
depend: $(SRCS)
	$(DEPEND) $(SRCS)

$(OBJS):
	$(CRITICAL_CXX_FLAGS) 

# DO NOT DELETE
