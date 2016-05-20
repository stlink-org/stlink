# Makefile.am -- Process this file with automake to produce Makefile.in

SUBDIRS = . $(STLINK_HAS_GUI)

AUTOMAKE_OPTIONS = subdir-objects

if MINGW
bin_PROGRAMS = st-flash st-util st-info
else
bin_PROGRAMS = st-flash st-util st-term st-info
endif

noinst_LIBRARIES      = libstlink.a

st_flash_SOURCES = src/tools/flash.c
st_term_SOURCES = src/tools/term.c
st_info_SOURCES = src/tools/info.c
st_util_SOURCES = src/gdbserver/gdb-remote.c src/gdbserver/gdb-remote.h src/gdbserver/gdb-server.c src/mingw/mingw.c src/mingw/mingw.h

CFILES = \
	src/chipid.c \
	src/common.c \
	src/usb.c \
	src/sg.c \
	src/logging.c \
	src/flash_loader.c

if !MINGW
CFILES += src/tools/term.c
endif

HFILES	= \
	include/stlink.h \
	include/stlink/chipid.h \
	include/stlink/usb.h \
	include/stlink/flash_loader.h \
	include/stlink/commands.h \
	include/stlink/sg.h \
	include/stlink/logging.h \
	include/stlink/mmap.h

libstlink_a_SOURCES	=	$(CFILES) $(HFILES)

libstlink_a_LIBADD = $(LIBOBJS)
libstlink_a_CPPFLAGS	= -std=gnu99 -Wall -Wextra -O2 -I$(top_srcdir)/include

st_flash_LDADD	=	libstlink.a
st_flash_CPPFLAGS	= -std=gnu99 -Wall -Wextra -O2 -I$(top_srcdir)/include -I$(top_srcdir)/src/mingw

st_util_LDADD	=	libstlink.a
st_util_CPPFLAGS	= -std=gnu99 -Wall -Wextra -O2 -I$(top_srcdir)/include -I$(top_srcdir)/src/mingw

st_term_LDADD   =       libstlink.a
st_term_CPPFLAGS        = -std=gnu99 -Wall -Wextra -O2 -I$(top_srcdir)/include -I$(top_srcdir)/src/mingw

st_info_LDADD   =       libstlink.a
st_info_CPPFLAGS        = -std=gnu99 -Wall -Wextra -O2 -I$(top_srcdir)/include -I$(top_srcdir)/src/mingw

EXTRA_DIST = autogen.sh
