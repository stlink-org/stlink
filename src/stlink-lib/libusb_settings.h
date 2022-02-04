#ifndef LIBUSB_SETTINGS_H
#define LIBUSB_SETTINGS_H

#include <libusb.h>

/*
 *  libusb ver | LIBUSB_API_VERSION
 *  -----------+--------------------
 *  v1.0.13    | 0x01000100
 *  v1.0.14    | 0x010000FF
 *  v1.0.15    | 0x01000101
 *  v1.0.16    | 0x01000102
 *  v1.0.17    | 0x01000102
 *  v1.0.18    | 0x01000102
 *  v1.0.19    | 0x01000103
 *  v1.0.20    | 0x01000104
 *  v1.0.21    | 0x01000105
 *  v1.0.22    | 0x01000106
 *  v1.0.23    | 0x01000107
 *  v1.0.24    | 0x01000108
 *  v1.0.25    | 0x01000109
 */

#if defined (__FreeBSD__)
    #if !defined (LIBUSBX_API_VERSION)
        #define LIBUSBX_API_VERSION LIBUSB_API_VERSION
    #elif !defined (LIBUSB_API_VERSION)
        #error unsupported libusb version
    #endif
#endif

#if defined (__FreeBSD__)
    #define MINIMAL_API_VERSION 0x01000102 // v1.0.16
#elif defined (__OpenBSD__)
    #define MINIMAL_API_VERSION 0x01000105 // v1.0.21
#elif defined (__linux__)
    #define MINIMAL_API_VERSION 0x01000105 // v1.0.21
#elif defined (__APPLE__)
    #define MINIMAL_API_VERSION 0x01000109 // v1.0.25
#elif defined (_WIN32)
    #define MINIMAL_API_VERSION 0x01000109 // v1.0.25
#endif

#if (LIBUSB_API_VERSION < MINIMAL_API_VERSION)
    #error unsupported libusb version
#endif

#endif // LIBUSB_SETTINGS_H
