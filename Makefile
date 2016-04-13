CC=gcc
CXX=g++
CFLAGS+=-D_FILE_OFFSET_BITS=64
#CXXFLAGS+=-Wall -D_FILE_OFFSET_BITS=64 -D USE_UTF16
CXXFLAGS+=-Wall -D_FILE_OFFSET_BITS=64
LDFLAGS+=
LIB_NAMES=crc32 support guid gptpart mbrpart basicmbr mbr gpt bsd parttypes attributes diskio diskio-unix
MBR_LIBS=support diskio diskio-unix basicmbr mbrpart
LIB_OBJS=$(LIB_NAMES:=.o)
MBR_LIB_OBJS=$(MBR_LIBS:=.o)
LIB_HEADERS=$(LIB_NAMES:=.h)
DEPEND= makedepend $(CXXFLAGS)

all:	cgdisk gdisk sgdisk fixparts

gdisk:	$(LIB_OBJS) gdisk.o gpttext.o
	$(CXX) $(LIB_OBJS) gdisk.o gpttext.o $(LDFLAGS) -luuid -o gdisk
#	$(CXX) $(LIB_OBJS) gdisk.o gpttext.o $(LDFLAGS) -licuio -licuuc -luuid -o gdisk

cgdisk: $(LIB_OBJS) cgdisk.o gptcurses.o
	$(CXX) $(LIB_OBJS) cgdisk.o gptcurses.o $(LDFLAGS) -luuid -lncursesw -o cgdisk
#	$(CXX) $(LIB_OBJS) cgdisk.o gptcurses.o $(LDFLAGS) -licuio -licuuc -luuid -lncurses -o cgdisk

sgdisk: $(LIB_OBJS) sgdisk.o gptcl.o
	$(CXX) $(LIB_OBJS) sgdisk.o gptcl.o $(LDFLAGS) -luuid -lpopt -o sgdisk
#	$(CXX) $(LIB_OBJS) sgdisk.o gptcl.o $(LDFLAGS) -licuio -licuuc -luuid -lpopt -o sgdisk

fixparts: $(MBR_LIB_OBJS) fixparts.o
	$(CXX) $(MBR_LIB_OBJS) fixparts.o $(LDFLAGS) -o fixparts

test:
	./gdisk_test.sh

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
attributes.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
attributes.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
attributes.o: /usr/include/bits/wchar.h /usr/include/stdio.h
attributes.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
attributes.o: /usr/include/libio.h /usr/include/_G_config.h
attributes.o: /usr/include/wchar.h /usr/include/xlocale.h
attributes.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
attributes.o: attributes.h support.h /usr/include/stdlib.h
attributes.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
attributes.o: /usr/include/endian.h /usr/include/bits/endian.h
attributes.o: /usr/include/bits/byteswap.h /usr/include/bits/byteswap-16.h
attributes.o: /usr/include/sys/types.h /usr/include/time.h
attributes.o: /usr/include/sys/select.h /usr/include/bits/select.h
attributes.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
attributes.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
attributes.o: /usr/include/alloca.h /usr/include/bits/stdlib-float.h
basicmbr.o: /usr/include/stdio.h /usr/include/features.h
basicmbr.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
basicmbr.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
basicmbr.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
basicmbr.o: /usr/include/libio.h /usr/include/_G_config.h
basicmbr.o: /usr/include/wchar.h /usr/include/bits/wchar.h
basicmbr.o: /usr/include/xlocale.h /usr/include/bits/stdio_lim.h
basicmbr.o: /usr/include/bits/sys_errlist.h /usr/include/stdlib.h
basicmbr.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
basicmbr.o: /usr/include/endian.h /usr/include/bits/endian.h
basicmbr.o: /usr/include/bits/byteswap.h /usr/include/bits/byteswap-16.h
basicmbr.o: /usr/include/sys/types.h /usr/include/time.h
basicmbr.o: /usr/include/sys/select.h /usr/include/bits/select.h
basicmbr.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
basicmbr.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
basicmbr.o: /usr/include/alloca.h /usr/include/bits/stdlib-float.h
basicmbr.o: /usr/include/stdint.h /usr/include/fcntl.h
basicmbr.o: /usr/include/bits/fcntl.h /usr/include/bits/fcntl-linux.h
basicmbr.o: /usr/include/bits/stat.h /usr/include/string.h
basicmbr.o: /usr/include/sys/stat.h /usr/include/errno.h
basicmbr.o: /usr/include/bits/errno.h /usr/include/linux/errno.h mbr.h
basicmbr.o: gptpart.h support.h parttypes.h guid.h /usr/include/uuid/uuid.h
basicmbr.o: /usr/include/sys/time.h attributes.h diskio.h
basicmbr.o: /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h
basicmbr.o: /usr/include/bits/ioctl-types.h /usr/include/sys/ttydefaults.h
basicmbr.o: basicmbr.h mbrpart.h
bsd.o: /usr/include/stdio.h /usr/include/features.h
bsd.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
bsd.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
bsd.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
bsd.o: /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h
bsd.o: /usr/include/bits/wchar.h /usr/include/xlocale.h
bsd.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
bsd.o: /usr/include/stdlib.h /usr/include/bits/waitflags.h
bsd.o: /usr/include/bits/waitstatus.h /usr/include/endian.h
bsd.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
bsd.o: /usr/include/bits/byteswap-16.h /usr/include/sys/types.h
bsd.o: /usr/include/time.h /usr/include/sys/select.h
bsd.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
bsd.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
bsd.o: /usr/include/bits/pthreadtypes.h /usr/include/alloca.h
bsd.o: /usr/include/bits/stdlib-float.h /usr/include/stdint.h
bsd.o: /usr/include/fcntl.h /usr/include/bits/fcntl.h
bsd.o: /usr/include/bits/fcntl-linux.h /usr/include/bits/stat.h
bsd.o: /usr/include/sys/stat.h /usr/include/errno.h /usr/include/bits/errno.h
bsd.o: /usr/include/linux/errno.h support.h bsd.h gptpart.h parttypes.h
bsd.o: guid.h /usr/include/uuid/uuid.h /usr/include/sys/time.h attributes.h
bsd.o: diskio.h /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h
bsd.o: /usr/include/bits/ioctl-types.h /usr/include/sys/ttydefaults.h
cgdisk.o: /usr/include/string.h /usr/include/features.h
cgdisk.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
cgdisk.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
cgdisk.o: /usr/include/xlocale.h gptcurses.h gptpart.h /usr/include/stdint.h
cgdisk.o: /usr/include/bits/wchar.h /usr/include/sys/types.h
cgdisk.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
cgdisk.o: /usr/include/time.h /usr/include/endian.h
cgdisk.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
cgdisk.o: /usr/include/bits/byteswap-16.h /usr/include/sys/select.h
cgdisk.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
cgdisk.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
cgdisk.o: /usr/include/bits/pthreadtypes.h support.h /usr/include/stdlib.h
cgdisk.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
cgdisk.o: /usr/include/alloca.h /usr/include/bits/stdlib-float.h parttypes.h
cgdisk.o: guid.h /usr/include/uuid/uuid.h /usr/include/sys/time.h
cgdisk.o: attributes.h gpt.h mbr.h diskio.h /usr/include/sys/ioctl.h
cgdisk.o: /usr/include/bits/ioctls.h /usr/include/bits/ioctl-types.h
cgdisk.o: /usr/include/sys/ttydefaults.h basicmbr.h mbrpart.h bsd.h
crc32.o: /usr/include/stdio.h /usr/include/features.h
crc32.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
crc32.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
crc32.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
crc32.o: /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h
crc32.o: /usr/include/bits/wchar.h /usr/include/xlocale.h
crc32.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
crc32.o: /usr/include/stdlib.h /usr/include/bits/waitflags.h
crc32.o: /usr/include/bits/waitstatus.h /usr/include/endian.h
crc32.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
crc32.o: /usr/include/bits/byteswap-16.h /usr/include/sys/types.h
crc32.o: /usr/include/time.h /usr/include/sys/select.h
crc32.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
crc32.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
crc32.o: /usr/include/bits/pthreadtypes.h /usr/include/alloca.h
crc32.o: /usr/include/bits/stdlib-float.h crc32.h /usr/include/stdint.h
diskio.o: /usr/include/sys/ioctl.h /usr/include/features.h
diskio.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
diskio.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
diskio.o: /usr/include/bits/ioctls.h /usr/include/bits/ioctl-types.h
diskio.o: /usr/include/sys/ttydefaults.h /usr/include/stdint.h
diskio.o: /usr/include/bits/wchar.h /usr/include/errno.h
diskio.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
diskio.o: /usr/include/fcntl.h /usr/include/bits/types.h
diskio.o: /usr/include/bits/typesizes.h /usr/include/bits/fcntl.h
diskio.o: /usr/include/bits/fcntl-linux.h /usr/include/time.h
diskio.o: /usr/include/bits/stat.h /usr/include/sys/stat.h support.h
diskio.o: /usr/include/stdlib.h /usr/include/bits/waitflags.h
diskio.o: /usr/include/bits/waitstatus.h /usr/include/endian.h
diskio.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
diskio.o: /usr/include/bits/byteswap-16.h /usr/include/sys/types.h
diskio.o: /usr/include/sys/select.h /usr/include/bits/select.h
diskio.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
diskio.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
diskio.o: /usr/include/alloca.h /usr/include/bits/stdlib-float.h diskio.h
diskio-unix.o: /usr/include/sys/ioctl.h /usr/include/features.h
diskio-unix.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
diskio-unix.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
diskio-unix.o: /usr/include/bits/ioctls.h /usr/include/bits/ioctl-types.h
diskio-unix.o: /usr/include/sys/ttydefaults.h /usr/include/string.h
diskio-unix.o: /usr/include/xlocale.h /usr/include/stdint.h
diskio-unix.o: /usr/include/bits/wchar.h /usr/include/unistd.h
diskio-unix.o: /usr/include/bits/posix_opt.h /usr/include/bits/types.h
diskio-unix.o: /usr/include/bits/typesizes.h /usr/include/bits/confname.h
diskio-unix.o: /usr/include/getopt.h /usr/include/errno.h
diskio-unix.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
diskio-unix.o: /usr/include/fcntl.h /usr/include/bits/fcntl.h
diskio-unix.o: /usr/include/bits/fcntl-linux.h /usr/include/time.h
diskio-unix.o: /usr/include/bits/stat.h /usr/include/sys/stat.h diskio.h
diskio-unix.o: /usr/include/sys/types.h /usr/include/endian.h
diskio-unix.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
diskio-unix.o: /usr/include/bits/byteswap-16.h /usr/include/sys/select.h
diskio-unix.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
diskio-unix.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
diskio-unix.o: /usr/include/bits/pthreadtypes.h support.h
diskio-unix.o: /usr/include/stdlib.h /usr/include/bits/waitflags.h
diskio-unix.o: /usr/include/bits/waitstatus.h /usr/include/alloca.h
diskio-unix.o: /usr/include/bits/stdlib-float.h
diskio-unix-orig.o: /usr/include/sys/ioctl.h /usr/include/features.h
diskio-unix-orig.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
diskio-unix-orig.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
diskio-unix-orig.o: /usr/include/bits/ioctls.h
diskio-unix-orig.o: /usr/include/bits/ioctl-types.h
diskio-unix-orig.o: /usr/include/sys/ttydefaults.h /usr/include/string.h
diskio-unix-orig.o: /usr/include/xlocale.h /usr/include/stdint.h
diskio-unix-orig.o: /usr/include/bits/wchar.h /usr/include/unistd.h
diskio-unix-orig.o: /usr/include/bits/posix_opt.h /usr/include/bits/types.h
diskio-unix-orig.o: /usr/include/bits/typesizes.h
diskio-unix-orig.o: /usr/include/bits/confname.h /usr/include/getopt.h
diskio-unix-orig.o: /usr/include/errno.h /usr/include/bits/errno.h
diskio-unix-orig.o: /usr/include/linux/errno.h /usr/include/fcntl.h
diskio-unix-orig.o: /usr/include/bits/fcntl.h /usr/include/bits/fcntl-linux.h
diskio-unix-orig.o: /usr/include/time.h /usr/include/bits/stat.h
diskio-unix-orig.o: /usr/include/sys/stat.h diskio.h /usr/include/sys/types.h
diskio-unix-orig.o: /usr/include/endian.h /usr/include/bits/endian.h
diskio-unix-orig.o: /usr/include/bits/byteswap.h
diskio-unix-orig.o: /usr/include/bits/byteswap-16.h /usr/include/sys/select.h
diskio-unix-orig.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
diskio-unix-orig.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
diskio-unix-orig.o: /usr/include/bits/pthreadtypes.h support.h
diskio-unix-orig.o: /usr/include/stdlib.h /usr/include/bits/waitflags.h
diskio-unix-orig.o: /usr/include/bits/waitstatus.h /usr/include/alloca.h
diskio-unix-orig.o: /usr/include/bits/stdlib-float.h
diskio-windows.o: /usr/include/stdio.h /usr/include/features.h
diskio-windows.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
diskio-windows.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
diskio-windows.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
diskio-windows.o: /usr/include/libio.h /usr/include/_G_config.h
diskio-windows.o: /usr/include/wchar.h /usr/include/bits/wchar.h
diskio-windows.o: /usr/include/xlocale.h /usr/include/bits/stdio_lim.h
diskio-windows.o: /usr/include/bits/sys_errlist.h /usr/include/stdint.h
diskio-windows.o: /usr/include/errno.h /usr/include/bits/errno.h
diskio-windows.o: /usr/include/linux/errno.h /usr/include/fcntl.h
diskio-windows.o: /usr/include/bits/fcntl.h /usr/include/bits/fcntl-linux.h
diskio-windows.o: /usr/include/time.h /usr/include/bits/stat.h
diskio-windows.o: /usr/include/sys/stat.h support.h /usr/include/stdlib.h
diskio-windows.o: /usr/include/bits/waitflags.h
diskio-windows.o: /usr/include/bits/waitstatus.h /usr/include/endian.h
diskio-windows.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
diskio-windows.o: /usr/include/bits/byteswap-16.h /usr/include/sys/types.h
diskio-windows.o: /usr/include/sys/select.h /usr/include/bits/select.h
diskio-windows.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
diskio-windows.o: /usr/include/sys/sysmacros.h
diskio-windows.o: /usr/include/bits/pthreadtypes.h /usr/include/alloca.h
diskio-windows.o: /usr/include/bits/stdlib-float.h diskio.h
diskio-windows.o: /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h
diskio-windows.o: /usr/include/bits/ioctl-types.h
diskio-windows.o: /usr/include/sys/ttydefaults.h
fixparts.o: /usr/include/stdio.h /usr/include/features.h
fixparts.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
fixparts.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
fixparts.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
fixparts.o: /usr/include/libio.h /usr/include/_G_config.h
fixparts.o: /usr/include/wchar.h /usr/include/bits/wchar.h
fixparts.o: /usr/include/xlocale.h /usr/include/bits/stdio_lim.h
fixparts.o: /usr/include/bits/sys_errlist.h /usr/include/string.h basicmbr.h
fixparts.o: /usr/include/stdint.h /usr/include/sys/types.h
fixparts.o: /usr/include/time.h /usr/include/endian.h
fixparts.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
fixparts.o: /usr/include/bits/byteswap-16.h /usr/include/sys/select.h
fixparts.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
fixparts.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
fixparts.o: /usr/include/bits/pthreadtypes.h diskio.h
fixparts.o: /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h
fixparts.o: /usr/include/bits/ioctl-types.h /usr/include/sys/ttydefaults.h
fixparts.o: support.h /usr/include/stdlib.h /usr/include/bits/waitflags.h
fixparts.o: /usr/include/bits/waitstatus.h /usr/include/alloca.h
fixparts.o: /usr/include/bits/stdlib-float.h mbrpart.h
gdisk.o: /usr/include/string.h /usr/include/features.h
gdisk.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
gdisk.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
gdisk.o: /usr/include/xlocale.h gpttext.h gpt.h /usr/include/stdint.h
gdisk.o: /usr/include/bits/wchar.h /usr/include/sys/types.h
gdisk.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
gdisk.o: /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h
gdisk.o: /usr/include/bits/byteswap.h /usr/include/bits/byteswap-16.h
gdisk.o: /usr/include/sys/select.h /usr/include/bits/select.h
gdisk.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
gdisk.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
gdisk.o: gptpart.h support.h /usr/include/stdlib.h
gdisk.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
gdisk.o: /usr/include/alloca.h /usr/include/bits/stdlib-float.h parttypes.h
gdisk.o: guid.h /usr/include/uuid/uuid.h /usr/include/sys/time.h attributes.h
gdisk.o: mbr.h diskio.h /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h
gdisk.o: /usr/include/bits/ioctl-types.h /usr/include/sys/ttydefaults.h
gdisk.o: basicmbr.h mbrpart.h bsd.h
gpt.o: /usr/include/stdio.h /usr/include/features.h
gpt.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
gpt.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
gpt.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
gpt.o: /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h
gpt.o: /usr/include/bits/wchar.h /usr/include/xlocale.h
gpt.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
gpt.o: /usr/include/stdlib.h /usr/include/bits/waitflags.h
gpt.o: /usr/include/bits/waitstatus.h /usr/include/endian.h
gpt.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
gpt.o: /usr/include/bits/byteswap-16.h /usr/include/sys/types.h
gpt.o: /usr/include/time.h /usr/include/sys/select.h
gpt.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
gpt.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
gpt.o: /usr/include/bits/pthreadtypes.h /usr/include/alloca.h
gpt.o: /usr/include/bits/stdlib-float.h /usr/include/stdint.h
gpt.o: /usr/include/fcntl.h /usr/include/bits/fcntl.h
gpt.o: /usr/include/bits/fcntl-linux.h /usr/include/bits/stat.h
gpt.o: /usr/include/string.h /usr/include/math.h /usr/include/bits/huge_val.h
gpt.o: /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h
gpt.o: /usr/include/bits/inf.h /usr/include/bits/nan.h
gpt.o: /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h
gpt.o: /usr/include/sys/stat.h /usr/include/errno.h /usr/include/bits/errno.h
gpt.o: /usr/include/linux/errno.h crc32.h gpt.h gptpart.h support.h
gpt.o: parttypes.h guid.h /usr/include/uuid/uuid.h /usr/include/sys/time.h
gpt.o: attributes.h mbr.h diskio.h /usr/include/sys/ioctl.h
gpt.o: /usr/include/bits/ioctls.h /usr/include/bits/ioctl-types.h
gpt.o: /usr/include/sys/ttydefaults.h basicmbr.h mbrpart.h bsd.h
gptcl.o: /usr/include/string.h /usr/include/features.h
gptcl.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
gptcl.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
gptcl.o: /usr/include/xlocale.h /usr/include/errno.h
gptcl.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
gptcl.o: /usr/include/popt.h /usr/include/stdio.h /usr/include/bits/types.h
gptcl.o: /usr/include/bits/typesizes.h /usr/include/libio.h
gptcl.o: /usr/include/_G_config.h /usr/include/wchar.h
gptcl.o: /usr/include/bits/wchar.h /usr/include/bits/stdio_lim.h
gptcl.o: /usr/include/bits/sys_errlist.h gptcl.h gpt.h /usr/include/stdint.h
gptcl.o: /usr/include/sys/types.h /usr/include/time.h /usr/include/endian.h
gptcl.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
gptcl.o: /usr/include/bits/byteswap-16.h /usr/include/sys/select.h
gptcl.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
gptcl.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
gptcl.o: /usr/include/bits/pthreadtypes.h gptpart.h support.h
gptcl.o: /usr/include/stdlib.h /usr/include/bits/waitflags.h
gptcl.o: /usr/include/bits/waitstatus.h /usr/include/alloca.h
gptcl.o: /usr/include/bits/stdlib-float.h parttypes.h guid.h
gptcl.o: /usr/include/uuid/uuid.h /usr/include/sys/time.h attributes.h mbr.h
gptcl.o: diskio.h /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h
gptcl.o: /usr/include/bits/ioctl-types.h /usr/include/sys/ttydefaults.h
gptcl.o: basicmbr.h mbrpart.h bsd.h
gptcurses.o: /usr/include/ncurses.h /usr/include/ncurses_dll.h
gptcurses.o: /usr/include/stdio.h /usr/include/features.h
gptcurses.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
gptcurses.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
gptcurses.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
gptcurses.o: /usr/include/libio.h /usr/include/_G_config.h
gptcurses.o: /usr/include/wchar.h /usr/include/bits/wchar.h
gptcurses.o: /usr/include/xlocale.h /usr/include/bits/stdio_lim.h
gptcurses.o: /usr/include/bits/sys_errlist.h /usr/include/unctrl.h
gptcurses.o: /usr/include/curses.h gptcurses.h gptpart.h
gptcurses.o: /usr/include/stdint.h /usr/include/sys/types.h
gptcurses.o: /usr/include/time.h /usr/include/endian.h
gptcurses.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
gptcurses.o: /usr/include/bits/byteswap-16.h /usr/include/sys/select.h
gptcurses.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
gptcurses.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
gptcurses.o: /usr/include/bits/pthreadtypes.h support.h /usr/include/stdlib.h
gptcurses.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
gptcurses.o: /usr/include/alloca.h /usr/include/bits/stdlib-float.h
gptcurses.o: parttypes.h guid.h /usr/include/uuid/uuid.h
gptcurses.o: /usr/include/sys/time.h attributes.h gpt.h mbr.h diskio.h
gptcurses.o: /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h
gptcurses.o: /usr/include/bits/ioctl-types.h /usr/include/sys/ttydefaults.h
gptcurses.o: basicmbr.h mbrpart.h bsd.h
gptpart.o: /usr/include/string.h /usr/include/features.h
gptpart.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
gptpart.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
gptpart.o: /usr/include/xlocale.h /usr/include/stdio.h
gptpart.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
gptpart.o: /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h
gptpart.o: /usr/include/bits/wchar.h /usr/include/bits/stdio_lim.h
gptpart.o: /usr/include/bits/sys_errlist.h gptpart.h /usr/include/stdint.h
gptpart.o: /usr/include/sys/types.h /usr/include/time.h /usr/include/endian.h
gptpart.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
gptpart.o: /usr/include/bits/byteswap-16.h /usr/include/sys/select.h
gptpart.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
gptpart.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
gptpart.o: /usr/include/bits/pthreadtypes.h support.h /usr/include/stdlib.h
gptpart.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
gptpart.o: /usr/include/alloca.h /usr/include/bits/stdlib-float.h parttypes.h
gptpart.o: guid.h /usr/include/uuid/uuid.h /usr/include/sys/time.h
gptpart.o: attributes.h
gpttext.o: /usr/include/string.h /usr/include/features.h
gpttext.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
gpttext.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
gpttext.o: /usr/include/xlocale.h /usr/include/errno.h
gpttext.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
gpttext.o: /usr/include/stdint.h /usr/include/bits/wchar.h
gpttext.o: /usr/include/limits.h /usr/include/bits/posix1_lim.h
gpttext.o: /usr/include/bits/local_lim.h /usr/include/linux/limits.h
gpttext.o: /usr/include/bits/posix2_lim.h attributes.h gpttext.h gpt.h
gpttext.o: /usr/include/sys/types.h /usr/include/bits/types.h
gpttext.o: /usr/include/bits/typesizes.h /usr/include/time.h
gpttext.o: /usr/include/endian.h /usr/include/bits/endian.h
gpttext.o: /usr/include/bits/byteswap.h /usr/include/bits/byteswap-16.h
gpttext.o: /usr/include/sys/select.h /usr/include/bits/select.h
gpttext.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
gpttext.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
gpttext.o: gptpart.h support.h /usr/include/stdlib.h
gpttext.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
gpttext.o: /usr/include/alloca.h /usr/include/bits/stdlib-float.h parttypes.h
gpttext.o: guid.h /usr/include/uuid/uuid.h /usr/include/sys/time.h mbr.h
gpttext.o: diskio.h /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h
gpttext.o: /usr/include/bits/ioctl-types.h /usr/include/sys/ttydefaults.h
gpttext.o: basicmbr.h mbrpart.h bsd.h
guid.o: /usr/include/stdio.h /usr/include/features.h
guid.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
guid.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
guid.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
guid.o: /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h
guid.o: /usr/include/bits/wchar.h /usr/include/xlocale.h
guid.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
guid.o: /usr/include/time.h /usr/include/string.h guid.h
guid.o: /usr/include/stdint.h /usr/include/uuid/uuid.h
guid.o: /usr/include/sys/types.h /usr/include/endian.h
guid.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
guid.o: /usr/include/bits/byteswap-16.h /usr/include/sys/select.h
guid.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
guid.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
guid.o: /usr/include/bits/pthreadtypes.h /usr/include/sys/time.h support.h
guid.o: /usr/include/stdlib.h /usr/include/bits/waitflags.h
guid.o: /usr/include/bits/waitstatus.h /usr/include/alloca.h
guid.o: /usr/include/bits/stdlib-float.h
mbr.o: /usr/include/stdio.h /usr/include/features.h
mbr.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
mbr.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
mbr.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
mbr.o: /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h
mbr.o: /usr/include/bits/wchar.h /usr/include/xlocale.h
mbr.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
mbr.o: /usr/include/stdlib.h /usr/include/bits/waitflags.h
mbr.o: /usr/include/bits/waitstatus.h /usr/include/endian.h
mbr.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
mbr.o: /usr/include/bits/byteswap-16.h /usr/include/sys/types.h
mbr.o: /usr/include/time.h /usr/include/sys/select.h
mbr.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
mbr.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
mbr.o: /usr/include/bits/pthreadtypes.h /usr/include/alloca.h
mbr.o: /usr/include/bits/stdlib-float.h /usr/include/stdint.h
mbr.o: /usr/include/fcntl.h /usr/include/bits/fcntl.h
mbr.o: /usr/include/bits/fcntl-linux.h /usr/include/bits/stat.h
mbr.o: /usr/include/string.h /usr/include/sys/stat.h /usr/include/errno.h
mbr.o: /usr/include/bits/errno.h /usr/include/linux/errno.h mbr.h gptpart.h
mbr.o: support.h parttypes.h guid.h /usr/include/uuid/uuid.h
mbr.o: /usr/include/sys/time.h attributes.h diskio.h /usr/include/sys/ioctl.h
mbr.o: /usr/include/bits/ioctls.h /usr/include/bits/ioctl-types.h
mbr.o: /usr/include/sys/ttydefaults.h basicmbr.h mbrpart.h
mbrpart.o: /usr/include/stdint.h /usr/include/features.h
mbrpart.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
mbrpart.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
mbrpart.o: /usr/include/bits/wchar.h support.h /usr/include/stdlib.h
mbrpart.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
mbrpart.o: /usr/include/endian.h /usr/include/bits/endian.h
mbrpart.o: /usr/include/bits/byteswap.h /usr/include/bits/types.h
mbrpart.o: /usr/include/bits/typesizes.h /usr/include/bits/byteswap-16.h
mbrpart.o: /usr/include/sys/types.h /usr/include/time.h
mbrpart.o: /usr/include/sys/select.h /usr/include/bits/select.h
mbrpart.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
mbrpart.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
mbrpart.o: /usr/include/alloca.h /usr/include/bits/stdlib-float.h mbrpart.h
parttypes.o: /usr/include/string.h /usr/include/features.h
parttypes.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
parttypes.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
parttypes.o: /usr/include/xlocale.h /usr/include/stdint.h
parttypes.o: /usr/include/bits/wchar.h /usr/include/stdio.h
parttypes.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
parttypes.o: /usr/include/libio.h /usr/include/_G_config.h
parttypes.o: /usr/include/wchar.h /usr/include/bits/stdio_lim.h
parttypes.o: /usr/include/bits/sys_errlist.h parttypes.h
parttypes.o: /usr/include/stdlib.h /usr/include/bits/waitflags.h
parttypes.o: /usr/include/bits/waitstatus.h /usr/include/endian.h
parttypes.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
parttypes.o: /usr/include/bits/byteswap-16.h /usr/include/sys/types.h
parttypes.o: /usr/include/time.h /usr/include/sys/select.h
parttypes.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
parttypes.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
parttypes.o: /usr/include/bits/pthreadtypes.h /usr/include/alloca.h
parttypes.o: /usr/include/bits/stdlib-float.h support.h guid.h
parttypes.o: /usr/include/uuid/uuid.h /usr/include/sys/time.h
sgdisk.o: gptcl.h gpt.h /usr/include/stdint.h /usr/include/features.h
sgdisk.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
sgdisk.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
sgdisk.o: /usr/include/bits/wchar.h /usr/include/sys/types.h
sgdisk.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
sgdisk.o: /usr/include/time.h /usr/include/endian.h
sgdisk.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
sgdisk.o: /usr/include/bits/byteswap-16.h /usr/include/sys/select.h
sgdisk.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
sgdisk.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
sgdisk.o: /usr/include/bits/pthreadtypes.h gptpart.h support.h
sgdisk.o: /usr/include/stdlib.h /usr/include/bits/waitflags.h
sgdisk.o: /usr/include/bits/waitstatus.h /usr/include/alloca.h
sgdisk.o: /usr/include/bits/stdlib-float.h parttypes.h guid.h
sgdisk.o: /usr/include/uuid/uuid.h /usr/include/sys/time.h attributes.h mbr.h
sgdisk.o: diskio.h /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h
sgdisk.o: /usr/include/bits/ioctl-types.h /usr/include/sys/ttydefaults.h
sgdisk.o: basicmbr.h mbrpart.h bsd.h /usr/include/popt.h /usr/include/stdio.h
sgdisk.o: /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h
sgdisk.o: /usr/include/xlocale.h /usr/include/bits/stdio_lim.h
sgdisk.o: /usr/include/bits/sys_errlist.h
support.o: /usr/include/stdio.h /usr/include/features.h
support.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
support.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
support.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
support.o: /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h
support.o: /usr/include/bits/wchar.h /usr/include/xlocale.h
support.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
support.o: /usr/include/stdint.h /usr/include/errno.h
support.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
support.o: /usr/include/fcntl.h /usr/include/bits/fcntl.h
support.o: /usr/include/bits/fcntl-linux.h /usr/include/time.h
support.o: /usr/include/bits/stat.h /usr/include/string.h
support.o: /usr/include/sys/stat.h support.h /usr/include/stdlib.h
support.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
support.o: /usr/include/endian.h /usr/include/bits/endian.h
support.o: /usr/include/bits/byteswap.h /usr/include/bits/byteswap-16.h
support.o: /usr/include/sys/types.h /usr/include/sys/select.h
support.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
support.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
support.o: /usr/include/bits/pthreadtypes.h /usr/include/alloca.h
support.o: /usr/include/bits/stdlib-float.h
test.o: /usr/include/stdio.h /usr/include/features.h
test.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
test.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
test.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
test.o: /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h
test.o: /usr/include/bits/wchar.h /usr/include/xlocale.h
test.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
test.o: support.h /usr/include/stdint.h /usr/include/stdlib.h
test.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
test.o: /usr/include/endian.h /usr/include/bits/endian.h
test.o: /usr/include/bits/byteswap.h /usr/include/bits/byteswap-16.h
test.o: /usr/include/sys/types.h /usr/include/time.h
test.o: /usr/include/sys/select.h /usr/include/bits/select.h
test.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
test.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
test.o: /usr/include/alloca.h /usr/include/bits/stdlib-float.h
testguid.o: guid.h /usr/include/stdint.h /usr/include/features.h
testguid.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
testguid.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
testguid.o: /usr/include/bits/wchar.h /usr/include/uuid/uuid.h
testguid.o: /usr/include/sys/types.h /usr/include/bits/types.h
testguid.o: /usr/include/bits/typesizes.h /usr/include/time.h
testguid.o: /usr/include/endian.h /usr/include/bits/endian.h
testguid.o: /usr/include/bits/byteswap.h /usr/include/bits/byteswap-16.h
testguid.o: /usr/include/sys/select.h /usr/include/bits/select.h
testguid.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
testguid.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
testguid.o: /usr/include/sys/time.h parttypes.h /usr/include/stdlib.h
testguid.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
testguid.o: /usr/include/alloca.h /usr/include/bits/stdlib-float.h support.h
