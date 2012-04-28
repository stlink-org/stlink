# Makefile.am -- Process this file with automake to produce Makefile.in
bin_PROGRAMS = st-flash st-util

noinst_LTLIBRARIES      = libstlink.la

st_flash_SOURCES = flash/main.c
st_util_SOURCES = gdbserver/gdb-remote.c gdbserver/gdb-remote.h gdbserver/gdb-server.c

libstlink_la_SOURCES	=	src/stlink-common.c src/stlink-usb.c src/stlink-sg.c src/uglylogging.c \
				src/stlink-common.h src/stlink-usb.h src/stlink-sg.h src/uglylogging.h

libstlink_la_CPPFLAGS	= -std=gnu99 -Wall -Wextra -O2

st_flash_LDADD	=	libstlink.la
st_flash_CPPFLAGS	= -std=gnu99 -Wall -Wextra -O2 -I$(top_srcdir)/src

st_util_LDADD	=	libstlink.la
st_util_CPPFLAGS	= -std=gnu99 -Wall -Wextra -O2 -I$(top_srcdir)/src

EXTRA_DIST = autogen.sh
