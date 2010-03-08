CC=gcc
CXX=g++
CFLAGS+=-D_FILE_OFFSET_BITS=64
CXXFLAGS+=-Wall -D_FILE_OFFSET_BITS=64
LDFLAGS+=
LIB_NAMES=crc32 support guid partnotes gptpart mbr gpt bsd parttypes attributes diskio diskio-unix
LIB_SRCS=$(NAMES:=.cc)
LIB_OBJS=$(LIB_NAMES:=.o)
LIB_HEADERS=$(LIB_NAMES:=.h)
DEPEND= makedepend $(CFLAGS)

all:	gdisk sgdisk

gdisk:	$(LIB_OBJS) gdisk.o gpttext.o
	$(CXX) $(LIB_OBJS) gdisk.o gpttext.o -L/opt/local/lib -L/usr/local/lib $(LDFLAGS) -luuid -o gdisk

sgdisk: $(LIB_OBJS) sgdisk.o
	$(CXX) $(LIB_OBJS) sgdisk.o -L/opt/local/lib -L/usr/local/lib $(LDFLAGS) -luuid -lpopt -o sgdisk

testguid:	$(LIB_OBJS) testguid.o
	$(CXX) $(LIB_OBJS) testguid.o -o testguid

lint:	#no pre-reqs
	lint $(SRCS)

clean:	#no pre-reqs
	rm -f core *.o *~ gdisk sgdisk

# what are the source dependencies
depend: $(SRCS)
	$(DEPEND) $(SRCS)

$(OBJS):

# DO NOT DELETE
