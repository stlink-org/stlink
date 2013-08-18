/* 
 * File:   stlink-usb.h
 * Author: karl
 *
 * Created on October 1, 2011, 11:29 PM
 */

#ifndef STLINK_USB_H
#define	STLINK_USB_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <libusb.h>
#include "stlink-common.h"
    
#define STLINK_SG_SIZE 31
#define STLINK_CMD_SIZE 16

    struct stlink_libusb {
        libusb_context* libusb_ctx;
        libusb_device_handle* usb_handle;
        struct libusb_transfer* req_trans;
        struct libusb_transfer* rep_trans;
        unsigned int ep_req;
        unsigned int ep_rep;
        int protocoll;
        unsigned int sg_transfer_idx;
        unsigned int cmd_len;
    };
    
    stlink_t* stlink_open_usb(const int verbose, int reset);


#ifdef	__cplusplus
}
#endif

#endif	/* STLINK_USB_H */

