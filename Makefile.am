# Makefile.am -- Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = st-flash st-util

noinst_LIBRARIES      = libstlink.a

st_flash_SOURCES = flash/main.c
st_util_SOURCES = gdbserver/gdb-remote.c gdbserver/gdb-remote.h gdbserver/gdb-server.c mingw/mingw.c mingw/mingw.h

CFILES = \
	src/stlink-common.c \
	src/stlink-usb.c \
	src/stlink-sg.c \
	src/uglylogging.c

HFILES	= \
	src/stlink-common.h \
	src/stlink-usb.h \
	src/stlink-sg.h \
	src/uglylogging.h \
	src/mmap.h

libstlink_a_SOURCES	=	$(CFILES) $(HFILES)

libstlink_a_CPPFLAGS	= -std=gnu99 -Wall -Wextra -O2
libstlink_a_LIBADD = $(LIBOBJS)

st_flash_LDADD	=	libstlink.a
st_flash_CPPFLAGS	= -std=gnu99 -Wall -Wextra -O2 -I$(top_srcdir)/src -I$(top_srcdir)/mingw

st_util_LDADD	=	libstlink.a
st_util_CPPFLAGS	= -std=gnu99 -Wall -Wextra -O2 -I$(top_srcdir)/src -I$(top_srcdir)/mingw

EXTRA_DIST = autogen.sh

