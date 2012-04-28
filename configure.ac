#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([stlink],[0.5.0],[davem@devkitpro.org])
AC_CONFIG_SRCDIR([src/stlink-common.c])

AM_INIT_AUTOMAKE([1.10])


# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_CANONICAL_HOST
AC_CANONICAL_BUILD
AC_PROG_LIBTOOL
AM_PROG_CC_C_O

# Checks for libraries.
PKG_CHECK_MODULES(USB, libusb-1.0 >= 1.0.0,,
                 AC_MSG_ERROR([*** Required libusb-1.0 >= 1.0.0 not installed ***]))
AC_CHECK_LIB([usbpath],[usb_path2devnum],,,-lusb)

LIBS="$LIBS $USB_LIBS"
CFLAGS="$CFLAGS $USB_CFLAGS"

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

