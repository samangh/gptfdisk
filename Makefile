CC=gcc
CXX=g++
CFLAGS=-O2 -D_FILE_OFFSET_BITS=64 -g
CXXFLAGS=-O2 -Wall -D_FILE_OFFSET_BITS=64 -I /usr/local/include -I/opt/local/include -g
LIB_NAMES=crc32 support gptpart mbr gpt bsd parttypes attributes diskio diskio-unix
LIB_SRCS=$(NAMES:=.cc)
LIB_OBJS=$(LIB_NAMES:=.o)
LIB_HEADERS=$(LIB_NAMES:=.h)
DEPEND= makedepend $(CFLAGS)

all:	gdisk sgdisk

gdisk:	$(LIB_OBJS) gdisk.o
	$(CXX) $(LIB_OBJS) gdisk.o -o gdisk

sgdisk: $(LIB_OBJS) sgdisk.o
	$(CXX) $(LIB_OBJS) sgdisk.o -L/opt/local/lib -L/usr/local/lib -lpopt -o sgdisk

lint:	#no pre-reqs
	lint $(SRCS)

clean:	#no pre-reqs
	rm -f core *.o *~ gdisk sgdisk

# what are the source dependencies
depend: $(SRCS)
	$(DEPEND) $(SRCS)

$(OBJS):

# DO NOT DELETE
