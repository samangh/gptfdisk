CC=gcc
CXX=g++
CFLAGS+=-D_FILE_OFFSET_BITS=64
CXXFLAGS+=-Wall -D_FILE_OFFSET_BITS=64
LDFLAGS+=
LIB_NAMES=crc32 support guid gptpart mbrpart basicmbr mbr gpt bsd parttypes attributes diskio diskio-unix
MBR_LIBS=support diskio diskio-unix basicmbr mbrpart
#LIB_SRCS=$(NAMES:=.cc)
LIB_OBJS=$(LIB_NAMES:=.o)
MBR_LIB_OBJS=$(MBR_LIBS:=.o)
LIB_HEADERS=$(LIB_NAMES:=.h)
DEPEND= makedepend $(CXXFLAGS)

all:	gdisk sgdisk fixparts

gdisk:	$(LIB_OBJS) gdisk.o gpttext.o
	$(CXX) $(LIB_OBJS) gdisk.o gpttext.o $(LDFLAGS) -licuio -licuuc -luuid -o gdisk

sgdisk: $(LIB_OBJS) sgdisk.o
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

attributes.o: /usr/include/stdint.h /usr/include/features.h
attributes.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
attributes.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-64.h
attributes.o: /usr/include/bits/wchar.h /usr/include/stdio.h
attributes.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
attributes.o: /usr/include/libio.h /usr/include/_G_config.h
attributes.o: /usr/include/wchar.h /usr/include/bits/stdio_lim.h
attributes.o: /usr/include/bits/sys_errlist.h attributes.h support.h
attributes.o: /usr/include/stdlib.h /usr/include/bits/waitflags.h
attributes.o: /usr/include/bits/waitstatus.h /usr/include/endian.h
attributes.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
attributes.o: /usr/include/sys/types.h /usr/include/time.h
attributes.o: /usr/include/sys/select.h /usr/include/bits/select.h
attributes.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
attributes.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
attributes.o: /usr/include/alloca.h
basicmbr.o: /usr/include/stdio.h /usr/include/features.h
basicmbr.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
basicmbr.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-64.h
basicmbr.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
basicmbr.o: /usr/include/libio.h /usr/include/_G_config.h
basicmbr.o: /usr/include/wchar.h /usr/include/bits/stdio_lim.h
basicmbr.o: /usr/include/bits/sys_errlist.h /usr/include/stdlib.h
basicmbr.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
basicmbr.o: /usr/include/endian.h /usr/include/bits/endian.h
basicmbr.o: /usr/include/bits/byteswap.h /usr/include/sys/types.h
basicmbr.o: /usr/include/time.h /usr/include/sys/select.h
basicmbr.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
basicmbr.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
basicmbr.o: /usr/include/bits/pthreadtypes.h /usr/include/alloca.h
basicmbr.o: /usr/include/stdint.h /usr/include/bits/wchar.h
basicmbr.o: /usr/include/fcntl.h /usr/include/bits/fcntl.h
basicmbr.o: /usr/include/bits/stat.h /usr/include/string.h
basicmbr.o: /usr/include/xlocale.h /usr/include/sys/stat.h
basicmbr.o: /usr/include/errno.h /usr/include/bits/errno.h
basicmbr.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
basicmbr.o: /usr/include/asm-generic/errno.h
basicmbr.o: /usr/include/asm-generic/errno-base.h mbr.h gptpart.h support.h
basicmbr.o: parttypes.h /usr/include/unicode/ustream.h
basicmbr.o: /usr/include/unicode/unistr.h /usr/include/unicode/utypes.h
basicmbr.o: /usr/include/unicode/umachine.h /usr/include/unicode/ptypes.h
basicmbr.o: /usr/include/unicode/platform.h /usr/include/inttypes.h
basicmbr.o: /usr/include/unicode/urename.h /usr/include/unicode/uvernum.h
basicmbr.o: /usr/include/unicode/utf.h /usr/include/unicode/utf8.h
basicmbr.o: /usr/include/unicode/utf16.h /usr/include/unicode/utf_old.h
basicmbr.o: /usr/include/unicode/uversion.h /usr/include/unicode/uconfig.h
basicmbr.o: /usr/include/unicode/rep.h /usr/include/unicode/uobject.h
basicmbr.o: /usr/include/unicode/std_string.h
basicmbr.o: /usr/include/unicode/stringpiece.h
basicmbr.o: /usr/include/unicode/bytestream.h guid.h /usr/include/uuid/uuid.h
basicmbr.o: /usr/include/sys/time.h attributes.h diskio.h
basicmbr.o: /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h
basicmbr.o: /usr/include/asm/ioctls.h /usr/include/asm-generic/ioctls.h
basicmbr.o: /usr/include/linux/ioctl.h /usr/include/asm/ioctl.h
basicmbr.o: /usr/include/asm-generic/ioctl.h /usr/include/bits/ioctl-types.h
basicmbr.o: /usr/include/sys/ttydefaults.h basicmbr.h mbrpart.h
bsd.o: /usr/include/stdio.h /usr/include/features.h /usr/include/sys/cdefs.h
bsd.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
bsd.o: /usr/include/gnu/stubs-64.h /usr/include/bits/types.h
bsd.o: /usr/include/bits/typesizes.h /usr/include/libio.h
bsd.o: /usr/include/_G_config.h /usr/include/wchar.h
bsd.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
bsd.o: /usr/include/stdlib.h /usr/include/bits/waitflags.h
bsd.o: /usr/include/bits/waitstatus.h /usr/include/endian.h
bsd.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
bsd.o: /usr/include/sys/types.h /usr/include/time.h /usr/include/sys/select.h
bsd.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
bsd.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
bsd.o: /usr/include/bits/pthreadtypes.h /usr/include/alloca.h
bsd.o: /usr/include/stdint.h /usr/include/bits/wchar.h /usr/include/fcntl.h
bsd.o: /usr/include/bits/fcntl.h /usr/include/bits/stat.h
bsd.o: /usr/include/sys/stat.h /usr/include/errno.h /usr/include/bits/errno.h
bsd.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
bsd.o: /usr/include/asm-generic/errno.h /usr/include/asm-generic/errno-base.h
bsd.o: support.h bsd.h gptpart.h parttypes.h /usr/include/unicode/ustream.h
bsd.o: /usr/include/unicode/unistr.h /usr/include/unicode/utypes.h
bsd.o: /usr/include/unicode/umachine.h /usr/include/unicode/ptypes.h
bsd.o: /usr/include/unicode/platform.h /usr/include/inttypes.h
bsd.o: /usr/include/unicode/urename.h /usr/include/unicode/uvernum.h
bsd.o: /usr/include/unicode/utf.h /usr/include/unicode/utf8.h
bsd.o: /usr/include/unicode/utf16.h /usr/include/unicode/utf_old.h
bsd.o: /usr/include/unicode/uversion.h /usr/include/unicode/uconfig.h
bsd.o: /usr/include/unicode/rep.h /usr/include/unicode/uobject.h
bsd.o: /usr/include/unicode/std_string.h /usr/include/unicode/stringpiece.h
bsd.o: /usr/include/unicode/bytestream.h guid.h /usr/include/uuid/uuid.h
bsd.o: /usr/include/sys/time.h attributes.h diskio.h /usr/include/sys/ioctl.h
bsd.o: /usr/include/bits/ioctls.h /usr/include/asm/ioctls.h
bsd.o: /usr/include/asm-generic/ioctls.h /usr/include/linux/ioctl.h
bsd.o: /usr/include/asm/ioctl.h /usr/include/asm-generic/ioctl.h
bsd.o: /usr/include/bits/ioctl-types.h /usr/include/sys/ttydefaults.h
crc32.o: /usr/include/stdio.h /usr/include/features.h
crc32.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
crc32.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-64.h
crc32.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
crc32.o: /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h
crc32.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
crc32.o: /usr/include/stdlib.h /usr/include/bits/waitflags.h
crc32.o: /usr/include/bits/waitstatus.h /usr/include/endian.h
crc32.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
crc32.o: /usr/include/sys/types.h /usr/include/time.h
crc32.o: /usr/include/sys/select.h /usr/include/bits/select.h
crc32.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
crc32.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
crc32.o: /usr/include/alloca.h crc32.h /usr/include/stdint.h
crc32.o: /usr/include/bits/wchar.h
diskio.o: /usr/include/sys/ioctl.h /usr/include/features.h
diskio.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
diskio.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-64.h
diskio.o: /usr/include/bits/ioctls.h /usr/include/asm/ioctls.h
diskio.o: /usr/include/asm-generic/ioctls.h /usr/include/linux/ioctl.h
diskio.o: /usr/include/asm/ioctl.h /usr/include/asm-generic/ioctl.h
diskio.o: /usr/include/bits/ioctl-types.h /usr/include/sys/ttydefaults.h
diskio.o: /usr/include/stdint.h /usr/include/bits/wchar.h
diskio.o: /usr/include/errno.h /usr/include/bits/errno.h
diskio.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
diskio.o: /usr/include/asm-generic/errno.h
diskio.o: /usr/include/asm-generic/errno-base.h /usr/include/fcntl.h
diskio.o: /usr/include/bits/fcntl.h /usr/include/sys/types.h
diskio.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
diskio.o: /usr/include/time.h /usr/include/endian.h
diskio.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
diskio.o: /usr/include/sys/select.h /usr/include/bits/select.h
diskio.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
diskio.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
diskio.o: /usr/include/bits/stat.h /usr/include/sys/stat.h support.h
diskio.o: /usr/include/stdlib.h /usr/include/bits/waitflags.h
diskio.o: /usr/include/bits/waitstatus.h /usr/include/alloca.h diskio.h
diskio-unix.o: /usr/include/sys/ioctl.h /usr/include/features.h
diskio-unix.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
diskio-unix.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-64.h
diskio-unix.o: /usr/include/bits/ioctls.h /usr/include/asm/ioctls.h
diskio-unix.o: /usr/include/asm-generic/ioctls.h /usr/include/linux/ioctl.h
diskio-unix.o: /usr/include/asm/ioctl.h /usr/include/asm-generic/ioctl.h
diskio-unix.o: /usr/include/bits/ioctl-types.h /usr/include/sys/ttydefaults.h
diskio-unix.o: /usr/include/string.h /usr/include/xlocale.h
diskio-unix.o: /usr/include/stdint.h /usr/include/bits/wchar.h
diskio-unix.o: /usr/include/errno.h /usr/include/bits/errno.h
diskio-unix.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
diskio-unix.o: /usr/include/asm-generic/errno.h
diskio-unix.o: /usr/include/asm-generic/errno-base.h /usr/include/fcntl.h
diskio-unix.o: /usr/include/bits/fcntl.h /usr/include/sys/types.h
diskio-unix.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
diskio-unix.o: /usr/include/time.h /usr/include/endian.h
diskio-unix.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
diskio-unix.o: /usr/include/sys/select.h /usr/include/bits/select.h
diskio-unix.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
diskio-unix.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
diskio-unix.o: /usr/include/bits/stat.h /usr/include/sys/stat.h diskio.h
diskio-unix.o: support.h /usr/include/stdlib.h /usr/include/bits/waitflags.h
diskio-unix.o: /usr/include/bits/waitstatus.h /usr/include/alloca.h
diskio-windows.o: /usr/include/stdio.h /usr/include/features.h
diskio-windows.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
diskio-windows.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-64.h
diskio-windows.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
diskio-windows.o: /usr/include/libio.h /usr/include/_G_config.h
diskio-windows.o: /usr/include/wchar.h /usr/include/bits/stdio_lim.h
diskio-windows.o: /usr/include/bits/sys_errlist.h /usr/include/stdint.h
diskio-windows.o: /usr/include/bits/wchar.h /usr/include/errno.h
diskio-windows.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
diskio-windows.o: /usr/include/asm/errno.h /usr/include/asm-generic/errno.h
diskio-windows.o: /usr/include/asm-generic/errno-base.h /usr/include/fcntl.h
diskio-windows.o: /usr/include/bits/fcntl.h /usr/include/sys/types.h
diskio-windows.o: /usr/include/time.h /usr/include/endian.h
diskio-windows.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
diskio-windows.o: /usr/include/sys/select.h /usr/include/bits/select.h
diskio-windows.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
diskio-windows.o: /usr/include/sys/sysmacros.h
diskio-windows.o: /usr/include/bits/pthreadtypes.h /usr/include/bits/stat.h
diskio-windows.o: /usr/include/sys/stat.h support.h /usr/include/stdlib.h
diskio-windows.o: /usr/include/bits/waitflags.h
diskio-windows.o: /usr/include/bits/waitstatus.h /usr/include/alloca.h
diskio-windows.o: diskio.h /usr/include/sys/ioctl.h
diskio-windows.o: /usr/include/bits/ioctls.h /usr/include/asm/ioctls.h
diskio-windows.o: /usr/include/asm-generic/ioctls.h
diskio-windows.o: /usr/include/linux/ioctl.h /usr/include/asm/ioctl.h
diskio-windows.o: /usr/include/asm-generic/ioctl.h
diskio-windows.o: /usr/include/bits/ioctl-types.h
diskio-windows.o: /usr/include/sys/ttydefaults.h
fixparts.o: /usr/include/stdio.h /usr/include/features.h
fixparts.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
fixparts.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-64.h
fixparts.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
fixparts.o: /usr/include/libio.h /usr/include/_G_config.h
fixparts.o: /usr/include/wchar.h /usr/include/bits/stdio_lim.h
fixparts.o: /usr/include/bits/sys_errlist.h /usr/include/string.h
fixparts.o: /usr/include/xlocale.h basicmbr.h /usr/include/stdint.h
fixparts.o: /usr/include/bits/wchar.h /usr/include/sys/types.h
fixparts.o: /usr/include/time.h /usr/include/endian.h
fixparts.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
fixparts.o: /usr/include/sys/select.h /usr/include/bits/select.h
fixparts.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
fixparts.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
fixparts.o: diskio.h /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h
fixparts.o: /usr/include/asm/ioctls.h /usr/include/asm-generic/ioctls.h
fixparts.o: /usr/include/linux/ioctl.h /usr/include/asm/ioctl.h
fixparts.o: /usr/include/asm-generic/ioctl.h /usr/include/bits/ioctl-types.h
fixparts.o: /usr/include/sys/ttydefaults.h support.h /usr/include/stdlib.h
fixparts.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
fixparts.o: /usr/include/alloca.h mbrpart.h
gdisk.o: /usr/include/stdio.h /usr/include/features.h
gdisk.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
gdisk.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-64.h
gdisk.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
gdisk.o: /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h
gdisk.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
gdisk.o: /usr/include/string.h /usr/include/xlocale.h mbr.h
gdisk.o: /usr/include/stdint.h /usr/include/bits/wchar.h
gdisk.o: /usr/include/sys/types.h /usr/include/time.h /usr/include/endian.h
gdisk.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
gdisk.o: /usr/include/sys/select.h /usr/include/bits/select.h
gdisk.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
gdisk.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
gdisk.o: gptpart.h support.h /usr/include/stdlib.h
gdisk.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
gdisk.o: /usr/include/alloca.h parttypes.h /usr/include/unicode/ustream.h
gdisk.o: /usr/include/unicode/unistr.h /usr/include/unicode/utypes.h
gdisk.o: /usr/include/unicode/umachine.h /usr/include/unicode/ptypes.h
gdisk.o: /usr/include/unicode/platform.h /usr/include/inttypes.h
gdisk.o: /usr/include/unicode/urename.h /usr/include/unicode/uvernum.h
gdisk.o: /usr/include/unicode/utf.h /usr/include/unicode/utf8.h
gdisk.o: /usr/include/unicode/utf16.h /usr/include/unicode/utf_old.h
gdisk.o: /usr/include/unicode/uversion.h /usr/include/unicode/uconfig.h
gdisk.o: /usr/include/unicode/rep.h /usr/include/unicode/uobject.h
gdisk.o: /usr/include/unicode/std_string.h /usr/include/unicode/stringpiece.h
gdisk.o: /usr/include/unicode/bytestream.h guid.h /usr/include/uuid/uuid.h
gdisk.o: /usr/include/sys/time.h attributes.h diskio.h
gdisk.o: /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h
gdisk.o: /usr/include/asm/ioctls.h /usr/include/asm-generic/ioctls.h
gdisk.o: /usr/include/linux/ioctl.h /usr/include/asm/ioctl.h
gdisk.o: /usr/include/asm-generic/ioctl.h /usr/include/bits/ioctl-types.h
gdisk.o: /usr/include/sys/ttydefaults.h basicmbr.h mbrpart.h gpttext.h gpt.h
gdisk.o: bsd.h
gpt.o: /usr/include/stdio.h /usr/include/features.h /usr/include/sys/cdefs.h
gpt.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
gpt.o: /usr/include/gnu/stubs-64.h /usr/include/bits/types.h
gpt.o: /usr/include/bits/typesizes.h /usr/include/libio.h
gpt.o: /usr/include/_G_config.h /usr/include/wchar.h
gpt.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
gpt.o: /usr/include/stdlib.h /usr/include/bits/waitflags.h
gpt.o: /usr/include/bits/waitstatus.h /usr/include/endian.h
gpt.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
gpt.o: /usr/include/sys/types.h /usr/include/time.h /usr/include/sys/select.h
gpt.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
gpt.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
gpt.o: /usr/include/bits/pthreadtypes.h /usr/include/alloca.h
gpt.o: /usr/include/stdint.h /usr/include/bits/wchar.h /usr/include/fcntl.h
gpt.o: /usr/include/bits/fcntl.h /usr/include/bits/stat.h
gpt.o: /usr/include/string.h /usr/include/xlocale.h /usr/include/math.h
gpt.o: /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h
gpt.o: /usr/include/bits/huge_vall.h /usr/include/bits/inf.h
gpt.o: /usr/include/bits/nan.h /usr/include/bits/mathdef.h
gpt.o: /usr/include/bits/mathcalls.h /usr/include/sys/stat.h
gpt.o: /usr/include/errno.h /usr/include/bits/errno.h
gpt.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
gpt.o: /usr/include/asm-generic/errno.h /usr/include/asm-generic/errno-base.h
gpt.o: crc32.h gpt.h gptpart.h support.h parttypes.h
gpt.o: /usr/include/unicode/ustream.h /usr/include/unicode/unistr.h
gpt.o: /usr/include/unicode/utypes.h /usr/include/unicode/umachine.h
gpt.o: /usr/include/unicode/ptypes.h /usr/include/unicode/platform.h
gpt.o: /usr/include/inttypes.h /usr/include/unicode/urename.h
gpt.o: /usr/include/unicode/uvernum.h /usr/include/unicode/utf.h
gpt.o: /usr/include/unicode/utf8.h /usr/include/unicode/utf16.h
gpt.o: /usr/include/unicode/utf_old.h /usr/include/unicode/uversion.h
gpt.o: /usr/include/unicode/uconfig.h /usr/include/unicode/rep.h
gpt.o: /usr/include/unicode/uobject.h /usr/include/unicode/std_string.h
gpt.o: /usr/include/unicode/stringpiece.h /usr/include/unicode/bytestream.h
gpt.o: guid.h /usr/include/uuid/uuid.h /usr/include/sys/time.h attributes.h
gpt.o: mbr.h diskio.h /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h
gpt.o: /usr/include/asm/ioctls.h /usr/include/asm-generic/ioctls.h
gpt.o: /usr/include/linux/ioctl.h /usr/include/asm/ioctl.h
gpt.o: /usr/include/asm-generic/ioctl.h /usr/include/bits/ioctl-types.h
gpt.o: /usr/include/sys/ttydefaults.h basicmbr.h mbrpart.h bsd.h
gptpart.o: /usr/include/unicode/ustdio.h /usr/include/stdio.h
gptpart.o: /usr/include/features.h /usr/include/sys/cdefs.h
gptpart.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
gptpart.o: /usr/include/gnu/stubs-64.h /usr/include/bits/types.h
gptpart.o: /usr/include/bits/typesizes.h /usr/include/libio.h
gptpart.o: /usr/include/_G_config.h /usr/include/wchar.h
gptpart.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
gptpart.o: /usr/include/unicode/utypes.h /usr/include/unicode/umachine.h
gptpart.o: /usr/include/unicode/ptypes.h /usr/include/sys/types.h
gptpart.o: /usr/include/time.h /usr/include/endian.h
gptpart.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
gptpart.o: /usr/include/sys/select.h /usr/include/bits/select.h
gptpart.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
gptpart.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
gptpart.o: /usr/include/unicode/platform.h /usr/include/inttypes.h
gptpart.o: /usr/include/stdint.h /usr/include/bits/wchar.h
gptpart.o: /usr/include/unicode/urename.h /usr/include/unicode/uvernum.h
gptpart.o: /usr/include/unicode/utf.h /usr/include/unicode/utf8.h
gptpart.o: /usr/include/unicode/utf16.h /usr/include/unicode/utf_old.h
gptpart.o: /usr/include/unicode/uversion.h /usr/include/unicode/uconfig.h
gptpart.o: /usr/include/unicode/ucnv.h /usr/include/unicode/ucnv_err.h
gptpart.o: /usr/include/unicode/uenum.h /usr/include/unicode/localpointer.h
gptpart.o: /usr/include/unicode/utrans.h /usr/include/unicode/urep.h
gptpart.o: /usr/include/unicode/parseerr.h /usr/include/string.h
gptpart.o: /usr/include/xlocale.h gptpart.h support.h /usr/include/stdlib.h
gptpart.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
gptpart.o: /usr/include/alloca.h parttypes.h /usr/include/unicode/ustream.h
gptpart.o: /usr/include/unicode/unistr.h /usr/include/unicode/rep.h
gptpart.o: /usr/include/unicode/uobject.h /usr/include/unicode/std_string.h
gptpart.o: /usr/include/unicode/stringpiece.h
gptpart.o: /usr/include/unicode/bytestream.h guid.h /usr/include/uuid/uuid.h
gptpart.o: /usr/include/sys/time.h attributes.h
gpttext.o: /usr/include/string.h /usr/include/features.h
gpttext.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
gpttext.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-64.h
gpttext.o: /usr/include/xlocale.h /usr/include/errno.h
gpttext.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
gpttext.o: /usr/include/asm/errno.h /usr/include/asm-generic/errno.h
gpttext.o: /usr/include/asm-generic/errno-base.h /usr/include/stdint.h
gpttext.o: /usr/include/bits/wchar.h /usr/include/limits.h
gpttext.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
gpttext.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
gpttext.o: attributes.h gpttext.h gpt.h /usr/include/sys/types.h
gpttext.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
gpttext.o: /usr/include/time.h /usr/include/endian.h
gpttext.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
gpttext.o: /usr/include/sys/select.h /usr/include/bits/select.h
gpttext.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
gpttext.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
gpttext.o: gptpart.h support.h /usr/include/stdlib.h
gpttext.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
gpttext.o: /usr/include/alloca.h parttypes.h /usr/include/unicode/ustream.h
gpttext.o: /usr/include/unicode/unistr.h /usr/include/unicode/utypes.h
gpttext.o: /usr/include/unicode/umachine.h /usr/include/unicode/ptypes.h
gpttext.o: /usr/include/unicode/platform.h /usr/include/inttypes.h
gpttext.o: /usr/include/unicode/urename.h /usr/include/unicode/uvernum.h
gpttext.o: /usr/include/unicode/utf.h /usr/include/unicode/utf8.h
gpttext.o: /usr/include/unicode/utf16.h /usr/include/unicode/utf_old.h
gpttext.o: /usr/include/unicode/uversion.h /usr/include/unicode/uconfig.h
gpttext.o: /usr/include/unicode/rep.h /usr/include/unicode/uobject.h
gpttext.o: /usr/include/unicode/std_string.h
gpttext.o: /usr/include/unicode/stringpiece.h
gpttext.o: /usr/include/unicode/bytestream.h guid.h /usr/include/uuid/uuid.h
gpttext.o: /usr/include/sys/time.h mbr.h diskio.h /usr/include/sys/ioctl.h
gpttext.o: /usr/include/bits/ioctls.h /usr/include/asm/ioctls.h
gpttext.o: /usr/include/asm-generic/ioctls.h /usr/include/linux/ioctl.h
gpttext.o: /usr/include/asm/ioctl.h /usr/include/asm-generic/ioctl.h
gpttext.o: /usr/include/bits/ioctl-types.h /usr/include/sys/ttydefaults.h
gpttext.o: basicmbr.h mbrpart.h bsd.h
guid.o: /usr/include/stdio.h /usr/include/features.h /usr/include/sys/cdefs.h
guid.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
guid.o: /usr/include/gnu/stubs-64.h /usr/include/bits/types.h
guid.o: /usr/include/bits/typesizes.h /usr/include/libio.h
guid.o: /usr/include/_G_config.h /usr/include/wchar.h
guid.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
guid.o: /usr/include/time.h /usr/include/string.h /usr/include/xlocale.h
guid.o: guid.h /usr/include/stdint.h /usr/include/bits/wchar.h
guid.o: /usr/include/uuid/uuid.h /usr/include/sys/types.h
guid.o: /usr/include/endian.h /usr/include/bits/endian.h
guid.o: /usr/include/bits/byteswap.h /usr/include/sys/select.h
guid.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
guid.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
guid.o: /usr/include/bits/pthreadtypes.h /usr/include/sys/time.h support.h
guid.o: /usr/include/stdlib.h /usr/include/bits/waitflags.h
guid.o: /usr/include/bits/waitstatus.h /usr/include/alloca.h
mbr.o: /usr/include/stdio.h /usr/include/features.h /usr/include/sys/cdefs.h
mbr.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
mbr.o: /usr/include/gnu/stubs-64.h /usr/include/bits/types.h
mbr.o: /usr/include/bits/typesizes.h /usr/include/libio.h
mbr.o: /usr/include/_G_config.h /usr/include/wchar.h
mbr.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
mbr.o: /usr/include/stdlib.h /usr/include/bits/waitflags.h
mbr.o: /usr/include/bits/waitstatus.h /usr/include/endian.h
mbr.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
mbr.o: /usr/include/sys/types.h /usr/include/time.h /usr/include/sys/select.h
mbr.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
mbr.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
mbr.o: /usr/include/bits/pthreadtypes.h /usr/include/alloca.h
mbr.o: /usr/include/stdint.h /usr/include/bits/wchar.h /usr/include/fcntl.h
mbr.o: /usr/include/bits/fcntl.h /usr/include/bits/stat.h
mbr.o: /usr/include/string.h /usr/include/xlocale.h /usr/include/sys/stat.h
mbr.o: /usr/include/errno.h /usr/include/bits/errno.h
mbr.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
mbr.o: /usr/include/asm-generic/errno.h /usr/include/asm-generic/errno-base.h
mbr.o: mbr.h gptpart.h support.h parttypes.h /usr/include/unicode/ustream.h
mbr.o: /usr/include/unicode/unistr.h /usr/include/unicode/utypes.h
mbr.o: /usr/include/unicode/umachine.h /usr/include/unicode/ptypes.h
mbr.o: /usr/include/unicode/platform.h /usr/include/inttypes.h
mbr.o: /usr/include/unicode/urename.h /usr/include/unicode/uvernum.h
mbr.o: /usr/include/unicode/utf.h /usr/include/unicode/utf8.h
mbr.o: /usr/include/unicode/utf16.h /usr/include/unicode/utf_old.h
mbr.o: /usr/include/unicode/uversion.h /usr/include/unicode/uconfig.h
mbr.o: /usr/include/unicode/rep.h /usr/include/unicode/uobject.h
mbr.o: /usr/include/unicode/std_string.h /usr/include/unicode/stringpiece.h
mbr.o: /usr/include/unicode/bytestream.h guid.h /usr/include/uuid/uuid.h
mbr.o: /usr/include/sys/time.h attributes.h diskio.h /usr/include/sys/ioctl.h
mbr.o: /usr/include/bits/ioctls.h /usr/include/asm/ioctls.h
mbr.o: /usr/include/asm-generic/ioctls.h /usr/include/linux/ioctl.h
mbr.o: /usr/include/asm/ioctl.h /usr/include/asm-generic/ioctl.h
mbr.o: /usr/include/bits/ioctl-types.h /usr/include/sys/ttydefaults.h
mbr.o: basicmbr.h mbrpart.h
mbrpart.o: /usr/include/stdint.h /usr/include/features.h
mbrpart.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
mbrpart.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-64.h
mbrpart.o: /usr/include/bits/wchar.h support.h /usr/include/stdlib.h
mbrpart.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
mbrpart.o: /usr/include/endian.h /usr/include/bits/endian.h
mbrpart.o: /usr/include/bits/byteswap.h /usr/include/sys/types.h
mbrpart.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
mbrpart.o: /usr/include/time.h /usr/include/sys/select.h
mbrpart.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
mbrpart.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
mbrpart.o: /usr/include/bits/pthreadtypes.h /usr/include/alloca.h mbrpart.h
parttypes.o: /usr/include/string.h /usr/include/features.h
parttypes.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
parttypes.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-64.h
parttypes.o: /usr/include/xlocale.h /usr/include/stdint.h
parttypes.o: /usr/include/bits/wchar.h /usr/include/stdio.h
parttypes.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
parttypes.o: /usr/include/libio.h /usr/include/_G_config.h
parttypes.o: /usr/include/wchar.h /usr/include/bits/stdio_lim.h
parttypes.o: /usr/include/bits/sys_errlist.h parttypes.h
parttypes.o: /usr/include/stdlib.h /usr/include/bits/waitflags.h
parttypes.o: /usr/include/bits/waitstatus.h /usr/include/endian.h
parttypes.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
parttypes.o: /usr/include/sys/types.h /usr/include/time.h
parttypes.o: /usr/include/sys/select.h /usr/include/bits/select.h
parttypes.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
parttypes.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
parttypes.o: /usr/include/alloca.h /usr/include/unicode/ustream.h
parttypes.o: /usr/include/unicode/unistr.h /usr/include/unicode/utypes.h
parttypes.o: /usr/include/unicode/umachine.h /usr/include/unicode/ptypes.h
parttypes.o: /usr/include/unicode/platform.h /usr/include/inttypes.h
parttypes.o: /usr/include/unicode/urename.h /usr/include/unicode/uvernum.h
parttypes.o: /usr/include/unicode/utf.h /usr/include/unicode/utf8.h
parttypes.o: /usr/include/unicode/utf16.h /usr/include/unicode/utf_old.h
parttypes.o: /usr/include/unicode/uversion.h /usr/include/unicode/uconfig.h
parttypes.o: /usr/include/unicode/rep.h /usr/include/unicode/uobject.h
parttypes.o: /usr/include/unicode/std_string.h
parttypes.o: /usr/include/unicode/stringpiece.h
parttypes.o: /usr/include/unicode/bytestream.h support.h guid.h
parttypes.o: /usr/include/uuid/uuid.h /usr/include/sys/time.h
sgdisk.o: /usr/include/stdio.h /usr/include/features.h
sgdisk.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
sgdisk.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-64.h
sgdisk.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
sgdisk.o: /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h
sgdisk.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
sgdisk.o: /usr/include/popt.h /usr/include/errno.h /usr/include/bits/errno.h
sgdisk.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
sgdisk.o: /usr/include/asm-generic/errno.h
sgdisk.o: /usr/include/asm-generic/errno-base.h /usr/include/stdint.h
sgdisk.o: /usr/include/bits/wchar.h /usr/include/string.h
sgdisk.o: /usr/include/xlocale.h mbr.h /usr/include/sys/types.h
sgdisk.o: /usr/include/time.h /usr/include/endian.h
sgdisk.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
sgdisk.o: /usr/include/sys/select.h /usr/include/bits/select.h
sgdisk.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
sgdisk.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
sgdisk.o: gptpart.h support.h /usr/include/stdlib.h
sgdisk.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
sgdisk.o: /usr/include/alloca.h parttypes.h /usr/include/unicode/ustream.h
sgdisk.o: /usr/include/unicode/unistr.h /usr/include/unicode/utypes.h
sgdisk.o: /usr/include/unicode/umachine.h /usr/include/unicode/ptypes.h
sgdisk.o: /usr/include/unicode/platform.h /usr/include/inttypes.h
sgdisk.o: /usr/include/unicode/urename.h /usr/include/unicode/uvernum.h
sgdisk.o: /usr/include/unicode/utf.h /usr/include/unicode/utf8.h
sgdisk.o: /usr/include/unicode/utf16.h /usr/include/unicode/utf_old.h
sgdisk.o: /usr/include/unicode/uversion.h /usr/include/unicode/uconfig.h
sgdisk.o: /usr/include/unicode/rep.h /usr/include/unicode/uobject.h
sgdisk.o: /usr/include/unicode/std_string.h
sgdisk.o: /usr/include/unicode/stringpiece.h
sgdisk.o: /usr/include/unicode/bytestream.h guid.h /usr/include/uuid/uuid.h
sgdisk.o: /usr/include/sys/time.h attributes.h diskio.h
sgdisk.o: /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h
sgdisk.o: /usr/include/asm/ioctls.h /usr/include/asm-generic/ioctls.h
sgdisk.o: /usr/include/linux/ioctl.h /usr/include/asm/ioctl.h
sgdisk.o: /usr/include/asm-generic/ioctl.h /usr/include/bits/ioctl-types.h
sgdisk.o: /usr/include/sys/ttydefaults.h basicmbr.h mbrpart.h gpt.h bsd.h
support.o: /usr/include/stdio.h /usr/include/features.h
support.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
support.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-64.h
support.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
support.o: /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h
support.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
support.o: /usr/include/stdint.h /usr/include/bits/wchar.h
support.o: /usr/include/errno.h /usr/include/bits/errno.h
support.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
support.o: /usr/include/asm-generic/errno.h
support.o: /usr/include/asm-generic/errno-base.h /usr/include/fcntl.h
support.o: /usr/include/bits/fcntl.h /usr/include/sys/types.h
support.o: /usr/include/time.h /usr/include/endian.h
support.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
support.o: /usr/include/sys/select.h /usr/include/bits/select.h
support.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
support.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
support.o: /usr/include/bits/stat.h /usr/include/string.h
support.o: /usr/include/xlocale.h /usr/include/sys/stat.h support.h
support.o: /usr/include/stdlib.h /usr/include/bits/waitflags.h
support.o: /usr/include/bits/waitstatus.h /usr/include/alloca.h
test.o: /usr/include/stdio.h /usr/include/features.h /usr/include/sys/cdefs.h
test.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
test.o: /usr/include/gnu/stubs-64.h /usr/include/bits/types.h
test.o: /usr/include/bits/typesizes.h /usr/include/libio.h
test.o: /usr/include/_G_config.h /usr/include/wchar.h
test.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
test.o: support.h /usr/include/stdint.h /usr/include/bits/wchar.h
test.o: /usr/include/stdlib.h /usr/include/bits/waitflags.h
test.o: /usr/include/bits/waitstatus.h /usr/include/endian.h
test.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
test.o: /usr/include/sys/types.h /usr/include/time.h
test.o: /usr/include/sys/select.h /usr/include/bits/select.h
test.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
test.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
test.o: /usr/include/alloca.h
testguid.o: guid.h /usr/include/stdint.h /usr/include/features.h
testguid.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
testguid.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-64.h
testguid.o: /usr/include/bits/wchar.h /usr/include/uuid/uuid.h
testguid.o: /usr/include/sys/types.h /usr/include/bits/types.h
testguid.o: /usr/include/bits/typesizes.h /usr/include/time.h
testguid.o: /usr/include/endian.h /usr/include/bits/endian.h
testguid.o: /usr/include/bits/byteswap.h /usr/include/sys/select.h
testguid.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
testguid.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
testguid.o: /usr/include/bits/pthreadtypes.h /usr/include/sys/time.h
testguid.o: parttypes.h /usr/include/stdlib.h /usr/include/bits/waitflags.h
testguid.o: /usr/include/bits/waitstatus.h /usr/include/alloca.h
testguid.o: /usr/include/unicode/ustream.h /usr/include/unicode/unistr.h
testguid.o: /usr/include/unicode/utypes.h /usr/include/unicode/umachine.h
testguid.o: /usr/include/unicode/ptypes.h /usr/include/unicode/platform.h
testguid.o: /usr/include/inttypes.h /usr/include/unicode/urename.h
testguid.o: /usr/include/unicode/uvernum.h /usr/include/unicode/utf.h
testguid.o: /usr/include/unicode/utf8.h /usr/include/unicode/utf16.h
testguid.o: /usr/include/unicode/utf_old.h /usr/include/unicode/uversion.h
testguid.o: /usr/include/unicode/uconfig.h /usr/include/unicode/rep.h
testguid.o: /usr/include/unicode/uobject.h /usr/include/unicode/std_string.h
testguid.o: /usr/include/unicode/stringpiece.h
testguid.o: /usr/include/unicode/bytestream.h support.h
