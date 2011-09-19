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

all:	cgdisk gdisk sgdisk fixparts

gdisk:	$(LIB_OBJS) gdisk.o gpttext.o
#	$(CXX) $(LIB_OBJS) gdisk.o gpttext.o $(LDFLAGS) -luuid -o gdisk
	$(CXX) $(LIB_OBJS) gdisk.o gpttext.o $(LDFLAGS) -licuio -licuuc -luuid -o gdisk

cgdisk: $(LIB_OBJS) cgdisk.o gptcurses.o
#	$(CXX) $(LIB_OBJS) cgdisk.o gptcurses.o $(LDFLAGS) -luuid -lncurses -o cgdisk
	$(CXX) $(LIB_OBJS) cgdisk.o gptcurses.o $(LDFLAGS) -licuio -licuuc -luuid -lncurses -o cgdisk

sgdisk: $(LIB_OBJS) sgdisk.o gptcl.o
#	$(CXX) $(LIB_OBJS) sgdisk.o gptcl.o $(LDFLAGS) -luuid -lpopt -o sgdisk
	$(CXX) $(LIB_OBJS) sgdisk.o gptcl.o $(LDFLAGS) -licuio -licuuc -luuid -lpopt -o sgdisk

fixparts: $(MBR_LIB_OBJS) fixparts.o
	$(CXX) $(MBR_LIB_OBJS) fixparts.o $(LDFLAGS) -o fixparts

lint:	#no pre-reqs
	lint $(SRCS)

clean:	#no pre-reqs
	rm -f core *.o *~ gdisk sgdisk cgdisk fixparts

# what are the source dependencies
depend: $(SRCS)
	$(DEPEND) $(SRCS)

$(OBJS):
	$(CRITICAL_CXX_FLAGS) 

# DO NOT DELETE

attributes.o: /usr/include/stdint.h /usr/include/features.h
attributes.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
attributes.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-32.h
attributes.o: /usr/include/bits/wchar.h /usr/include/stdio.h
attributes.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
attributes.o: /usr/include/libio.h /usr/include/_G_config.h
attributes.o: /usr/include/wchar.h /usr/include/bits/libio-ldbl.h
attributes.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
attributes.o: /usr/include/bits/stdio-ldbl.h attributes.h support.h
attributes.o: /usr/include/stdlib.h /usr/include/sys/types.h
attributes.o: /usr/include/time.h /usr/include/endian.h
attributes.o: /usr/include/bits/endian.h /usr/include/sys/select.h
attributes.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
attributes.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
attributes.o: /usr/include/bits/pthreadtypes.h /usr/include/alloca.h
attributes.o: /usr/include/bits/stdlib-ldbl.h
basicmbr.o: /usr/include/stdio.h /usr/include/features.h
basicmbr.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
basicmbr.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-32.h
basicmbr.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
basicmbr.o: /usr/include/libio.h /usr/include/_G_config.h
basicmbr.o: /usr/include/wchar.h /usr/include/bits/libio-ldbl.h
basicmbr.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
basicmbr.o: /usr/include/bits/stdio-ldbl.h /usr/include/stdlib.h
basicmbr.o: /usr/include/sys/types.h /usr/include/time.h
basicmbr.o: /usr/include/endian.h /usr/include/bits/endian.h
basicmbr.o: /usr/include/sys/select.h /usr/include/bits/select.h
basicmbr.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
basicmbr.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
basicmbr.o: /usr/include/alloca.h /usr/include/bits/stdlib-ldbl.h
basicmbr.o: /usr/include/stdint.h /usr/include/bits/wchar.h
basicmbr.o: /usr/include/fcntl.h /usr/include/bits/fcntl.h
basicmbr.o: /usr/include/string.h /usr/include/sys/stat.h
basicmbr.o: /usr/include/bits/stat.h /usr/include/errno.h
basicmbr.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
basicmbr.o: /usr/include/asm/errno.h /usr/include/asm-generic/errno.h
basicmbr.o: /usr/include/asm-generic/errno-base.h mbr.h gptpart.h support.h
basicmbr.o: parttypes.h guid.h /usr/include/uuid/uuid.h
basicmbr.o: /usr/include/sys/time.h attributes.h diskio.h
basicmbr.o: /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h
basicmbr.o: /usr/include/asm/ioctls.h /usr/include/asm/ioctl.h
basicmbr.o: /usr/include/bits/ioctl-types.h /usr/include/termios.h
basicmbr.o: /usr/include/bits/termios.h /usr/include/sys/ttydefaults.h
basicmbr.o: basicmbr.h mbrpart.h
bsd.o: /usr/include/stdio.h /usr/include/features.h /usr/include/sys/cdefs.h
bsd.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
bsd.o: /usr/include/gnu/stubs-32.h /usr/include/bits/types.h
bsd.o: /usr/include/bits/typesizes.h /usr/include/libio.h
bsd.o: /usr/include/_G_config.h /usr/include/wchar.h
bsd.o: /usr/include/bits/libio-ldbl.h /usr/include/bits/stdio_lim.h
bsd.o: /usr/include/bits/sys_errlist.h /usr/include/bits/stdio-ldbl.h
bsd.o: /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h
bsd.o: /usr/include/endian.h /usr/include/bits/endian.h
bsd.o: /usr/include/sys/select.h /usr/include/bits/select.h
bsd.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
bsd.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
bsd.o: /usr/include/alloca.h /usr/include/bits/stdlib-ldbl.h
bsd.o: /usr/include/stdint.h /usr/include/bits/wchar.h /usr/include/fcntl.h
bsd.o: /usr/include/bits/fcntl.h /usr/include/sys/stat.h
bsd.o: /usr/include/bits/stat.h /usr/include/errno.h
bsd.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
bsd.o: /usr/include/asm/errno.h /usr/include/asm-generic/errno.h
bsd.o: /usr/include/asm-generic/errno-base.h support.h bsd.h gptpart.h
bsd.o: parttypes.h guid.h /usr/include/uuid/uuid.h /usr/include/sys/time.h
bsd.o: attributes.h diskio.h /usr/include/sys/ioctl.h
bsd.o: /usr/include/bits/ioctls.h /usr/include/asm/ioctls.h
bsd.o: /usr/include/asm/ioctl.h /usr/include/bits/ioctl-types.h
bsd.o: /usr/include/termios.h /usr/include/bits/termios.h
bsd.o: /usr/include/sys/ttydefaults.h
cgdisk.o: gptcurses.h gptpart.h /usr/include/stdint.h /usr/include/features.h
cgdisk.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
cgdisk.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-32.h
cgdisk.o: /usr/include/bits/wchar.h /usr/include/sys/types.h
cgdisk.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
cgdisk.o: /usr/include/time.h /usr/include/endian.h
cgdisk.o: /usr/include/bits/endian.h /usr/include/sys/select.h
cgdisk.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
cgdisk.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
cgdisk.o: /usr/include/bits/pthreadtypes.h support.h /usr/include/stdlib.h
cgdisk.o: /usr/include/alloca.h /usr/include/bits/stdlib-ldbl.h parttypes.h
cgdisk.o: guid.h /usr/include/uuid/uuid.h /usr/include/sys/time.h
cgdisk.o: attributes.h gpt.h mbr.h diskio.h /usr/include/sys/ioctl.h
cgdisk.o: /usr/include/bits/ioctls.h /usr/include/asm/ioctls.h
cgdisk.o: /usr/include/asm/ioctl.h /usr/include/bits/ioctl-types.h
cgdisk.o: /usr/include/termios.h /usr/include/bits/termios.h
cgdisk.o: /usr/include/sys/ttydefaults.h basicmbr.h mbrpart.h bsd.h
crc32.o: /usr/include/stdio.h /usr/include/features.h
crc32.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
crc32.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-32.h
crc32.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
crc32.o: /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h
crc32.o: /usr/include/bits/libio-ldbl.h /usr/include/bits/stdio_lim.h
crc32.o: /usr/include/bits/sys_errlist.h /usr/include/bits/stdio-ldbl.h
crc32.o: /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h
crc32.o: /usr/include/endian.h /usr/include/bits/endian.h
crc32.o: /usr/include/sys/select.h /usr/include/bits/select.h
crc32.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
crc32.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
crc32.o: /usr/include/alloca.h /usr/include/bits/stdlib-ldbl.h crc32.h
crc32.o: /usr/include/stdint.h /usr/include/bits/wchar.h
diskio.o: /usr/include/sys/ioctl.h /usr/include/features.h
diskio.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
diskio.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-32.h
diskio.o: /usr/include/bits/ioctls.h /usr/include/asm/ioctls.h
diskio.o: /usr/include/asm/ioctl.h /usr/include/bits/ioctl-types.h
diskio.o: /usr/include/termios.h /usr/include/bits/termios.h
diskio.o: /usr/include/sys/ttydefaults.h /usr/include/stdint.h
diskio.o: /usr/include/bits/wchar.h /usr/include/errno.h
diskio.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
diskio.o: /usr/include/asm/errno.h /usr/include/asm-generic/errno.h
diskio.o: /usr/include/asm-generic/errno-base.h /usr/include/fcntl.h
diskio.o: /usr/include/bits/fcntl.h /usr/include/sys/types.h
diskio.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
diskio.o: /usr/include/time.h /usr/include/endian.h
diskio.o: /usr/include/bits/endian.h /usr/include/sys/select.h
diskio.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
diskio.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
diskio.o: /usr/include/bits/pthreadtypes.h /usr/include/sys/stat.h
diskio.o: /usr/include/bits/stat.h support.h /usr/include/stdlib.h
diskio.o: /usr/include/alloca.h /usr/include/bits/stdlib-ldbl.h diskio.h
diskio-unix.o: /usr/include/sys/ioctl.h /usr/include/features.h
diskio-unix.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
diskio-unix.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-32.h
diskio-unix.o: /usr/include/bits/ioctls.h /usr/include/asm/ioctls.h
diskio-unix.o: /usr/include/asm/ioctl.h /usr/include/bits/ioctl-types.h
diskio-unix.o: /usr/include/termios.h /usr/include/bits/termios.h
diskio-unix.o: /usr/include/sys/ttydefaults.h /usr/include/string.h
diskio-unix.o: /usr/include/stdint.h /usr/include/bits/wchar.h
diskio-unix.o: /usr/include/errno.h /usr/include/bits/errno.h
diskio-unix.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
diskio-unix.o: /usr/include/asm-generic/errno.h
diskio-unix.o: /usr/include/asm-generic/errno-base.h /usr/include/fcntl.h
diskio-unix.o: /usr/include/bits/fcntl.h /usr/include/sys/types.h
diskio-unix.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
diskio-unix.o: /usr/include/time.h /usr/include/endian.h
diskio-unix.o: /usr/include/bits/endian.h /usr/include/sys/select.h
diskio-unix.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
diskio-unix.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
diskio-unix.o: /usr/include/bits/pthreadtypes.h /usr/include/sys/stat.h
diskio-unix.o: /usr/include/bits/stat.h diskio.h support.h
diskio-unix.o: /usr/include/stdlib.h /usr/include/alloca.h
diskio-unix.o: /usr/include/bits/stdlib-ldbl.h
diskio-windows.o: /usr/include/stdio.h /usr/include/features.h
diskio-windows.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
diskio-windows.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-32.h
diskio-windows.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
diskio-windows.o: /usr/include/libio.h /usr/include/_G_config.h
diskio-windows.o: /usr/include/wchar.h /usr/include/bits/libio-ldbl.h
diskio-windows.o: /usr/include/bits/stdio_lim.h
diskio-windows.o: /usr/include/bits/sys_errlist.h
diskio-windows.o: /usr/include/bits/stdio-ldbl.h /usr/include/stdint.h
diskio-windows.o: /usr/include/bits/wchar.h /usr/include/errno.h
diskio-windows.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
diskio-windows.o: /usr/include/asm/errno.h /usr/include/asm-generic/errno.h
diskio-windows.o: /usr/include/asm-generic/errno-base.h /usr/include/fcntl.h
diskio-windows.o: /usr/include/bits/fcntl.h /usr/include/sys/types.h
diskio-windows.o: /usr/include/time.h /usr/include/endian.h
diskio-windows.o: /usr/include/bits/endian.h /usr/include/sys/select.h
diskio-windows.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
diskio-windows.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
diskio-windows.o: /usr/include/bits/pthreadtypes.h /usr/include/sys/stat.h
diskio-windows.o: /usr/include/bits/stat.h support.h /usr/include/stdlib.h
diskio-windows.o: /usr/include/alloca.h /usr/include/bits/stdlib-ldbl.h
diskio-windows.o: diskio.h /usr/include/sys/ioctl.h
diskio-windows.o: /usr/include/bits/ioctls.h /usr/include/asm/ioctls.h
diskio-windows.o: /usr/include/asm/ioctl.h /usr/include/bits/ioctl-types.h
diskio-windows.o: /usr/include/termios.h /usr/include/bits/termios.h
diskio-windows.o: /usr/include/sys/ttydefaults.h
fixparts.o: /usr/include/stdio.h /usr/include/features.h
fixparts.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
fixparts.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-32.h
fixparts.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
fixparts.o: /usr/include/libio.h /usr/include/_G_config.h
fixparts.o: /usr/include/wchar.h /usr/include/bits/libio-ldbl.h
fixparts.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
fixparts.o: /usr/include/bits/stdio-ldbl.h /usr/include/string.h basicmbr.h
fixparts.o: /usr/include/stdint.h /usr/include/bits/wchar.h
fixparts.o: /usr/include/sys/types.h /usr/include/time.h
fixparts.o: /usr/include/endian.h /usr/include/bits/endian.h
fixparts.o: /usr/include/sys/select.h /usr/include/bits/select.h
fixparts.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
fixparts.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
fixparts.o: diskio.h /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h
fixparts.o: /usr/include/asm/ioctls.h /usr/include/asm/ioctl.h
fixparts.o: /usr/include/bits/ioctl-types.h /usr/include/termios.h
fixparts.o: /usr/include/bits/termios.h /usr/include/sys/ttydefaults.h
fixparts.o: support.h /usr/include/stdlib.h /usr/include/alloca.h
fixparts.o: /usr/include/bits/stdlib-ldbl.h mbrpart.h
gdisk.o: /usr/include/string.h /usr/include/features.h
gdisk.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
gdisk.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-32.h gpttext.h gpt.h
gdisk.o: /usr/include/stdint.h /usr/include/bits/wchar.h
gdisk.o: /usr/include/sys/types.h /usr/include/bits/types.h
gdisk.o: /usr/include/bits/typesizes.h /usr/include/time.h
gdisk.o: /usr/include/endian.h /usr/include/bits/endian.h
gdisk.o: /usr/include/sys/select.h /usr/include/bits/select.h
gdisk.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
gdisk.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
gdisk.o: gptpart.h support.h /usr/include/stdlib.h /usr/include/alloca.h
gdisk.o: /usr/include/bits/stdlib-ldbl.h parttypes.h guid.h
gdisk.o: /usr/include/uuid/uuid.h /usr/include/sys/time.h attributes.h mbr.h
gdisk.o: diskio.h /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h
gdisk.o: /usr/include/asm/ioctls.h /usr/include/asm/ioctl.h
gdisk.o: /usr/include/bits/ioctl-types.h /usr/include/termios.h
gdisk.o: /usr/include/bits/termios.h /usr/include/sys/ttydefaults.h
gdisk.o: basicmbr.h mbrpart.h bsd.h
gpt.o: /usr/include/stdio.h /usr/include/features.h /usr/include/sys/cdefs.h
gpt.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
gpt.o: /usr/include/gnu/stubs-32.h /usr/include/bits/types.h
gpt.o: /usr/include/bits/typesizes.h /usr/include/libio.h
gpt.o: /usr/include/_G_config.h /usr/include/wchar.h
gpt.o: /usr/include/bits/libio-ldbl.h /usr/include/bits/stdio_lim.h
gpt.o: /usr/include/bits/sys_errlist.h /usr/include/bits/stdio-ldbl.h
gpt.o: /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h
gpt.o: /usr/include/endian.h /usr/include/bits/endian.h
gpt.o: /usr/include/sys/select.h /usr/include/bits/select.h
gpt.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
gpt.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
gpt.o: /usr/include/alloca.h /usr/include/bits/stdlib-ldbl.h
gpt.o: /usr/include/stdint.h /usr/include/bits/wchar.h /usr/include/fcntl.h
gpt.o: /usr/include/bits/fcntl.h /usr/include/string.h /usr/include/math.h
gpt.o: /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h
gpt.o: /usr/include/bits/mathcalls.h /usr/include/sys/stat.h
gpt.o: /usr/include/bits/stat.h /usr/include/errno.h
gpt.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
gpt.o: /usr/include/asm/errno.h /usr/include/asm-generic/errno.h
gpt.o: /usr/include/asm-generic/errno-base.h crc32.h gpt.h gptpart.h
gpt.o: support.h parttypes.h guid.h /usr/include/uuid/uuid.h
gpt.o: /usr/include/sys/time.h attributes.h mbr.h diskio.h
gpt.o: /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h
gpt.o: /usr/include/asm/ioctls.h /usr/include/asm/ioctl.h
gpt.o: /usr/include/bits/ioctl-types.h /usr/include/termios.h
gpt.o: /usr/include/bits/termios.h /usr/include/sys/ttydefaults.h basicmbr.h
gpt.o: mbrpart.h bsd.h
gptcl.o: /usr/include/string.h /usr/include/features.h
gptcl.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
gptcl.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-32.h
gptcl.o: /usr/include/errno.h /usr/include/bits/errno.h
gptcl.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
gptcl.o: /usr/include/asm-generic/errno.h
gptcl.o: /usr/include/asm-generic/errno-base.h /usr/include/popt.h
gptcl.o: /usr/include/stdio.h /usr/include/bits/types.h
gptcl.o: /usr/include/bits/typesizes.h /usr/include/libio.h
gptcl.o: /usr/include/_G_config.h /usr/include/wchar.h
gptcl.o: /usr/include/bits/libio-ldbl.h /usr/include/bits/stdio_lim.h
gptcl.o: /usr/include/bits/sys_errlist.h /usr/include/bits/stdio-ldbl.h
gptcl.o: gptcl.h gpt.h /usr/include/stdint.h /usr/include/bits/wchar.h
gptcl.o: /usr/include/sys/types.h /usr/include/time.h /usr/include/endian.h
gptcl.o: /usr/include/bits/endian.h /usr/include/sys/select.h
gptcl.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
gptcl.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
gptcl.o: /usr/include/bits/pthreadtypes.h gptpart.h support.h
gptcl.o: /usr/include/stdlib.h /usr/include/alloca.h
gptcl.o: /usr/include/bits/stdlib-ldbl.h parttypes.h guid.h
gptcl.o: /usr/include/uuid/uuid.h /usr/include/sys/time.h attributes.h mbr.h
gptcl.o: diskio.h /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h
gptcl.o: /usr/include/asm/ioctls.h /usr/include/asm/ioctl.h
gptcl.o: /usr/include/bits/ioctl-types.h /usr/include/termios.h
gptcl.o: /usr/include/bits/termios.h /usr/include/sys/ttydefaults.h
gptcl.o: basicmbr.h mbrpart.h bsd.h
gptcurses.o: /usr/include/ncurses.h /usr/include/ncurses_dll.h
gptcurses.o: /usr/include/stdio.h /usr/include/features.h
gptcurses.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
gptcurses.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-32.h
gptcurses.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
gptcurses.o: /usr/include/libio.h /usr/include/_G_config.h
gptcurses.o: /usr/include/wchar.h /usr/include/bits/libio-ldbl.h
gptcurses.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
gptcurses.o: /usr/include/bits/stdio-ldbl.h /usr/include/unctrl.h
gptcurses.o: /usr/include/curses.h gptcurses.h gptpart.h
gptcurses.o: /usr/include/stdint.h /usr/include/bits/wchar.h
gptcurses.o: /usr/include/sys/types.h /usr/include/time.h
gptcurses.o: /usr/include/endian.h /usr/include/bits/endian.h
gptcurses.o: /usr/include/sys/select.h /usr/include/bits/select.h
gptcurses.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
gptcurses.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
gptcurses.o: support.h /usr/include/stdlib.h /usr/include/alloca.h
gptcurses.o: /usr/include/bits/stdlib-ldbl.h parttypes.h guid.h
gptcurses.o: /usr/include/uuid/uuid.h /usr/include/sys/time.h attributes.h
gptcurses.o: gpt.h mbr.h diskio.h /usr/include/sys/ioctl.h
gptcurses.o: /usr/include/bits/ioctls.h /usr/include/asm/ioctls.h
gptcurses.o: /usr/include/asm/ioctl.h /usr/include/bits/ioctl-types.h
gptcurses.o: /usr/include/termios.h /usr/include/bits/termios.h
gptcurses.o: /usr/include/sys/ttydefaults.h basicmbr.h mbrpart.h bsd.h
gptpart.o: /usr/include/string.h /usr/include/features.h
gptpart.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
gptpart.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-32.h
gptpart.o: /usr/include/stdio.h /usr/include/bits/types.h
gptpart.o: /usr/include/bits/typesizes.h /usr/include/libio.h
gptpart.o: /usr/include/_G_config.h /usr/include/wchar.h
gptpart.o: /usr/include/bits/libio-ldbl.h /usr/include/bits/stdio_lim.h
gptpart.o: /usr/include/bits/sys_errlist.h /usr/include/bits/stdio-ldbl.h
gptpart.o: gptpart.h /usr/include/stdint.h /usr/include/bits/wchar.h
gptpart.o: /usr/include/sys/types.h /usr/include/time.h /usr/include/endian.h
gptpart.o: /usr/include/bits/endian.h /usr/include/sys/select.h
gptpart.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
gptpart.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
gptpart.o: /usr/include/bits/pthreadtypes.h support.h /usr/include/stdlib.h
gptpart.o: /usr/include/alloca.h /usr/include/bits/stdlib-ldbl.h parttypes.h
gptpart.o: guid.h /usr/include/uuid/uuid.h /usr/include/sys/time.h
gptpart.o: attributes.h
gpttext.o: /usr/include/string.h /usr/include/features.h
gpttext.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
gpttext.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-32.h
gpttext.o: /usr/include/errno.h /usr/include/bits/errno.h
gpttext.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
gpttext.o: /usr/include/asm-generic/errno.h
gpttext.o: /usr/include/asm-generic/errno-base.h /usr/include/stdint.h
gpttext.o: /usr/include/bits/wchar.h /usr/include/limits.h
gpttext.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
gpttext.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
gpttext.o: attributes.h gpttext.h gpt.h /usr/include/sys/types.h
gpttext.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
gpttext.o: /usr/include/time.h /usr/include/endian.h
gpttext.o: /usr/include/bits/endian.h /usr/include/sys/select.h
gpttext.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
gpttext.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
gpttext.o: /usr/include/bits/pthreadtypes.h gptpart.h support.h
gpttext.o: /usr/include/stdlib.h /usr/include/alloca.h
gpttext.o: /usr/include/bits/stdlib-ldbl.h parttypes.h guid.h
gpttext.o: /usr/include/uuid/uuid.h /usr/include/sys/time.h mbr.h diskio.h
gpttext.o: /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h
gpttext.o: /usr/include/asm/ioctls.h /usr/include/asm/ioctl.h
gpttext.o: /usr/include/bits/ioctl-types.h /usr/include/termios.h
gpttext.o: /usr/include/bits/termios.h /usr/include/sys/ttydefaults.h
gpttext.o: basicmbr.h mbrpart.h bsd.h
guid.o: /usr/include/stdio.h /usr/include/features.h /usr/include/sys/cdefs.h
guid.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
guid.o: /usr/include/gnu/stubs-32.h /usr/include/bits/types.h
guid.o: /usr/include/bits/typesizes.h /usr/include/libio.h
guid.o: /usr/include/_G_config.h /usr/include/wchar.h
guid.o: /usr/include/bits/libio-ldbl.h /usr/include/bits/stdio_lim.h
guid.o: /usr/include/bits/sys_errlist.h /usr/include/bits/stdio-ldbl.h
guid.o: /usr/include/time.h /usr/include/string.h guid.h
guid.o: /usr/include/stdint.h /usr/include/bits/wchar.h
guid.o: /usr/include/uuid/uuid.h /usr/include/sys/types.h
guid.o: /usr/include/endian.h /usr/include/bits/endian.h
guid.o: /usr/include/sys/select.h /usr/include/bits/select.h
guid.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
guid.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
guid.o: /usr/include/sys/time.h support.h /usr/include/stdlib.h
guid.o: /usr/include/alloca.h /usr/include/bits/stdlib-ldbl.h
mbr.o: /usr/include/stdio.h /usr/include/features.h /usr/include/sys/cdefs.h
mbr.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
mbr.o: /usr/include/gnu/stubs-32.h /usr/include/bits/types.h
mbr.o: /usr/include/bits/typesizes.h /usr/include/libio.h
mbr.o: /usr/include/_G_config.h /usr/include/wchar.h
mbr.o: /usr/include/bits/libio-ldbl.h /usr/include/bits/stdio_lim.h
mbr.o: /usr/include/bits/sys_errlist.h /usr/include/bits/stdio-ldbl.h
mbr.o: /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h
mbr.o: /usr/include/endian.h /usr/include/bits/endian.h
mbr.o: /usr/include/sys/select.h /usr/include/bits/select.h
mbr.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
mbr.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
mbr.o: /usr/include/alloca.h /usr/include/bits/stdlib-ldbl.h
mbr.o: /usr/include/stdint.h /usr/include/bits/wchar.h /usr/include/fcntl.h
mbr.o: /usr/include/bits/fcntl.h /usr/include/string.h
mbr.o: /usr/include/sys/stat.h /usr/include/bits/stat.h /usr/include/errno.h
mbr.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
mbr.o: /usr/include/asm/errno.h /usr/include/asm-generic/errno.h
mbr.o: /usr/include/asm-generic/errno-base.h mbr.h gptpart.h support.h
mbr.o: parttypes.h guid.h /usr/include/uuid/uuid.h /usr/include/sys/time.h
mbr.o: attributes.h diskio.h /usr/include/sys/ioctl.h
mbr.o: /usr/include/bits/ioctls.h /usr/include/asm/ioctls.h
mbr.o: /usr/include/asm/ioctl.h /usr/include/bits/ioctl-types.h
mbr.o: /usr/include/termios.h /usr/include/bits/termios.h
mbr.o: /usr/include/sys/ttydefaults.h basicmbr.h mbrpart.h
mbrpart.o: /usr/include/stdint.h /usr/include/features.h
mbrpart.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
mbrpart.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-32.h
mbrpart.o: /usr/include/bits/wchar.h support.h /usr/include/stdlib.h
mbrpart.o: /usr/include/sys/types.h /usr/include/bits/types.h
mbrpart.o: /usr/include/bits/typesizes.h /usr/include/time.h
mbrpart.o: /usr/include/endian.h /usr/include/bits/endian.h
mbrpart.o: /usr/include/sys/select.h /usr/include/bits/select.h
mbrpart.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
mbrpart.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
mbrpart.o: /usr/include/alloca.h /usr/include/bits/stdlib-ldbl.h mbrpart.h
parttypes.o: /usr/include/string.h /usr/include/features.h
parttypes.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
parttypes.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-32.h
parttypes.o: /usr/include/stdint.h /usr/include/bits/wchar.h
parttypes.o: /usr/include/stdio.h /usr/include/bits/types.h
parttypes.o: /usr/include/bits/typesizes.h /usr/include/libio.h
parttypes.o: /usr/include/_G_config.h /usr/include/wchar.h
parttypes.o: /usr/include/bits/libio-ldbl.h /usr/include/bits/stdio_lim.h
parttypes.o: /usr/include/bits/sys_errlist.h /usr/include/bits/stdio-ldbl.h
parttypes.o: parttypes.h /usr/include/stdlib.h /usr/include/sys/types.h
parttypes.o: /usr/include/time.h /usr/include/endian.h
parttypes.o: /usr/include/bits/endian.h /usr/include/sys/select.h
parttypes.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
parttypes.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
parttypes.o: /usr/include/bits/pthreadtypes.h /usr/include/alloca.h
parttypes.o: /usr/include/bits/stdlib-ldbl.h support.h guid.h
parttypes.o: /usr/include/uuid/uuid.h /usr/include/sys/time.h
sgdisk.o: gptcl.h gpt.h /usr/include/stdint.h /usr/include/features.h
sgdisk.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
sgdisk.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-32.h
sgdisk.o: /usr/include/bits/wchar.h /usr/include/sys/types.h
sgdisk.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
sgdisk.o: /usr/include/time.h /usr/include/endian.h
sgdisk.o: /usr/include/bits/endian.h /usr/include/sys/select.h
sgdisk.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
sgdisk.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
sgdisk.o: /usr/include/bits/pthreadtypes.h gptpart.h support.h
sgdisk.o: /usr/include/stdlib.h /usr/include/alloca.h
sgdisk.o: /usr/include/bits/stdlib-ldbl.h parttypes.h guid.h
sgdisk.o: /usr/include/uuid/uuid.h /usr/include/sys/time.h attributes.h mbr.h
sgdisk.o: diskio.h /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h
sgdisk.o: /usr/include/asm/ioctls.h /usr/include/asm/ioctl.h
sgdisk.o: /usr/include/bits/ioctl-types.h /usr/include/termios.h
sgdisk.o: /usr/include/bits/termios.h /usr/include/sys/ttydefaults.h
sgdisk.o: basicmbr.h mbrpart.h bsd.h /usr/include/popt.h /usr/include/stdio.h
sgdisk.o: /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h
sgdisk.o: /usr/include/bits/libio-ldbl.h /usr/include/bits/stdio_lim.h
sgdisk.o: /usr/include/bits/sys_errlist.h /usr/include/bits/stdio-ldbl.h
support.o: /usr/include/stdio.h /usr/include/features.h
support.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
support.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-32.h
support.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
support.o: /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h
support.o: /usr/include/bits/libio-ldbl.h /usr/include/bits/stdio_lim.h
support.o: /usr/include/bits/sys_errlist.h /usr/include/bits/stdio-ldbl.h
support.o: /usr/include/stdint.h /usr/include/bits/wchar.h
support.o: /usr/include/errno.h /usr/include/bits/errno.h
support.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
support.o: /usr/include/asm-generic/errno.h
support.o: /usr/include/asm-generic/errno-base.h /usr/include/fcntl.h
support.o: /usr/include/bits/fcntl.h /usr/include/sys/types.h
support.o: /usr/include/time.h /usr/include/endian.h
support.o: /usr/include/bits/endian.h /usr/include/sys/select.h
support.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
support.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
support.o: /usr/include/bits/pthreadtypes.h /usr/include/string.h
support.o: /usr/include/sys/stat.h /usr/include/bits/stat.h support.h
support.o: /usr/include/stdlib.h /usr/include/alloca.h
support.o: /usr/include/bits/stdlib-ldbl.h
test.o: /usr/include/stdio.h /usr/include/features.h /usr/include/sys/cdefs.h
test.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
test.o: /usr/include/gnu/stubs-32.h /usr/include/bits/types.h
test.o: /usr/include/bits/typesizes.h /usr/include/libio.h
test.o: /usr/include/_G_config.h /usr/include/wchar.h
test.o: /usr/include/bits/libio-ldbl.h /usr/include/bits/stdio_lim.h
test.o: /usr/include/bits/sys_errlist.h /usr/include/bits/stdio-ldbl.h
test.o: support.h /usr/include/stdint.h /usr/include/bits/wchar.h
test.o: /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h
test.o: /usr/include/endian.h /usr/include/bits/endian.h
test.o: /usr/include/sys/select.h /usr/include/bits/select.h
test.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
test.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
test.o: /usr/include/alloca.h /usr/include/bits/stdlib-ldbl.h
testguid.o: guid.h /usr/include/stdint.h /usr/include/features.h
testguid.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
testguid.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-32.h
testguid.o: /usr/include/bits/wchar.h /usr/include/uuid/uuid.h
testguid.o: /usr/include/sys/types.h /usr/include/bits/types.h
testguid.o: /usr/include/bits/typesizes.h /usr/include/time.h
testguid.o: /usr/include/endian.h /usr/include/bits/endian.h
testguid.o: /usr/include/sys/select.h /usr/include/bits/select.h
testguid.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
testguid.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
testguid.o: /usr/include/sys/time.h parttypes.h /usr/include/stdlib.h
testguid.o: /usr/include/alloca.h /usr/include/bits/stdlib-ldbl.h support.h
