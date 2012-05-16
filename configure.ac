#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([stlink],[0.5.3],[davem@devkitpro.org])
AC_CONFIG_SRCDIR([src/stlink-common.c])
AC_CONFIG_LIBOBJ_DIR([src])
AM_INIT_AUTOMAKE([1.10])


# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_CANONICAL_HOST
AC_CANONICAL_BUILD
AC_PROG_RANLIB
AM_PROG_CC_C_O

AC_CHECK_HEADERS(sys/mman.h)
AC_CHECK_HEADERS(sys/poll.h)
AC_REPLACE_FUNCS(mmap)

# Checks for libraries.
PKG_CHECK_MODULES(USB, libusb-1.0 >= 1.0.0,,
                 AC_MSG_ERROR([*** Required libusb-1.0 >= 1.0.0 not installed ***]))
AC_CHECK_LIB([usbpath],[usb_path2devnum],,,-lusb)

LIBS="$LIBS $USB_LIBS"
CFLAGS="$CFLAGS $USB_CFLAGS"

case "${host}" in
	*-mingw32*)
		LIBS="$LIBS -lws2_32"
		CPPFLAGS="-D__USE_MINGW_ANSI_STDIO=1 $CPPFLAGS"
	;;
esac
AC_CONFIG_FILES([Makefile])
AC_OUTPUT

