# FindLibUSB.cmake - Try to find the Hiredis library
# Once done this will define
#
#  LIBUSB_FOUND - System has libusb
#  LIBUSB_INCLUDE_DIR - The libusb include directory
#  LIBUSB_LIBRARIES - The libraries needed to use libusb
#  LIBUSB_DEFINITIONS - Compiler switches required for using libusb

FIND_PATH(LIBUSB_INCLUDE_DIR NAMES libusb.h
   HINTS
   /usr
   /usr/local
   /opt
   PATH_SUFFIXES libusb-1.0
   )

FIND_LIBRARY(LIBUSB_LIBRARIES NAMES usb-1.0
   HINTS
   /usr
   /usr/local
   /opt
   )

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Libusb DEFAULT_MSG LIBUSB_LIBRARIES LIBUSB_INCLUDE_DIR)

MARK_AS_ADVANCED(LIBUSB_INCLUDE_DIR LIBUSB_LIBRARIES)
