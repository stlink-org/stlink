/*
 * File:   stlink/usb.h
 * Author: karl
 *
 * Created on October 1, 2011, 11:29 PM
 */

#ifndef STLINK_USB_H
#define STLINK_USB_H

#include <stdbool.h>
#include <libusb.h>

#include "stlink.h"
#include "stlink/logging.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STLINK_USB_VID_ST            0x0483
#define STLINK_USB_PID_STLINK        0x3744
#define STLINK_USB_PID_STLINK_32L    0x3748
#define STLINK_USB_PID_STLINK_NUCLEO 0x374b

#define STLINK_SG_SIZE 31
#define STLINK_CMD_SIZE 16

    struct stlink_libusb {
        libusb_context* libusb_ctx;
        libusb_device_handle* usb_handle;
        unsigned int ep_req;
        unsigned int ep_rep;
        int protocoll;
        unsigned int sg_transfer_idx;
        unsigned int cmd_len;
    };

    /**
     * Open a stlink
     * @param verbose Verbosity loglevel
     * @param reset   Reset stlink programmer
     * @param serial  Serial number to search for, when NULL the first stlink found is opened (binary format)
     * @retval NULL   Error while opening the stlink
     * @retval !NULL  Stlink found and ready to use
     */
    stlink_t *stlink_open_usb(enum ugly_loglevel verbose, bool reset, char serial[16]);
    size_t stlink_probe_usb(stlink_t **stdevs[]);
    void stlink_probe_usb_free(stlink_t **stdevs[], size_t size);

#ifdef __cplusplus
}
#endif

#endif /* STLINK_USB_H */

