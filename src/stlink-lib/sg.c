/*
 * Copyright (c) 2010 "Capt'ns Missing Link" Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 * A linux stlink access demo. The purpose of this file is to mitigate the usual
 * "reinventing the wheel" force by incompatible licenses and give you an idea,
 * how to access the stlink device. That doesn't mean you should be a free-loader
 * and not contribute your improvements to this code.
 *
 * Author: Martin Capitanio <m@capitanio.org>
 * The stlink related constants kindly provided by Oliver Spencer (OpenOCD)
 * for use in a GPL compatible license.
 *
 * Tested compatibility: linux, gcc >= 4.3.3
 *
 * The communication is based on standard USB mass storage device
 * BOT (Bulk Only Transfer)
 * - Endpoint 1: BULK_IN, 64 bytes max
 * - Endpoint 2: BULK_OUT, 64 bytes max
 *
 * All CBW transfers are ordered with the LSB (byte 0) first (little endian).
 * Any command must be answered before sending the next command.
 * Each USB transfer must complete in less than 1s.
 *
 * SB Device Class Definition for Mass Storage Devices:
 * www.usb.org/developers/devclass_docs/usbmassbulk_10.pdf
 *
 * dt		- Data Transfer (IN/OUT)
 * CBW      - Command Block Wrapper
 * CSW		- Command Status Wrapper
 * RFU		- Reserved for Future Use
 *
 * Originally, this driver used scsi pass through commands, which required the
 * usb-storage module to be loaded, providing the /dev/sgX links.  The USB mass
 * storage implementation on the STLinkv1 is however terribly broken, and it can
 * take many minutes for the kernel to give up.
 *
 * However, in Nov 2011, the scsi pass through was replaced by raw libusb, so
 * instead of having to let usb-storage struggle with the device, and also greatly
 * limiting the portability of the driver, you can now tell usb-storage to simply
 * ignore this device completely.
 *
 * usb-storage.quirks
 * http://git.kernel.org/?p=linux/kernel/git/torvalds/linux-2.6.git;a=blob_plain;f=Documentation/kernel-parameters.txt
 * Each entry has the form VID:PID:Flags where VID and PID are Vendor and Product
 * ID values (4-digit hex numbers) and Flags is a set of characters, each corresponding
 * to a common usb-storage quirk flag as follows:
 *
 * a = SANE_SENSE (collect more than 18 bytes of sense data);
 * b = BAD_SENSE (don't collect more than 18 bytes of sense data);
 * c = FIX_CAPACITY (decrease the reported device capacity by one sector);
 * h = CAPACITY_HEURISTICS (decrease the reported device capacity by one sector if the number is odd);
 * i = IGNORE_DEVICE (don't bind to this device);
 * l = NOT_LOCKABLE (don't try to lock and unlock ejectable media);
 * m = MAX_SECTORS_64 (don't transfer more than 64 sectors = 32 KB at a time);
 * o = CAPACITY_OK (accept the capacity reported by the device);
 * r = IGNORE_RESIDUE (the device reports bogus residue values);
 * s = SINGLE_LUN (the device has only one Logical Unit);
 * w = NO_WP_DETECT (don't test whether the medium is write-protected).
 *
 * Example: quirks=0419:aaf5:rl,0421:0433:rc
 * http://permalink.gmane.org/gmane.linux.usb.general/35053
 *
 * For the stlinkv1, you just want the following
 *
 * modprobe -r usb-storage && modprobe usb-storage quirks=483:3744:i
 *
 * Equivalently, you can add a line saying
 *
 * options usb-storage quirks=483:3744:i
 *
 * to your /etc/modprobe.conf or /etc/modprobe.d/local.conf (or add the "quirks=..."
 *         part to an existing options line for usb-storage).
 */


#define __USE_GNU
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>

#include <stlink.h>
#include "logging.h"
#include "sg.h"

#define STLINK_OK    0x80
#define STLINK_FALSE 0x81

static void clear_cdb(struct stlink_libsg *sl) {
    for (size_t i = 0; i < sizeof(sl->cdb_cmd_blk); i++) { sl->cdb_cmd_blk[i] = 0; }

    // set default
    sl->cdb_cmd_blk[0] = STLINK_DEBUG_COMMAND;
    sl->q_data_dir = Q_DATA_IN;
}

/**
 * Close and free any _backend_ related information...
 * @param sl
 */
void _stlink_sg_close(stlink_t *sl) {
    if (sl) {
        struct stlink_libsg *slsg = sl->backend_data;
        libusb_close(slsg->usb_handle);
        libusb_exit(slsg->libusb_ctx);
        free(slsg);
    }
}

static int get_usb_mass_storage_status(libusb_device_handle *handle, uint8_t endpoint, uint32_t *tag) {
    unsigned char csw[13];
    memset(csw, 0, sizeof(csw));
    int transferred;
    int ret;
    int try = 0;

    do {
        ret = libusb_bulk_transfer(handle, endpoint, (unsigned char *)&csw, sizeof(csw),
                                   &transferred, SG_TIMEOUT_MSEC);

        if (ret == LIBUSB_ERROR_PIPE) { libusb_clear_halt(handle, endpoint); }

        try++;
    } while ((ret == LIBUSB_ERROR_PIPE) && (try < 3));

    if (ret != LIBUSB_SUCCESS) {
        WLOG("%s: receiving failed: %d\n", __func__, ret);
        return(-1);
    }

    if (transferred != sizeof(csw)) {
        WLOG("%s: received unexpected amount: %d\n", __func__, transferred);
        return(-1);
    }

    uint32_t rsig = read_uint32(csw, 0);
    uint32_t rtag = read_uint32(csw, 4);
    /* uint32_t residue = read_uint32(csw, 8); */
#define USB_CSW_SIGNATURE 0x53425355  // 'U' 'S' 'B' 'S' (reversed)

    if (rsig != USB_CSW_SIGNATURE) {
        WLOG("status signature was invalid: %#x\n", rsig);
        return(-1);
    }

    *tag = rtag;
    uint8_t rstatus = csw[12];
    return(rstatus);
}

static int dump_CDB_command(uint8_t *cdb, uint8_t cdb_len) {
    char dbugblah[100];
    char *dbugp = dbugblah;
    dbugp += sprintf(dbugp, "Sending CDB [");

    for (uint8_t i = 0; i < cdb_len; i++) {
        dbugp += sprintf(dbugp, " %#02x", (unsigned int)cdb[i]);
    }

    sprintf(dbugp, "]\n");
    DLOG(dbugblah);
    return(0);
}

/**
 * Wraps a CDB mass storage command in the appropriate gunk to get it down
 * @param handle
 * @param endpoint
 * @param cdb
 * @param cdb_length
 * @param lun
 * @param flags
 * @param expected_rx_size
 * @return
 */
int send_usb_mass_storage_command(libusb_device_handle *handle, uint8_t endpoint_out,
                                  uint8_t *cdb, uint8_t cdb_length,
                                  uint8_t lun, uint8_t flags, uint32_t expected_rx_size) {
    DLOG("Sending usb m-s cmd: cdblen:%d, rxsize=%d\n", cdb_length, expected_rx_size);
    dump_CDB_command(cdb, cdb_length);

    static uint32_t tag;

    if (tag == 0) { tag = 1; }

    int try = 0;
    int ret = 0;
    int real_transferred;
    int i = 0;

    uint8_t c_buf[STLINK_SG_SIZE];
    // tag is allegedly ignored... TODO - verify
    c_buf[i++] = 'U';
    c_buf[i++] = 'S';
    c_buf[i++] = 'B';
    c_buf[i++] = 'C';
    write_uint32(&c_buf[i], tag);
    uint32_t this_tag = tag++;
    write_uint32(&c_buf[i + 4], expected_rx_size);
    i += 8;
    c_buf[i++] = flags;
    c_buf[i++] = lun;

    c_buf[i++] = cdb_length;

    // now the actual CDB request
    assert(cdb_length <= CDB_SL);
    memcpy(&(c_buf[i]), cdb, cdb_length);

    int sending_length = STLINK_SG_SIZE;

    // send....
    do {
        ret = libusb_bulk_transfer(handle, endpoint_out, c_buf, sending_length,
                                   &real_transferred, SG_TIMEOUT_MSEC);

        if (ret == LIBUSB_ERROR_PIPE) {
            libusb_clear_halt(handle, endpoint_out);
        }

        try++;
    } while ((ret == LIBUSB_ERROR_PIPE) && (try < 3));

    if (ret != LIBUSB_SUCCESS) {
        WLOG("sending failed: %d\n", ret);
        return(-1);
    }

    return(this_tag);
}

/**
 * Straight from stm8 stlink code...
 * @param handle
 * @param endpoint_in
 * @param endpoint_out
 */
static void get_sense(libusb_device_handle *handle, uint8_t endpoint_in, uint8_t endpoint_out) {
    DLOG("Fetching sense...\n");
    uint8_t cdb[16];
    memset(cdb, 0, sizeof(cdb));
#define REQUEST_SENSE 0x03
#define REQUEST_SENSE_LENGTH 18
    cdb[0] = REQUEST_SENSE;
    cdb[4] = REQUEST_SENSE_LENGTH;
    uint32_t tag = send_usb_mass_storage_command(handle, endpoint_out, cdb, sizeof(cdb), 0,
                                                 LIBUSB_ENDPOINT_IN, REQUEST_SENSE_LENGTH);

    if (tag == 0) {
        WLOG("refusing to send request sense with tag 0\n");
        return;
    }

    unsigned char sense[REQUEST_SENSE_LENGTH];
    int transferred;
    int ret;
    int try = 0;

    do {
        ret = libusb_bulk_transfer(handle, endpoint_in, sense, sizeof(sense),
                                   &transferred, SG_TIMEOUT_MSEC);

        if (ret == LIBUSB_ERROR_PIPE) { libusb_clear_halt(handle, endpoint_in); }

        try++;
    } while ((ret == LIBUSB_ERROR_PIPE) && (try < 3));

    if (ret != LIBUSB_SUCCESS) {
        WLOG("receiving sense failed: %d\n", ret);
        return;
    }

    if (transferred != sizeof(sense)) {
        WLOG("received unexpected amount of sense: %d != %d\n", transferred, sizeof(sense));
    }

    uint32_t received_tag;
    int status = get_usb_mass_storage_status(handle, endpoint_in, &received_tag);

    if (status != 0) {
        WLOG("receiving sense failed with status: %02x\n", status);
        return;
    }

    if (sense[0] != 0x70 && sense[0] != 0x71) {
        WLOG("No sense data\n");
    } else {
        WLOG("Sense KCQ: %02X %02X %02X\n", sense[2] & 0x0f, sense[12], sense[13]);
    }
}

/**
 * Just send a buffer on an endpoint, no questions asked.
 * Handles repeats, and time outs.  Also handles reading status reports and sense
 * @param handle libusb device *
 * @param endpoint_out sends
 * @param endpoint_in used to read status reports back in
 * @param cbuf  what to send
 * @param length how much to send
 * @return number of bytes actually sent, or -1 for failures.
 */
int send_usb_data_only(libusb_device_handle *handle, unsigned char endpoint_out,
                       unsigned char endpoint_in, unsigned char *cbuf, unsigned int length) {
    int ret;
    int real_transferred;
    int try = 0;

    do {
        ret = libusb_bulk_transfer(handle, endpoint_out, cbuf, length,
                                   &real_transferred, SG_TIMEOUT_MSEC);

        if (ret == LIBUSB_ERROR_PIPE) { libusb_clear_halt(handle, endpoint_out); }

        try++;
    } while ((ret == LIBUSB_ERROR_PIPE) && (try < 3));

    if (ret != LIBUSB_SUCCESS) {
        WLOG("sending failed: %d\n", ret);
        return(-1);
    }

    // now, swallow up the status, so that things behave nicely...
    uint32_t received_tag;
    // -ve is for my errors, 0 is good, +ve is libusb sense status bytes
    int status = get_usb_mass_storage_status(handle, endpoint_in, &received_tag);

    if (status < 0) {
        WLOG("receiving status failed: %d\n", status);
        return(-1);
    }

    if (status != 0) {
        WLOG("receiving status not passed :(: %02x\n", status);
    }

    if (status == 1) {
        get_sense(handle, endpoint_in, endpoint_out);
        return(-1);
    }

    return(real_transferred);
}

int stlink_q(stlink_t *sl) {
    struct stlink_libsg* sg = sl->backend_data;
    // uint8_t cdb_len = 6;  // FIXME varies!!!
    uint8_t cdb_len = 10;  // FIXME varies!!!
    uint8_t lun = 0;  // always zero...
    uint32_t tag = send_usb_mass_storage_command(sg->usb_handle, sg->ep_req,
                                                 sg->cdb_cmd_blk, cdb_len, lun,
                                                 LIBUSB_ENDPOINT_IN, sl->q_len);


    // now wait for our response...
    // length copied from stlink-usb...
    int rx_length = sl->q_len;
    int try = 0;
    int real_transferred;
    int ret;

    if (rx_length > 0) {
        do {
            ret = libusb_bulk_transfer(sg->usb_handle, sg->ep_rep, sl->q_buf, rx_length,
                                       &real_transferred, SG_TIMEOUT_MSEC);

            if (ret == LIBUSB_ERROR_PIPE) { libusb_clear_halt(sg->usb_handle, sg->ep_req); }

            try++;
        } while ((ret == LIBUSB_ERROR_PIPE) && (try < 3));

        if (ret != LIBUSB_SUCCESS) {
            WLOG("Receiving failed: %d\n", ret);
            return(-1);
        }

        if (real_transferred != rx_length) {
            WLOG("received unexpected amount: %d != %d\n", real_transferred, rx_length);
        }
    }

    uint32_t received_tag;
    // -ve is for my errors, 0 is good, +ve is libusb sense status bytes
    int status = get_usb_mass_storage_status(sg->usb_handle, sg->ep_rep, &received_tag);

    if (status < 0) {
        WLOG("receiving status failed: %d\n", status);
        return(-1);
    }

    if (status != 0) {
        WLOG("receiving status not passed :(: %02x\n", status);
    }

    if (status == 1) {
        get_sense(sg->usb_handle, sg->ep_rep, sg->ep_req);
        return(-1);
    }

    if (received_tag != tag) {
        WLOG("received tag %d but expected %d\n", received_tag, tag);
        // return -1;
    }

    if (rx_length > 0 && real_transferred != rx_length) {
        return(-1);
    }

    return(0);
}

// TODO: thinking, cleanup
void stlink_stat(stlink_t *stl, char *txt) {
    if (stl->q_len <= 0) { return; }

    stlink_print_data(stl);

    switch (stl->q_buf[0]) {
    case STLINK_OK:
        DLOG("  %s: ok\n", txt);
        return;
    case STLINK_FALSE:
        DLOG("  %s: false\n", txt);
        return;
    default:
        DLOG("  %s: unknown\n", txt);
    }
}

int _stlink_sg_version(stlink_t *stl) {
    struct stlink_libsg *sl = stl->backend_data;
    clear_cdb(sl);
    sl->cdb_cmd_blk[0] = STLINK_GET_VERSION;
    stl->q_len = 6;
    sl->q_addr = 0;
    return(stlink_q(stl));
}

// Get stlink mode:
// STLINK_DEV_DFU_MODE || STLINK_DEV_MASS_MODE || STLINK_DEV_DEBUG_MODE
// usb dfu             || usb mass             || jtag or swd
int _stlink_sg_current_mode(stlink_t *stl) {
    struct stlink_libsg *sl = stl->backend_data;
    clear_cdb(sl);
    sl->cdb_cmd_blk[0] = STLINK_GET_CURRENT_MODE;
    stl->q_len = 2;
    sl->q_addr = 0;

    if (stlink_q(stl)) { return(-1); }

    return(stl->q_buf[0]);
}

// exit the mass mode and enter the swd debug mode.
int _stlink_sg_enter_swd_mode(stlink_t *sl) {
    struct stlink_libsg *sg = sl->backend_data;
    clear_cdb(sg);
    sg->cdb_cmd_blk[1] = STLINK_DEBUG_APIV1_ENTER;
    sg->cdb_cmd_blk[2] = STLINK_DEBUG_ENTER_SWD;
    sl->q_len = 0; // >0 -> aboard
    return(stlink_q(sl));
}

// exit the mass mode and enter the jtag debug mode.
// (jtag is disabled in the discovery's stlink firmware)
int _stlink_sg_enter_jtag_mode(stlink_t *sl) {
    struct stlink_libsg *sg = sl->backend_data;
    DLOG("\n*** stlink_enter_jtag_mode ***\n");
    clear_cdb(sg);
    sg->cdb_cmd_blk[1] = STLINK_DEBUG_APIV1_ENTER;
    sg->cdb_cmd_blk[2] = STLINK_DEBUG_ENTER_JTAG;
    sl->q_len = 0;
    return(stlink_q(sl));
}

// XXX kernel driver performs reset, the device temporally disappears
// Suspect this is no longer the case when we have ignore on? RECHECK
int _stlink_sg_exit_dfu_mode(stlink_t *sl) {
    struct stlink_libsg *sg = sl->backend_data;
    DLOG("\n*** stlink_exit_dfu_mode ***\n");
    clear_cdb(sg);
    sg->cdb_cmd_blk[0] = STLINK_DFU_COMMAND;
    sg->cdb_cmd_blk[1] = STLINK_DFU_EXIT;
    sl->q_len = 0; // ??
    return(stlink_q(sl));
    /*
       [135121.844564] sd 19:0:0:0: [sdb] Unhandled error code
       [135121.844569] sd 19:0:0:0: [sdb] Result: hostbyte=DID_ERROR driverbyte=DRIVER_OK
       [135121.844574] sd 19:0:0:0: [sdb] CDB: Read(10): 28 00 00 00 10 00 00 00 08 00
       [135121.844584] end_request: I/O error, dev sdb, sector 4096
       [135121.844590] Buffer I/O error on device sdb, logical block 512
       [135130.122567] usb 6-1: reset full speed USB device using uhci_hcd and address 7
       [135130.274551] usb 6-1: device firmware changed
       [135130.274618] usb 6-1: USB disconnect, address 7
       [135130.275186] VFS: busy inodes on changed media or resized disk sdb
       [135130.275424] VFS: busy inodes on changed media or resized disk sdb
       [135130.286758] VFS: busy inodes on changed media or resized disk sdb
       [135130.292796] VFS: busy inodes on changed media or resized disk sdb
       [135130.301481] VFS: busy inodes on changed media or resized disk sdb
       [135130.304316] VFS: busy inodes on changed media or resized disk sdb
       [135130.431113] usb 6-1: new full speed USB device using uhci_hcd and address 8
       [135130.629444] usb-storage 6-1:1.0: Quirks match for vid 0483 pid 3744: 102a1
       [135130.629492] scsi20 : usb-storage 6-1:1.0
       [135131.625600] scsi 20:0:0:0: Direct-Access     STM32                          PQ: 0 ANSI: 0
       [135131.627010] sd 20:0:0:0: Attached scsi generic sg2 type 0
       [135131.633603] sd 20:0:0:0: [sdb] 64000 512-byte logical blocks: (32.7 MB/31.2 MiB)
       [135131.633613] sd 20:0:0:0: [sdb] Assuming Write Enabled
       [135131.633620] sd 20:0:0:0: [sdb] Assuming drive cache: write through
       [135131.640584] sd 20:0:0:0: [sdb] Assuming Write Enabled
       [135131.640592] sd 20:0:0:0: [sdb] Assuming drive cache: write through
       [135131.640609]  sdb:
       [135131.652634] sd 20:0:0:0: [sdb] Assuming Write Enabled
       [135131.652639] sd 20:0:0:0: [sdb] Assuming drive cache: write through
       [135131.652645] sd 20:0:0:0: [sdb] Attached SCSI removable disk
       [135131.671536] sd 20:0:0:0: [sdb] Result: hostbyte=DID_OK driverbyte=DRIVER_SENSE
       [135131.671548] sd 20:0:0:0: [sdb] Sense Key : Illegal Request [current]
       [135131.671553] sd 20:0:0:0: [sdb] Add. Sense: Logical block address out of range
       [135131.671560] sd 20:0:0:0: [sdb] CDB: Read(10): 28 00 00 00 f9 80 00 00 08 00
       [135131.671570] end_request: I/O error, dev sdb, sector 63872
       [135131.671575] Buffer I/O error on device sdb, logical block 7984
       [135131.678527] sd 20:0:0:0: [sdb] Result: hostbyte=DID_OK driverbyte=DRIVER_SENSE
       [135131.678532] sd 20:0:0:0: [sdb] Sense Key : Illegal Request [current]
       [135131.678537] sd 20:0:0:0: [sdb] Add. Sense: Logical block address out of range
       [135131.678542] sd 20:0:0:0: [sdb] CDB: Read(10): 28 00 00 00 f9 80 00 00 08 00
       [135131.678551] end_request: I/O error, dev sdb, sector 63872
       ...
       [135131.853565] end_request: I/O error, dev sdb, sector 4096
     */
}

int _stlink_sg_core_id(stlink_t *sl) {
    struct stlink_libsg *sg = sl->backend_data;
    int ret;
    clear_cdb(sg);
    sg->cdb_cmd_blk[1] = STLINK_DEBUG_READCOREID;
    sl->q_len = 4;
    sg->q_addr = 0;
    ret = stlink_q(sl);

    if (ret) { return(ret); }

    sl->core_id = read_uint32(sl->q_buf, 0);
    return(0);
}

// arm-core reset -> halted state.
int _stlink_sg_reset(stlink_t *sl) {
    struct stlink_libsg *sg = sl->backend_data;
    clear_cdb(sg);
    sg->cdb_cmd_blk[1] = STLINK_DEBUG_APIV1_RESETSYS;
    sl->q_len = 2;
    sg->q_addr = 0;

    if (stlink_q(sl)) { return(-1); }

    // Reset through AIRCR so NRST does not need to be connected
    if (stlink_write_debug32(sl, STLINK_REG_AIRCR,
                             STLINK_REG_AIRCR_VECTKEY | \
                             STLINK_REG_AIRCR_SYSRESETREQ)) {
        return(-1);
    }

    stlink_stat(sl, "core reset");
    return(0);
}

// arm-core reset -> halted state.
int _stlink_sg_jtag_reset(stlink_t *sl, int value) {
    struct stlink_libsg *sg = sl->backend_data;
    clear_cdb(sg);
    sg->cdb_cmd_blk[1] = STLINK_JTAG_DRIVE_NRST;
    sg->cdb_cmd_blk[2] = (value) ? 0 : 1;
    sl->q_len = 3;
    sg->q_addr = 2;

    if (stlink_q(sl)) { return(-1); }

    stlink_stat(sl, "core reset");

    return(0);
}

// arm-core status: halted or running.
int _stlink_sg_status(stlink_t *sl) {
    struct stlink_libsg *sg = sl->backend_data;
    clear_cdb(sg);
    sg->cdb_cmd_blk[1] = STLINK_DEBUG_GETSTATUS;
    sl->q_len = 2;
    sg->q_addr = 0;
    return(stlink_q(sl));
}

// force the core into the debug mode -> halted state.
int _stlink_sg_force_debug(stlink_t *sl) {
    struct stlink_libsg *sg = sl->backend_data;
    clear_cdb(sg);
    sg->cdb_cmd_blk[1] = STLINK_DEBUG_FORCEDEBUG;
    sl->q_len = 2;
    sg->q_addr = 0;

    if (stlink_q(sl)) { return(-1); }

    stlink_stat(sl, "force debug");
    return(0);
}

// read all arm-core registers.
int _stlink_sg_read_all_regs(stlink_t *sl, struct stlink_reg *regp) {
    struct stlink_libsg *sg = sl->backend_data;

    clear_cdb(sg);
    sg->cdb_cmd_blk[1] = STLINK_DEBUG_APIV1_READALLREGS;
    sl->q_len = 84;
    sg->q_addr = 0;

    if (stlink_q(sl)) { return(-1); }

    stlink_print_data(sl);

    // TODO: most of this should be re-extracted up....

    // 0-3 | 4-7 | ... | 60-63 | 64-67 | 68-71   | 72-75      | 76-79 | 80-83
    // r0  | r1  | ... | r15   | xpsr  | main_sp | process_sp | rw    | rw2
    for (int i = 0; i < 16; i++) {
        regp->r[i] = read_uint32(sl->q_buf, 4 * i);

        if (sl->verbose > 1) { DLOG("r%2d = 0x%08x\n", i, regp->r[i]); }
    }

    regp->xpsr = read_uint32(sl->q_buf, 64);
    regp->main_sp = read_uint32(sl->q_buf, 68);
    regp->process_sp = read_uint32(sl->q_buf, 72);
    regp->rw = read_uint32(sl->q_buf, 76);
    regp->rw2 = read_uint32(sl->q_buf, 80);

    if (sl->verbose < 2) { return(0); }

    DLOG("xpsr       = 0x%08x\n", regp->xpsr);
    DLOG("main_sp    = 0x%08x\n", regp->main_sp);
    DLOG("process_sp = 0x%08x\n", regp->process_sp);
    DLOG("rw         = 0x%08x\n", regp->rw);
    DLOG("rw2        = 0x%08x\n", regp->rw2);

    return(0);
}

// read an arm-core register, the index must be in the range 0..20.
//  0  |  1  | ... |  15   |  16   |   17    |   18       |  19   |  20
// r0  | r1  | ... | r15   | xpsr  | main_sp | process_sp | rw    | rw2

int _stlink_sg_read_reg(stlink_t *sl, int r_idx, struct stlink_reg *regp) {
    struct stlink_libsg *sg = sl->backend_data;
    clear_cdb(sg);
    sg->cdb_cmd_blk[1] = STLINK_DEBUG_APIV1_READREG;
    sg->cdb_cmd_blk[2] = r_idx;
    sl->q_len = 4;
    sg->q_addr = 0;

    if (stlink_q(sl)) { return(-1); }

    //  0  |  1  | ... |  15   |  16   |   17    |   18       |  19   |  20
    // 0-3 | 4-7 | ... | 60-63 | 64-67 | 68-71   | 72-75      | 76-79 | 80-83
    // r0  | r1  | ... | r15   | xpsr  | main_sp | process_sp | rw    | rw2
    stlink_print_data(sl);

    uint32_t r = read_uint32(sl->q_buf, 0);
    DLOG("r_idx (%2d) = 0x%08x\n", r_idx, r);

    switch (r_idx) {
    case 16:
        regp->xpsr = r;
        break;
    case 17:
        regp->main_sp = r;
        break;
    case 18:
        regp->process_sp = r;
        break;
    case 19:
        regp->rw = r; // XXX ?(primask, basemask etc.)
        break;
    case 20:
        regp->rw2 = r; // XXX ?(primask, basemask etc.)
        break;
    default:
        regp->r[r_idx] = r;
    }

    return(0);
}

// write an arm-core register. Index:
//  0  |  1  | ... |  15   |  16   |   17    |   18       |  19   |  20
// r0  | r1  | ... | r15   | xpsr  | main_sp | process_sp | rw    | rw2

int _stlink_sg_write_reg(stlink_t *sl, uint32_t reg, int idx) {
    struct stlink_libsg *sg = sl->backend_data;
    clear_cdb(sg);
    sg->cdb_cmd_blk[1] = STLINK_DEBUG_APIV1_WRITEREG;
    //   2: reg index
    // 3-6: reg content
    sg->cdb_cmd_blk[2] = idx;
    write_uint32(sg->cdb_cmd_blk + 3, reg);
    sl->q_len = 2;
    sg->q_addr = 0;

    if (stlink_q(sl)) { return(-1); }

    stlink_stat(sl, "write reg");
    return(0);
}

// write a register of the debug module of the core.
// XXX ?(atomic writes)
// TODO: test
void stlink_write_dreg(stlink_t *sl, uint32_t reg, uint32_t addr) {
    struct stlink_libsg *sg = sl->backend_data;
    DLOG("\n*** stlink_write_dreg ***\n");
    clear_cdb(sg);
    sg->cdb_cmd_blk[1] = STLINK_DEBUG_APIV1_WRITEDEBUGREG;
    // 2-5: address of reg of the debug module
    // 6-9: reg content
    write_uint32(sg->cdb_cmd_blk + 2, addr);
    write_uint32(sg->cdb_cmd_blk + 6, reg);
    sl->q_len = 2;
    sg->q_addr = addr;
    stlink_q(sl);
    stlink_stat(sl, "write debug reg");
}

// force the core exit the debug mode.
int _stlink_sg_run(stlink_t *sl) {
    struct stlink_libsg *sg = sl->backend_data;
    clear_cdb(sg);
    sg->cdb_cmd_blk[1] = STLINK_DEBUG_RUNCORE;
    sl->q_len = 2;
    sg->q_addr = 0;

    if (stlink_q(sl)) { return(-1); }

    stlink_stat(sl, "run core");

    return(0);
}

// step the arm-core.
int _stlink_sg_step(stlink_t *sl) {
    struct stlink_libsg *sg = sl->backend_data;
    clear_cdb(sg);
    sg->cdb_cmd_blk[1] = STLINK_DEBUG_STEPCORE;
    sl->q_len = 2;
    sg->q_addr = 0;

    if (stlink_q(sl)) { return(-1); }

    stlink_stat(sl, "step core");
    return(0);
}

// TODO: test and make delegate!
// see Cortex-M3 Technical Reference Manual
void stlink_set_hw_bp(stlink_t *sl, int fp_nr, uint32_t addr, int fp) {
    DLOG("\n*** stlink_set_hw_bp ***\n");
    struct stlink_libsg *sg = sl->backend_data;
    clear_cdb(sg);
    sg->cdb_cmd_blk[1] = STLINK_DEBUG_APIV1_SETFP;
    // 2:The number of the flash patch used to set the breakpoint
    // 3-6: Address of the breakpoint (LSB)
    // 7: FP_ALL (0x02) / FP_UPPER (0x01) / FP_LOWER (0x00)
    sl->q_buf[2] = fp_nr;
    write_uint32(sl->q_buf, addr);
    sl->q_buf[7] = fp;

    sl->q_len = 2;
    stlink_q(sl);
    stlink_stat(sl, "set flash breakpoint");
}

// TODO: test and make delegate!
void stlink_clr_hw_bp(stlink_t *sl, int fp_nr) {
    struct stlink_libsg *sg = sl->backend_data;
    DLOG("\n*** stlink_clr_hw_bp ***\n");
    clear_cdb(sg);
    sg->cdb_cmd_blk[1] = STLINK_DEBUG_APIV1_CLEARFP;
    sg->cdb_cmd_blk[2] = fp_nr;

    sl->q_len = 2;
    stlink_q(sl);
    stlink_stat(sl, "clear flash breakpoint");
}

// read a "len" bytes to the sl->q_buf from the memory, max 6kB (6144 bytes)
int _stlink_sg_read_mem32(stlink_t *sl, uint32_t addr, uint16_t len) {
    struct stlink_libsg *sg = sl->backend_data;
    clear_cdb(sg);
    sg->cdb_cmd_blk[1] = STLINK_DEBUG_READMEM_32BIT;
    // 2-5: addr
    // 6-7: len
    write_uint32(sg->cdb_cmd_blk + 2, addr);
    write_uint16(sg->cdb_cmd_blk + 6, len);

    // data_in 0-0x40-len
    // !!! len _and_ q_len must be max 6k,
    //     i.e. >1024 * 6 = 6144 -> aboard)
    // !!! if len < q_len: 64*k, 1024*n, n=1..5  -> aboard
    //     (broken residue issue)
    sl->q_len = len;
    sg->q_addr = addr;

    if (stlink_q(sl)) { return(-1); }

    stlink_print_data(sl);
    return(0);
}

// write a "len" bytes from the sl->q_buf to the memory, max 64 Bytes.
int _stlink_sg_write_mem8(stlink_t *sl, uint32_t addr, uint16_t len) {
    struct stlink_libsg *sg = sl->backend_data;
    int ret;

    clear_cdb(sg);
    sg->cdb_cmd_blk[1] = STLINK_DEBUG_WRITEMEM_8BIT;
    // 2-5: addr
    // 6-7: len (>0x40 (64) -> aboard)
    write_uint32(sg->cdb_cmd_blk + 2, addr);
    write_uint16(sg->cdb_cmd_blk + 6, len);

    // this sends the command...
    ret = send_usb_mass_storage_command(sg->usb_handle,
                                        sg->ep_req, sg->cdb_cmd_blk, CDB_SL, 0, 0, 0);

    if (ret == -1) { return(ret); }

    // This sends the data...
    ret = send_usb_data_only(sg->usb_handle,
                             sg->ep_req, sg->ep_rep, sl->q_buf, len);

    if (ret == -1) { return(ret); }

    stlink_print_data(sl);
    return(0);
}

// write a "len" bytes from the sl->q_buf to the memory, max Q_BUF_LEN bytes.
int _stlink_sg_write_mem32(stlink_t *sl, uint32_t addr, uint16_t len) {
    struct stlink_libsg *sg = sl->backend_data;
    int ret;

    clear_cdb(sg);
    sg->cdb_cmd_blk[1] = STLINK_DEBUG_WRITEMEM_32BIT;
    // 2-5: addr
    // 6-7: len "unlimited"
    write_uint32(sg->cdb_cmd_blk + 2, addr);
    write_uint16(sg->cdb_cmd_blk + 6, len);

    // this sends the command...
    ret = send_usb_mass_storage_command(sg->usb_handle,
                                        sg->ep_req, sg->cdb_cmd_blk, CDB_SL, 0, 0, 0);

    if (ret == -1) { return(ret); }

    // This sends the data...
    ret = send_usb_data_only(sg->usb_handle,
                             sg->ep_req, sg->ep_rep, sl->q_buf, len);

    if (ret == -1) { return(ret); }

    stlink_print_data(sl);
    return(0);
}

// write one DWORD data to memory
int _stlink_sg_write_debug32(stlink_t *sl, uint32_t addr, uint32_t data) {
    struct stlink_libsg *sg = sl->backend_data;
    clear_cdb(sg);
    sg->cdb_cmd_blk[1] = STLINK_JTAG_WRITEDEBUG_32BIT;
    // 2-5: addr
    write_uint32(sg->cdb_cmd_blk + 2, addr);
    write_uint32(sg->cdb_cmd_blk + 6, data);
    sl->q_len = 2;
    return(stlink_q(sl));
}

// read one DWORD data from memory
int _stlink_sg_read_debug32(stlink_t *sl, uint32_t addr, uint32_t *data) {
    struct stlink_libsg *sg = sl->backend_data;
    clear_cdb(sg);
    sg->cdb_cmd_blk[1] = STLINK_JTAG_READDEBUG_32BIT;
    // 2-5: addr
    write_uint32(sg->cdb_cmd_blk + 2, addr);
    sl->q_len = 8;

    if (stlink_q(sl)) { return(-1); }

    *data = read_uint32(sl->q_buf, 4);
    return(0);
}

// exit the jtag or swd mode and enter the mass mode.
int _stlink_sg_exit_debug_mode(stlink_t *stl) {
    if (stl) {
        struct stlink_libsg* sl = stl->backend_data;
        clear_cdb(sl);
        sl->cdb_cmd_blk[1] = STLINK_DEBUG_EXIT;
        stl->q_len = 0; // >0 -> aboard
        return(stlink_q(stl));
    }

    return(0);
}

// 1) open a sg device, switch the stlink from dfu to mass mode
// 2) wait 5s until the kernel driver stops reseting the broken device
// 3) reopen the device
// 4) the device driver is now ready for a switch to jtag/swd mode
// TODO thinking, better error handling, wait until the kernel driver stops reseting the plugged-in device

static stlink_backend_t _stlink_sg_backend = {
    _stlink_sg_close,
    _stlink_sg_exit_debug_mode,
    _stlink_sg_enter_swd_mode,
    _stlink_sg_enter_jtag_mode,
    _stlink_sg_exit_dfu_mode,
    _stlink_sg_core_id,
    _stlink_sg_reset,
    _stlink_sg_jtag_reset,
    _stlink_sg_run,
    _stlink_sg_status,
    _stlink_sg_version,
    _stlink_sg_read_debug32,
    _stlink_sg_read_mem32,
    _stlink_sg_write_debug32,
    _stlink_sg_write_mem32,
    _stlink_sg_write_mem8,
    _stlink_sg_read_all_regs,
    _stlink_sg_read_reg,
    NULL,                   // read_all_unsupported_regs
    NULL,                   // read_unsupported_regs
    NULL,                   // write_unsupported_regs
    _stlink_sg_write_reg,
    _stlink_sg_step,
    _stlink_sg_current_mode,
    _stlink_sg_force_debug,
    NULL,                   // target_voltage
    NULL                    // set_swdclk
};

static stlink_t* stlink_open(const int verbose) {
    stlink_t *sl = malloc(sizeof(stlink_t));
    struct stlink_libsg *slsg = malloc(sizeof(struct stlink_libsg));

    if (sl == NULL || slsg == NULL) {
        WLOG("Couldn't malloc stlink and stlink_sg structures out of memory!\n");

        if (sl != NULL) { free(sl); }

        if (slsg != NULL) { free(slsg); }

        return(NULL);
    }

    memset(sl, 0, sizeof(stlink_t));

    if (libusb_init(&(slsg->libusb_ctx))) {
        WLOG("failed to init libusb context, wrong version of libraries?\n");
        free(sl);
        free(slsg);
        return(NULL);
    }

#if LIBUSB_API_VERSION < 0x01000106
    libusb_set_debug(slsg->libusb_ctx, ugly_libusb_log_level(verbose));
#else
    libusb_set_option(slsg->libusb_ctx, LIBUSB_OPTION_LOG_LEVEL, ugly_libusb_log_level(verbose));
#endif

    slsg->usb_handle = libusb_open_device_with_vid_pid(slsg->libusb_ctx, STLINK_USB_VID_ST, STLINK_USB_PID_STLINK);

    if (slsg->usb_handle == NULL) {
        WLOG("Failed to find an stlink v1 by VID:PID\n");
        libusb_close(slsg->usb_handle);
        libusb_exit(slsg->libusb_ctx);
        free(sl);
        free(slsg);
        return(NULL);
    }

    // TODO: Could read the interface config descriptor, and assert lots of the assumptions
    // assumption: numInterfaces is always 1...
    if (libusb_kernel_driver_active(slsg->usb_handle, 0) == 1) {
        int r = libusb_detach_kernel_driver(slsg->usb_handle, 0);

        if (r < 0) {
            WLOG("libusb_detach_kernel_driver(() error %s\n", strerror(-r));
            libusb_close(slsg->usb_handle);
            libusb_exit(slsg->libusb_ctx);
            free(sl);
            free(slsg);
            return(NULL);
        }

        DLOG("Kernel driver was successfully detached\n");
    }

    int config;

    if (libusb_get_configuration(slsg->usb_handle, &config)) {
        /* this may fail for a previous configured device */
        WLOG("libusb_get_configuration()\n");
        libusb_close(slsg->usb_handle);
        libusb_exit(slsg->libusb_ctx);
        free(sl);
        free(slsg);
        return(NULL);

    }

    // assumption: bConfigurationValue is always 1
    if (config != 1) {
        WLOG("Your stlink got into a real weird configuration, trying to fix it!\n");
        DLOG("setting new configuration (%d -> 1)\n", config);

        if (libusb_set_configuration(slsg->usb_handle, 1)) {
            /* this may fail for a previous configured device */
            WLOG("libusb_set_configuration() failed\n");
            libusb_close(slsg->usb_handle);
            libusb_exit(slsg->libusb_ctx);
            free(sl);
            free(slsg);
            return(NULL);
        }
    }

    if (libusb_claim_interface(slsg->usb_handle, 0)) {
        WLOG("libusb_claim_interface() failed\n");
        libusb_close(slsg->usb_handle);
        libusb_exit(slsg->libusb_ctx);
        free(sl);
        free(slsg);
        return(NULL);
    }

    // assumption: endpoint config is fixed mang. really.
    slsg->ep_rep = 1 /* ep rep */ | LIBUSB_ENDPOINT_IN;
    slsg->ep_req = 2 /* ep req */ | LIBUSB_ENDPOINT_OUT;

    DLOG("Successfully opened stlinkv1 by libusb :)\n");

    sl->verbose = verbose;
    sl->backend_data = slsg;
    sl->backend = &_stlink_sg_backend;

    sl->core_stat = TARGET_UNKNOWN;
    slsg->q_addr = 0;

    return(sl);
}


stlink_t* stlink_v1_open_inner(const int verbose) {
    ugly_init(verbose);
    stlink_t *sl = stlink_open(verbose);

    if (sl == NULL) {
        ELOG("Could not open stlink device\n");
        return(NULL);
    }

    stlink_version(sl);

    if ((sl->version.st_vid != STLINK_USB_VID_ST) || (sl->version.stlink_pid != STLINK_USB_PID_STLINK)) {
        ELOG("WTF? successfully opened, but unable to read version details. BROKEN!\n");
        return(NULL);
    }

    DLOG("Reading current mode...\n");

    switch (stlink_current_mode(sl)) {
    case STLINK_DEV_MASS_MODE:
        return(sl);
    case STLINK_DEV_DEBUG_MODE:
        // TODO go to mass?
        return(sl);
    default:
        ILOG("Current mode unusable, trying to get back to a useful state...\n");
        break;
    }

    DLOG("Attempting to exit DFU mode\n");
    _stlink_sg_exit_dfu_mode(sl);

    // re-query device info (and retest)
    stlink_version(sl);

    if ((sl->version.st_vid != STLINK_USB_VID_ST) || (sl->version.stlink_pid != STLINK_USB_PID_STLINK)) {
        ELOG("WTF? successfully opened, but unable to read version details. BROKEN!\n");
        return(NULL);
    }

    return(sl);
}

stlink_t* stlink_v1_open(const int verbose, int reset) {
    stlink_t *sl = stlink_v1_open_inner(verbose);

    if (sl == NULL) { return(NULL); }

    // by now, it _must_ be fully open and in a useful mode....
    stlink_enter_swd_mode(sl);

    // now we are ready to read the parameters
    if (reset) { stlink_reset(sl); }

    stlink_load_device_params(sl);
    ILOG("Successfully opened a stlink v1 debugger\n");
    return(sl);
}
