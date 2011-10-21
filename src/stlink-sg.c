/*
 Copyright (c) 2010 "Capt'ns Missing Link" Authors. All rights reserved.
 Use of this source code is governed by a BSD-style
 license that can be found in the LICENSE file.

 A linux stlink access demo. The purpose of this file is to mitigate the usual
 "reinventing the wheel" force by incompatible licenses and give you an idea,
 how to access the stlink device. That doesn't mean you should be a free-loader
 and not contribute your improvements to this code.

 Author: Martin Capitanio <m@capitanio.org>
 The stlink related constants kindly provided by Oliver Spencer (OpenOCD)
 for use in a GPL compatible license.

 Code format ~ TAB = 8, K&R, linux kernel source, golang oriented
 Tested compatibility: linux, gcc >= 4.3.3

 The communication is based on standard USB mass storage device
 BOT (Bulk Only Transfer)
 - Endpoint 1: BULK_IN, 64 bytes max
 - Endpoint 2: BULK_OUT, 64 bytes max

 All CBW transfers are ordered with the LSB (byte 0) first (little endian).
 Any command must be answered before sending the next command.
 Each USB transfer must complete in less than 1s.

 SB Device Class Definition for Mass Storage Devices:
 www.usb.org/developers/devclass_docs/usbmassbulk_10.pdf

 dt		- Data Transfer (IN/OUT)
 CBW 		- Command Block Wrapper
 CSW		- Command Status Wrapper
 RFU		- Reserved for Future Use
 scsi_pt	- SCSI pass-through
 sg		- SCSI generic

 * usb-storage.quirks
 http://git.kernel.org/?p=linux/kernel/git/torvalds/linux-2.6.git;a=blob_plain;f=Documentation/kernel-parameters.txt
 Each entry has the form VID:PID:Flags where VID and PID are Vendor and Product
 ID values (4-digit hex numbers) and Flags is a set of characters, each corresponding
 to a common usb-storage quirk flag as follows:

 a = SANE_SENSE (collect more than 18 bytes of sense data);
 b = BAD_SENSE (don't collect more than 18 bytes of sense data);
 c = FIX_CAPACITY (decrease the reported device capacity by one sector);
 h = CAPACITY_HEURISTICS (decrease the reported device capacity by one sector if the number is odd);
 i = IGNORE_DEVICE (don't bind to this device);
 l = NOT_LOCKABLE (don't try to lock and unlock ejectable media);
 m = MAX_SECTORS_64 (don't transfer more than 64 sectors = 32 KB at a time);
 o = CAPACITY_OK (accept the capacity reported by the device);
 r = IGNORE_RESIDUE (the device reports bogus residue values);
 s = SINGLE_LUN (the device has only one Logical Unit);
 w = NO_WP_DETECT (don't test whether the medium is write-protected).

 Example: quirks=0419:aaf5:rl,0421:0433:rc
 http://permalink.gmane.org/gmane.linux.usb.general/35053

 modprobe -r usb-storage && modprobe usb-storage quirks=483:3744:l

 Equivalently, you can add a line saying

 options usb-storage quirks=483:3744:l

 to your /etc/modprobe.conf or /etc/modprobe.d/local.conf (or add the "quirks=..."
 part to an existing options line for usb-storage).

 https://wiki.kubuntu.org/Kernel/Debugging/USB explains the protocoll and 
 would allow to replace the sg access to pure libusb access
 */


#define __USE_GNU
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "stlink-common.h"

#if CONFIG_USE_LIBSG
// sgutils2 (apt-get install libsgutils2-dev)
#include <scsi/sg_lib.h>
#include <scsi/sg_pt.h>
#include "stlink-sg.h"
#endif


// Suspends execution of the calling process for
// (at least) ms milliseconds.

static void delay(int ms) {
    //fprintf(stderr, "*** wait %d ms\n", ms);
    usleep(1000 * ms);
}

static void clear_cdb(struct stlink_libsg *sl) {
    for (size_t i = 0; i < sizeof (sl->cdb_cmd_blk); i++)
        sl->cdb_cmd_blk[i] = 0;
    // set default
    sl->cdb_cmd_blk[0] = STLINK_DEBUG_COMMAND;
    sl->q_data_dir = Q_DATA_IN;
}

// E.g. make the valgrind happy.

static void clear_buf(stlink_t *sl) {
    DD(sl, "*** clear_buf ***\n");
    for (size_t i = 0; i < sizeof (sl->q_buf); i++)
        sl->q_buf[i] = 0;

}

// close the device, free the allocated memory

void _stlink_sg_close(stlink_t *sl) {
    if (sl) {
        struct stlink_libsg *slsg = sl->backend_data;
        scsi_pt_close_device(slsg->sg_fd);
        free(slsg);
    }
}


//TODO rewrite/cleanup, save the error in sl

static void stlink_confirm_inq(stlink_t *stl, struct sg_pt_base *ptvp) {
    struct stlink_libsg *sl = stl->backend_data;
    const int e = sl->do_scsi_pt_err;
    if (e < 0) {
        fprintf(stderr, "scsi_pt error: pass through os error: %s\n",
                safe_strerror(-e));
        return;
    } else if (e == SCSI_PT_DO_BAD_PARAMS) {
        fprintf(stderr, "scsi_pt error: bad pass through setup\n");
        return;
    } else if (e == SCSI_PT_DO_TIMEOUT) {
        fprintf(stderr, "  pass through timeout\n");
        return;
    }
    const int duration = get_scsi_pt_duration_ms(ptvp);
    if ((stl->verbose > 1) && (duration >= 0))
        DD(stl, "      duration=%d ms\n", duration);

    // XXX stlink fw sends broken residue, so ignore it and use the known q_len
    // "usb-storage quirks=483:3744:r"
    // forces residue to be ignored and calculated, but this causes aboard if
    // data_len = 0 and by some other data_len values.

    const int resid = get_scsi_pt_resid(ptvp);
    const int dsize = stl->q_len - resid;

    const int cat = get_scsi_pt_result_category(ptvp);
    char buf[512];
    unsigned int slen;

    switch (cat) {
        case SCSI_PT_RESULT_GOOD:
            if (stl->verbose && (resid > 0))
                DD(stl, "      notice: requested %d bytes but "
                    "got %d bytes, ignore [broken] residue = %d\n",
                    stl->q_len, dsize, resid);
            break;
        case SCSI_PT_RESULT_STATUS:
            if (stl->verbose) {
                sg_get_scsi_status_str(
                        get_scsi_pt_status_response(ptvp), sizeof (buf),
                        buf);
                DD(stl, "  scsi status: %s\n", buf);
            }
            return;
        case SCSI_PT_RESULT_SENSE:
            slen = get_scsi_pt_sense_len(ptvp);
            if (stl->verbose) {
                sg_get_sense_str("", sl->sense_buf, slen, (stl->verbose
                        > 1), sizeof (buf), buf);
                DD(stl, "%s", buf);
            }
            if (stl->verbose && (resid > 0)) {
                if ((stl->verbose) || (stl->q_len > 0))
                    DD(stl, "    requested %d bytes but "
                        "got %d bytes\n", stl->q_len, dsize);
            }
            return;
        case SCSI_PT_RESULT_TRANSPORT_ERR:
            if (stl->verbose) {
                get_scsi_pt_transport_err_str(ptvp, sizeof (buf), buf);
                // http://tldp.org/HOWTO/SCSI-Generic-HOWTO/x291.html
                // These codes potentially come from the firmware on a host adapter
                // or from one of several hosts that an adapter driver controls.
                // The 'host_status' field has the following values:
                //	[0x07] Internal error detected in the host adapter.
                // This may not be fatal (and the command may have succeeded).
                DD(stl, "  transport: %s", buf);
            }
            return;
        case SCSI_PT_RESULT_OS_ERR:
            if (stl->verbose) {
                get_scsi_pt_os_err_str(ptvp, sizeof (buf), buf);
                DD(stl, "  os: %s", buf);
            }
            return;
        default:
            fprintf(stderr, "  unknown pass through result "
                    "category (%d)\n", cat);
    }
}

void stlink_q(stlink_t *sl) {
    struct stlink_libsg* sg = sl->backend_data;
    DD(sl, "CDB[");
    for (int i = 0; i < CDB_SL; i++)
        DD(sl, " 0x%02x", (unsigned int) sg->cdb_cmd_blk[i]);
    DD(sl, "]\n");

    // Get control command descriptor of scsi structure,
    // (one object per command!!)
    struct sg_pt_base *ptvp = construct_scsi_pt_obj();
    if (NULL == ptvp) {
        fprintf(stderr, "construct_scsi_pt_obj: out of memory\n");
        return;
    }

    set_scsi_pt_cdb(ptvp, sg->cdb_cmd_blk, sizeof (sg->cdb_cmd_blk));

    // set buffer for sense (error information) data
    set_scsi_pt_sense(ptvp, sg->sense_buf, sizeof (sg->sense_buf));

    // Set a buffer to be used for data transferred from device
    if (sg->q_data_dir == Q_DATA_IN) {
        //clear_buf(sl);
        set_scsi_pt_data_in(ptvp, sl->q_buf, sl->q_len);
    } else {
        set_scsi_pt_data_out(ptvp, sl->q_buf, sl->q_len);
    }
    // Executes SCSI command (or at least forwards it to lower layers).
    sg->do_scsi_pt_err = do_scsi_pt(ptvp, sg->sg_fd, SG_TIMEOUT_SEC,
            sl->verbose);

    // check for scsi errors
    stlink_confirm_inq(sl, ptvp);
    // TODO recycle: clear_scsi_pt_obj(struct sg_pt_base * objp);
    destruct_scsi_pt_obj(ptvp);
}

// TODO thinking, cleanup

void stlink_stat(stlink_t *stl, char *txt) {
    if (stl->q_len <= 0)
        return;

    stlink_print_data(stl);

    switch (stl->q_buf[0]) {
        case STLINK_OK:
            DD(stl, "  %s: ok\n", txt);
            return;
        case STLINK_FALSE:
            DD(stl, "  %s: false\n", txt);
            return;
        default:
            DD(stl, "  %s: unknown\n", txt);
    }
}


static void parse_version(stlink_t *stl) {
  struct stlink_libsg *sl = stl->backend_data;

  sl->st_vid = 0;
  sl->stlink_pid = 0;

  if (stl->q_len <= 0) {
    fprintf(stderr, "Error: could not parse the stlink version");
    return;
  }

  uint32_t b0 = stl->q_buf[0]; //lsb
  uint32_t b1 = stl->q_buf[1];
  uint32_t b2 = stl->q_buf[2];
  uint32_t b3 = stl->q_buf[3];
  uint32_t b4 = stl->q_buf[4];
  uint32_t b5 = stl->q_buf[5]; //msb

  // b0 b1 || b2 b3 | b4 b5
  // 4b | 6b | 6b || 2B | 2B
  // stlink_v | jtag_v | swim_v || st_vid | stlink_pid

  sl->stlink_v = (b0 & 0xf0) >> 4;
  sl->jtag_v = ((b0 & 0x0f) << 2) | ((b1 & 0xc0) >> 6);
  sl->swim_v = b1 & 0x3f;
  sl->st_vid = (b3 << 8) | b2;
  sl->stlink_pid = (b5 << 8) | b4;
}

void _stlink_sg_version(stlink_t *stl) {
    struct stlink_libsg *sl = stl->backend_data;
    D(stl, "\n*** stlink_version ***\n");
    clear_cdb(sl);
    sl->cdb_cmd_blk[0] = STLINK_GET_VERSION;
    stl->q_len = 6;
    sl->q_addr = 0;
    stlink_q(stl);
    parse_version(stl);
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
    stlink_q(stl);
    return stl->q_buf[0];
}

// Exit the mass mode and enter the swd debug mode.

void _stlink_sg_enter_swd_mode(stlink_t *sl) {
    struct stlink_libsg *sg = sl->backend_data;
    clear_cdb(sg);
    sg->cdb_cmd_blk[1] = STLINK_DEBUG_ENTER;
    sg->cdb_cmd_blk[2] = STLINK_DEBUG_ENTER_SWD;
    sl->q_len = 0; // >0 -> aboard
    stlink_q(sl);
}

// Exit the mass mode and enter the jtag debug mode.
// (jtag is disabled in the discovery's stlink firmware)

void _stlink_sg_enter_jtag_mode(stlink_t *sl) {
    struct stlink_libsg *sg = sl->backend_data;
    D(sl, "\n*** stlink_enter_jtag_mode ***\n");
    clear_cdb(sg);
    sg->cdb_cmd_blk[1] = STLINK_DEBUG_ENTER;
    sg->cdb_cmd_blk[2] = STLINK_DEBUG_ENTER_JTAG;
    sl->q_len = 0;
    stlink_q(sl);
}

// XXX kernel driver performs reset, the device temporally disappears

void _stlink_sg_exit_dfu_mode(stlink_t *sl) {
    struct stlink_libsg *sg = sl->backend_data;
    D(sl, "\n*** stlink_exit_dfu_mode ***\n");
    clear_cdb(sg);
    sg->cdb_cmd_blk[0] = STLINK_DFU_COMMAND;
    sg->cdb_cmd_blk[1] = STLINK_DFU_EXIT;
    sl->q_len = 0; // ??
    stlink_q(sl);
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

void _stlink_sg_core_id(stlink_t *sl) {
    struct stlink_libsg *sg = sl->backend_data;
    clear_cdb(sg);
    sg->cdb_cmd_blk[1] = STLINK_DEBUG_READCOREID;
    sl->q_len = 4;
    sg->q_addr = 0;
    stlink_q(sl);
    sl->core_id = read_uint32(sl->q_buf, 0);
}

// Arm-core reset -> halted state.

void _stlink_sg_reset(stlink_t *sl) {
    struct stlink_libsg *sg = sl->backend_data;
    clear_cdb(sg);
    sg->cdb_cmd_blk[1] = STLINK_DEBUG_RESETSYS;
    sl->q_len = 2;
    sg->q_addr = 0;
    stlink_q(sl);
    stlink_stat(sl, "core reset");
}

// Arm-core status: halted or running.

void _stlink_sg_status(stlink_t *sl) {
    struct stlink_libsg *sg = sl->backend_data;
    D(sl, "\n*** stlink_status ***\n");
    clear_cdb(sg);
    sg->cdb_cmd_blk[1] = STLINK_DEBUG_GETSTATUS;
    sl->q_len = 2;
    sg->q_addr = 0;
    stlink_q(sl);
}

// Force the core into the debug mode -> halted state.

void _stlink_sg_force_debug(stlink_t *sl) {
    struct stlink_libsg *sg = sl->backend_data;
    D(sl, "\n*** stlink_force_debug ***\n");
    clear_cdb(sg);
    sg->cdb_cmd_blk[1] = STLINK_DEBUG_FORCEDEBUG;
    sl->q_len = 2;
    sg->q_addr = 0;
    stlink_q(sl);
    stlink_stat(sl, "force debug");
}

// Read all arm-core registers.

void _stlink_sg_read_all_regs(stlink_t *sl, reg *regp) {
    struct stlink_libsg *sg = sl->backend_data;

    /* unused */
    regp = regp;

    clear_cdb(sg);
    sg->cdb_cmd_blk[1] = STLINK_DEBUG_READALLREGS;
    sl->q_len = 84;
    sg->q_addr = 0;
    stlink_q(sl);
    stlink_print_data(sl);

    // TODO - most of this should be re-extracted up....
    
    // 0-3 | 4-7 | ... | 60-63 | 64-67 | 68-71   | 72-75      | 76-79 | 80-83
    // r0  | r1  | ... | r15   | xpsr  | main_sp | process_sp | rw    | rw2
    for (int i = 0; i < 16; i++) {
        sg->reg.r[i] = read_uint32(sl->q_buf, 4 * i);
        if (sl->verbose > 1)
            DD(sl, "r%2d = 0x%08x\n", i, sg->reg.r[i]);
    }
    sg->reg.xpsr = read_uint32(sl->q_buf, 64);
    sg->reg.main_sp = read_uint32(sl->q_buf, 68);
    sg->reg.process_sp = read_uint32(sl->q_buf, 72);
    sg->reg.rw = read_uint32(sl->q_buf, 76);
    sg->reg.rw2 = read_uint32(sl->q_buf, 80);
    if (sl->verbose < 2)
        return;

    DD(sl, "xpsr       = 0x%08x\n", sg->reg.xpsr);
    DD(sl, "main_sp    = 0x%08x\n", sg->reg.main_sp);
    DD(sl, "process_sp = 0x%08x\n", sg->reg.process_sp);
    DD(sl, "rw         = 0x%08x\n", sg->reg.rw);
    DD(sl, "rw2        = 0x%08x\n", sg->reg.rw2);
}

// Read an arm-core register, the index must be in the range 0..20.
//  0  |  1  | ... |  15   |  16   |   17    |   18       |  19   |  20
// r0  | r1  | ... | r15   | xpsr  | main_sp | process_sp | rw    | rw2

void _stlink_sg_read_reg(stlink_t *sl, int r_idx, reg *regp) {
    struct stlink_libsg *sg = sl->backend_data;
    clear_cdb(sg);
    sg->cdb_cmd_blk[1] = STLINK_DEBUG_READREG;
    sg->cdb_cmd_blk[2] = r_idx;
    sl->q_len = 4;
    sg->q_addr = 0;
    stlink_q(sl);
    //  0  |  1  | ... |  15   |  16   |   17    |   18       |  19   |  20
    // 0-3 | 4-7 | ... | 60-63 | 64-67 | 68-71   | 72-75      | 76-79 | 80-83
    // r0  | r1  | ... | r15   | xpsr  | main_sp | process_sp | rw    | rw2
    stlink_print_data(sl);

    uint32_t r = read_uint32(sl->q_buf, 0);
    DD(sl, "r_idx (%2d) = 0x%08x\n", r_idx, r);

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
            regp->rw = r; //XXX ?(primask, basemask etc.)
            break;
        case 20:
            regp->rw2 = r; //XXX ?(primask, basemask etc.)
            break;
        default:
            regp->r[r_idx] = r;
    }
}

// Write an arm-core register. Index:
//  0  |  1  | ... |  15   |  16   |   17    |   18       |  19   |  20
// r0  | r1  | ... | r15   | xpsr  | main_sp | process_sp | rw    | rw2

void _stlink_sg_write_reg(stlink_t *sl, uint32_t reg, int idx) {
    struct stlink_libsg *sg = sl->backend_data;
    clear_cdb(sg);
    sg->cdb_cmd_blk[1] = STLINK_DEBUG_WRITEREG;
    //   2: reg index
    // 3-6: reg content
    sg->cdb_cmd_blk[2] = idx;
    write_uint32(sg->cdb_cmd_blk + 3, reg);
    sl->q_len = 2;
    sg->q_addr = 0;
    stlink_q(sl);
    stlink_stat(sl, "write reg");
}

// Write a register of the debug module of the core.
// XXX ?(atomic writes)
// TODO test

void stlink_write_dreg(stlink_t *sl, uint32_t reg, uint32_t addr) {
    struct stlink_libsg *sg = sl->backend_data;
    D(sl, "\n*** stlink_write_dreg ***\n");
    clear_cdb(sg);
    sg->cdb_cmd_blk[1] = STLINK_DEBUG_WRITEDEBUGREG;
    // 2-5: address of reg of the debug module
    // 6-9: reg content
    write_uint32(sg->cdb_cmd_blk + 2, addr);
    write_uint32(sg->cdb_cmd_blk + 6, reg);
    sl->q_len = 2;
    sg->q_addr = addr;
    stlink_q(sl);
    stlink_stat(sl, "write debug reg");
}

// Force the core exit the debug mode.

void _stlink_sg_run(stlink_t *sl) {
    struct stlink_libsg *sg = sl->backend_data;
    D(sl, "\n*** stlink_run ***\n");
    clear_cdb(sg);
    sg->cdb_cmd_blk[1] = STLINK_DEBUG_RUNCORE;
    sl->q_len = 2;
    sg->q_addr = 0;
    stlink_q(sl);
    stlink_stat(sl, "run core");
}

// Step the arm-core.

void _stlink_sg_step(stlink_t *sl) {
    struct stlink_libsg *sg = sl->backend_data;
    clear_cdb(sg);
    sg->cdb_cmd_blk[1] = STLINK_DEBUG_STEPCORE;
    sl->q_len = 2;
    sg->q_addr = 0;
    stlink_q(sl);
    stlink_stat(sl, "step core");
}

// TODO test
// see Cortex-M3 Technical Reference Manual
// TODO make delegate!
void stlink_set_hw_bp(stlink_t *sl, int fp_nr, uint32_t addr, int fp) {
    D(sl, "\n*** stlink_set_hw_bp ***\n");
    struct stlink_libsg *sg = sl->backend_data;
    clear_cdb(sg);
    sg->cdb_cmd_blk[1] = STLINK_DEBUG_SETFP;
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

// TODO test

// TODO make delegate!
void stlink_clr_hw_bp(stlink_t *sl, int fp_nr) {
    struct stlink_libsg *sg = sl->backend_data;
    D(sl, "\n*** stlink_clr_hw_bp ***\n");
    clear_cdb(sg);
    sg->cdb_cmd_blk[1] = STLINK_DEBUG_CLEARFP;
    sg->cdb_cmd_blk[2] = fp_nr;

    sl->q_len = 2;
    stlink_q(sl);
    stlink_stat(sl, "clear flash breakpoint");
}

// Read a "len" bytes to the sl->q_buf from the memory, max 6kB (6144 bytes)

void _stlink_sg_read_mem32(stlink_t *sl, uint32_t addr, uint16_t len) {
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
    stlink_q(sl);
    stlink_print_data(sl);
}

// Write a "len" bytes from the sl->q_buf to the memory, max 64 Bytes.

void _stlink_sg_write_mem8(stlink_t *sl, uint32_t addr, uint16_t len) {
    struct stlink_libsg *sg = sl->backend_data;
    clear_cdb(sg);
    sg->cdb_cmd_blk[1] = STLINK_DEBUG_WRITEMEM_8BIT;
    // 2-5: addr
    // 6-7: len (>0x40 (64) -> aboard)
    write_uint32(sg->cdb_cmd_blk + 2, addr);
    write_uint16(sg->cdb_cmd_blk + 6, len);

    // data_out 0-len
    sl->q_len = len;
    sg->q_addr = addr;
    sg->q_data_dir = Q_DATA_OUT;
    stlink_q(sl);
    stlink_print_data(sl);
}

// Write a "len" bytes from the sl->q_buf to the memory, max Q_BUF_LEN bytes.

void _stlink_sg_write_mem32(stlink_t *sl, uint32_t addr, uint16_t len) {
    struct stlink_libsg *sg = sl->backend_data;
    clear_cdb(sg);
    sg->cdb_cmd_blk[1] = STLINK_DEBUG_WRITEMEM_32BIT;
    // 2-5: addr
    // 6-7: len "unlimited"
    write_uint32(sg->cdb_cmd_blk + 2, addr);
    write_uint16(sg->cdb_cmd_blk + 6, len);

    // data_out 0-0x40-...-len
    sl->q_len = len;
    sg->q_addr = addr;
    sg->q_data_dir = Q_DATA_OUT;
    stlink_q(sl);
    stlink_print_data(sl);
}

#if 0 /* not working */

static int write_flash_mem16
(struct stlink* sl, uint32_t addr, uint16_t val) {
    /* half word writes */
    if (addr % 2) return -1;

    /* unlock if locked */
    unlock_flash_if(sl);

    /* set flash programming chosen bit */
    set_flash_cr_pg(sl);

    write_uint16(sl->q_buf, val);
    stlink_write_mem16(sl, addr, 2);

    /* wait for non business */
    wait_flash_busy(sl);

    lock_flash(sl);

    /* check the programmed value back */
    stlink_read_mem16(sl, addr, 2);
    if (*(const uint16_t*) sl->q_buf != val) {
        /* values differ at i * sizeof(uint16_t) */
        return -1;
    }

    /* success */
    return 0;
}
#endif /* not working */

// Exit the jtag or swd mode and enter the mass mode.

void _stlink_sg_exit_debug_mode(stlink_t *stl) {

    if (stl) {
        struct stlink_libsg* sl = stl->backend_data;
        clear_cdb(sl);
        sl->cdb_cmd_blk[1] = STLINK_DEBUG_EXIT;
        stl->q_len = 0; // >0 -> aboard
        stlink_q(stl);
    }
}


// 1) open a sg device, switch the stlink from dfu to mass mode
// 2) wait 5s until the kernel driver stops reseting the broken device
// 3) reopen the device
// 4) the device driver is now ready for a switch to jtag/swd mode
// TODO thinking, better error handling, wait until the kernel driver stops reseting the plugged-in device

stlink_backend_t _stlink_sg_backend = {
    _stlink_sg_close,
    _stlink_sg_exit_debug_mode,
    _stlink_sg_enter_swd_mode,
    _stlink_sg_enter_jtag_mode,
    _stlink_sg_exit_dfu_mode,
    _stlink_sg_core_id,
    _stlink_sg_reset,
    _stlink_sg_run,
    _stlink_sg_status,
    _stlink_sg_version,
    _stlink_sg_read_mem32,
    _stlink_sg_write_mem32,
    _stlink_sg_write_mem8,
    _stlink_sg_read_all_regs,
    _stlink_sg_read_reg,
    _stlink_sg_write_reg,
    _stlink_sg_step,
    _stlink_sg_current_mode,
    _stlink_sg_force_debug
};

stlink_t* stlink_open(const char *dev_name, const int verbose) {
    fprintf(stderr, "\n*** stlink_open [%s] ***\n", dev_name);
    int sg_fd = scsi_pt_open_device(dev_name, RDWR, verbose);
    if (sg_fd < 0) {
        fprintf(stderr, "error opening device: %s: %s\n", dev_name,
                safe_strerror(-sg_fd));
        return NULL;
    }

    stlink_t *sl = malloc(sizeof (stlink_t));
    struct stlink_libsg *slsg = malloc(sizeof (struct stlink_libsg));
    if (sl == NULL || slsg == NULL) {
        fprintf(stderr, "Couldn't malloc stlink and stlink_sg structures out of memory!\n");
        return NULL;
    }
    sl->verbose = verbose;
    sl->backend_data = slsg;
    sl->backend = &_stlink_sg_backend;

    slsg->sg_fd = sg_fd;
    sl->core_stat = STLINK_CORE_STAT_UNKNOWN;
    slsg->core_id = 0;
    slsg->q_addr = 0;
    clear_buf(sl);

    /* flash memory settings */
    sl->flash_base = STM32_FLASH_BASE;
    sl->flash_size = STM32_FLASH_SIZE;
    sl->flash_pgsz = STM32_FLASH_PGSZ;

    /* system memory */
    sl->sys_base = STM32_SYSTEM_BASE;
    sl->sys_size = STM32_SYSTEM_SIZE;

    /* sram memory settings */
    sl->sram_base = STM32_SRAM_BASE;
    sl->sram_size = STM32_SRAM_SIZE;

    return sl;
}



stlink_t* stlink_quirk_open(const char *dev_name, const int verbose) {

    stlink_t *sl = stlink_open(dev_name, verbose);
    if (sl == NULL) {
        fputs("Error: could not open stlink device\n", stderr);
        return NULL;
    }

    stlink_version(sl);
    struct stlink_libsg *sg = sl->backend_data;

    if ((sg->st_vid != USB_ST_VID) || (sg->stlink_pid != USB_STLINK_PID)) {
        fprintf(stderr, "Error: the device %s is not a stlink\n",
                dev_name);
        fprintf(stderr, "       VID: got %04x expect %04x \n",
                sg->st_vid, USB_ST_VID);
        fprintf(stderr, "       PID: got %04x expect %04x \n",
                sg->stlink_pid, USB_STLINK_PID);
        return NULL;
    }

    D(sl, "\n*** stlink_force_open ***\n");
    switch (stlink_current_mode(sl)) {
        case STLINK_DEV_MASS_MODE:
            return sl;
        case STLINK_DEV_DEBUG_MODE:
            // TODO go to mass?
            return sl;
    }
    DD(sl, "\n*** switch the stlink to mass mode ***\n");
    _stlink_sg_exit_dfu_mode(sl);
    // exit the dfu mode -> the device is gone
    DD(sl, "\n*** reopen the stlink device ***\n");
    delay(1000);
    stlink_close(sl);
    delay(5000);

    sl = stlink_open(dev_name, verbose);
    if (sl == NULL) {
        fputs("Error: could not open stlink device\n", stderr);
        return NULL;
    }
    // re-query device info
    stlink_version(sl);
    return sl;
}

static void __attribute__((unused)) mark_buf(stlink_t *sl) {
    clear_buf(sl);
    sl->q_buf[0] = 0x12;
    sl->q_buf[1] = 0x34;
    sl->q_buf[2] = 0x56;
    sl->q_buf[3] = 0x78;
    sl->q_buf[4] = 0x90;
    sl->q_buf[15] = 0x42;
    sl->q_buf[16] = 0x43;
    sl->q_buf[63] = 0x42;
    sl->q_buf[64] = 0x43;
    sl->q_buf[1024 * 6 - 1] = 0x42; //6kB
    sl->q_buf[1024 * 8 - 1] = 0x42; //8kB
}
