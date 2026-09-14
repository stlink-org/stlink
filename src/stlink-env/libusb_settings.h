/**
  ******************************************************************************
  * @file           : libusb_settings.h
  * @brief          : Settings for libusb library
  * @copyright      : Copyright (c) 2026 stlink-org. All rights reserved.
  * @date           : 2026-07-27
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */

#ifndef LIBUSB_SETTINGS_H
#define LIBUSB_SETTINGS_H

#include <libusb.h>


/*
 *  libusb ver | LIBUSB_API_VERSION
 *  -----------+--------------------
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
 *  v1.0.26    | 0x01000110
 *  v1.0.27    | 0x01000111
 *  v1.0.28    | 0x01000112
 *  v1.0.29    | 0x01000113
 *  v1.0.30    | 0x01000114
 *  v1.0.31    | 0x01000115
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
#else // OpenBSD, Linux, WIN32, macOS
    #define MINIMAL_API_VERSION 0x01000108 // v1.0.24
#endif

#if(LIBUSB_API_VERSION < MINIMAL_API_VERSION)
    #error unsupported libusb version
#endif

#endif // LIBUSB_SETTINGS_H
