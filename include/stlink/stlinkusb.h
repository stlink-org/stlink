#ifndef STLINKUSB_H
#define STLINKUSB_H

#include <libusb.h>

/*

 libusb ver  | LIBUSB_API_VERSION
-------------+--------------------
   v1.0.13   |    0x01000100
   v1.0.14   |    0x010000FF
   v1.0.15   |    0x01000101
   v1.0.16   |    0x01000102       < MINIMAL_API_VERSION
   v1.0.17   |    0x01000102
   v1.0.18   |    0x01000102
   v1.0.19   |    0x01000103
   v1.0.20   |    0x01000104
   v1.0.21   |    0x01000105
   v1.0.22   |    0x01000106
   v1.0.23   |    0x01000107

*/

#if !defined ( LIBUSBX_API_VERSION )
#if defined (__FreeBSD__)
#define LIBUSBX_API_VERSION LIBUSB_API_VERSION
#elif !defined (LIBUSB_API_VERSION)
#error unsupported libusb version
#endif
#endif

#define MINIMAL_API_VERSION 0x01000102

#if ( LIBUSB_API_VERSION < MINIMAL_API_VERSION )
#error unsupported libusb version
#endif

#endif // STLINKUSB_H
