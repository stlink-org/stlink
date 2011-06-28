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

// sgutils2 (apt-get install libsgutils2-dev)
#include <scsi/sg_lib.h>
#include <scsi/sg_pt.h>

#include "stlink-hw.h"

static void D(struct stlink *sl, char *txt) {
	if (sl->verbose > 1)
		fputs(txt, stderr);
}

static void DD(struct stlink *sl, char *format, ...) {
	if (sl->verbose > 0) {
		va_list list;
		va_start(list, format);
		vfprintf(stderr, format, list);
		va_end(list);
	}
}

// Suspends execution of the calling process for
// (at least) ms milliseconds.
static void delay(int ms) {
	//fprintf(stderr, "*** wait %d ms\n", ms);
	usleep(1000 * ms);
}

// Endianness
// http://www.ibm.com/developerworks/aix/library/au-endianc/index.html
// const int i = 1;
// #define is_bigendian() ( (*(char*)&i) == 0 )
static inline unsigned int is_bigendian(void) {
	static volatile const unsigned int i = 1;
	return *(volatile const char*) &i == 0;
}

static void write_uint32(unsigned char* buf, uint32_t ui) {
	if (!is_bigendian()) { // le -> le (don't swap)
		buf[0] = ((unsigned char*) &ui)[0];
		buf[1] = ((unsigned char*) &ui)[1];
		buf[2] = ((unsigned char*) &ui)[2];
		buf[3] = ((unsigned char*) &ui)[3];
	} else {
		buf[0] = ((unsigned char*) &ui)[3];
		buf[1] = ((unsigned char*) &ui)[2];
		buf[2] = ((unsigned char*) &ui)[1];
		buf[3] = ((unsigned char*) &ui)[0];
	}
}

static void write_uint16(unsigned char* buf, uint16_t ui) {
	if (!is_bigendian()) { // le -> le (don't swap)
		buf[0] = ((unsigned char*) &ui)[0];
		buf[1] = ((unsigned char*) &ui)[1];
	} else {
		buf[0] = ((unsigned char*) &ui)[1];
		buf[1] = ((unsigned char*) &ui)[0];
	}
}

static uint32_t read_uint32(const unsigned char *c, const int pt) {
	uint32_t ui;
	char *p = (char *) &ui;

	if (!is_bigendian()) { // le -> le (don't swap)
		p[0] = c[pt];
		p[1] = c[pt + 1];
		p[2] = c[pt + 2];
		p[3] = c[pt + 3];
	} else {
		p[0] = c[pt + 3];
		p[1] = c[pt + 2];
		p[2] = c[pt + 1];
		p[3] = c[pt];
	}
	return ui;
}

static uint16_t read_uint16(const unsigned char *c, const int pt) {
	uint32_t ui;
	char *p = (char *) &ui;

	if (!is_bigendian()) { // le -> le (don't swap)
		p[0] = c[pt];
		p[1] = c[pt + 1];
	} else {
		p[0] = c[pt + 1];
		p[1] = c[pt];
	}
	return ui;
}

static void clear_cdb(struct stlink *sl) {
	for (int i = 0; i < sizeof(sl->cdb_cmd_blk); i++)
		sl->cdb_cmd_blk[i] = 0;
	// set default
	sl->cdb_cmd_blk[0] = STLINK_DEBUG_COMMAND;
	sl->q_data_dir = Q_DATA_IN;
}

// E.g. make the valgrind happy.
static void clear_buf(struct stlink *sl) {
	DD(sl, "*** clear_buf ***\n");
	for (int i = 0; i < sizeof(sl->q_buf); i++)
		sl->q_buf[i] = 0;

}

static struct stlink* stlink_open(const char *dev_name, const int verbose) {
	fprintf(stderr, "\n*** stlink_open [%s] ***\n", dev_name);
	int sg_fd = scsi_pt_open_device(dev_name, RDWR, verbose);
	if (sg_fd < 0) {
		fprintf(stderr, "error opening device: %s: %s\n", dev_name,
			safe_strerror(-sg_fd));
		return NULL;
	}

	struct stlink *sl = malloc(sizeof(struct stlink));
	if (sl == NULL) {
		fprintf(stderr, "struct stlink: out of memory\n");
		return NULL;
	}

	sl->sg_fd = sg_fd;
	sl->verbose = verbose;
	sl->core_stat = STLINK_CORE_STAT_UNKNOWN;
	sl->core_id = 0;
	sl->q_addr = 0;
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

// close the device, free the allocated memory
void stlink_close(struct stlink *sl) {
	D(sl, "\n*** stlink_close ***\n");
	if (sl) {
		scsi_pt_close_device(sl->sg_fd);
		free(sl);
	}
}

//TODO rewrite/cleanup, save the error in sl
static void stlink_confirm_inq(struct stlink *sl, struct sg_pt_base *ptvp) {
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
	if ((sl->verbose > 1) && (duration >= 0))
		DD(sl, "      duration=%d ms\n", duration);

	// XXX stlink fw sends broken residue, so ignore it and use the known q_len
	// "usb-storage quirks=483:3744:r"
	// forces residue to be ignored and calculated, but this causes aboard if
	// data_len = 0 and by some other data_len values.

	const int resid = get_scsi_pt_resid(ptvp);
	const int dsize = sl->q_len - resid;

	const int cat = get_scsi_pt_result_category(ptvp);
	char buf[512];
	unsigned int slen;

	switch (cat) {
	case SCSI_PT_RESULT_GOOD:
		if (sl->verbose && (resid > 0))
			DD(sl, "      notice: requested %d bytes but "
				"got %d bytes, ignore [broken] residue = %d\n",
				sl->q_len, dsize, resid);
		break;
	case SCSI_PT_RESULT_STATUS:
		if (sl->verbose) {
			sg_get_scsi_status_str(
				get_scsi_pt_status_response(ptvp), sizeof(buf),
				buf);
			DD(sl, "  scsi status: %s\n", buf);
		}
		return;
	case SCSI_PT_RESULT_SENSE:
		slen = get_scsi_pt_sense_len(ptvp);
		if (sl->verbose) {
			sg_get_sense_str("", sl->sense_buf, slen, (sl->verbose
				> 1), sizeof(buf), buf);
			DD(sl, "%s", buf);
		}
		if (sl->verbose && (resid > 0)) {
			if ((sl->verbose) || (sl->q_len > 0))
				DD(sl, "    requested %d bytes but "
					"got %d bytes\n", sl->q_len, dsize);
		}
		return;
	case SCSI_PT_RESULT_TRANSPORT_ERR:
		if (sl->verbose) {
			get_scsi_pt_transport_err_str(ptvp, sizeof(buf), buf);
			// http://tldp.org/HOWTO/SCSI-Generic-HOWTO/x291.html
			// These codes potentially come from the firmware on a host adapter
			// or from one of several hosts that an adapter driver controls.
			// The 'host_status' field has the following values:
			//	[0x07] Internal error detected in the host adapter.
			// This may not be fatal (and the command may have succeeded).
			DD(sl, "  transport: %s", buf);
		}
		return;
	case SCSI_PT_RESULT_OS_ERR:
		if (sl->verbose) {
			get_scsi_pt_os_err_str(ptvp, sizeof(buf), buf);
			DD(sl, "  os: %s", buf);
		}
		return;
	default:
		fprintf(stderr, "  unknown pass through result "
			"category (%d)\n", cat);
	}
}

static void stlink_q(struct stlink* sl) {
	DD(sl, "CDB[");
	for (int i = 0; i < CDB_SL; i++)
		DD(sl, " 0x%02x", (unsigned int) sl->cdb_cmd_blk[i]);
	DD(sl, "]\n");

	// Get control command descriptor of scsi structure,
	// (one object per command!!)
	struct sg_pt_base *ptvp = construct_scsi_pt_obj();
	if (NULL == ptvp) {
		fprintf(stderr, "construct_scsi_pt_obj: out of memory\n");
		return;
	}

	set_scsi_pt_cdb(ptvp, sl->cdb_cmd_blk, sizeof(sl->cdb_cmd_blk));

	// set buffer for sense (error information) data
	set_scsi_pt_sense(ptvp, sl->sense_buf, sizeof(sl->sense_buf));

	// Set a buffer to be used for data transferred from device
	if (sl->q_data_dir == Q_DATA_IN) {
		//clear_buf(sl);
		set_scsi_pt_data_in(ptvp, sl->q_buf, sl->q_len);
	} else {
		set_scsi_pt_data_out(ptvp, sl->q_buf, sl->q_len);
	}
	// Executes SCSI command (or at least forwards it to lower layers).
	sl->do_scsi_pt_err = do_scsi_pt(ptvp, sl->sg_fd, SG_TIMEOUT_SEC,
		sl->verbose);

	// check for scsi errors
	stlink_confirm_inq(sl, ptvp);
	// TODO recycle: clear_scsi_pt_obj(struct sg_pt_base * objp);
	destruct_scsi_pt_obj(ptvp);
}

static void stlink_print_data(struct stlink *sl) {
	if (sl->q_len <= 0 || sl->verbose < 2)
		return;
	if (sl->verbose > 2)
		fprintf(stdout, "data_len = %d 0x%x\n", sl->q_len, sl->q_len);

	for (uint32_t i = 0; i < sl->q_len; i++) {
		if (i % 16 == 0) {
			if (sl->q_data_dir == Q_DATA_OUT)
				fprintf(stdout, "\n<- 0x%08x ", sl->q_addr + i);
			else
				fprintf(stdout, "\n-> 0x%08x ", sl->q_addr + i);
		}
		fprintf(stdout, " %02x", (unsigned int) sl->q_buf[i]);
	}
	fputs("\n\n", stdout);
}

// TODO thinking, cleanup
static void stlink_parse_version(struct stlink *sl) {
	sl->st_vid = 0;
	sl->stlink_pid = 0;
	if (sl->q_len <= 0) {
		fprintf(stderr, "Error: could not parse the stlink version");
		return;
	}
	stlink_print_data(sl);
	uint32_t b0 = sl->q_buf[0]; //lsb
	uint32_t b1 = sl->q_buf[1];
	uint32_t b2 = sl->q_buf[2];
	uint32_t b3 = sl->q_buf[3];
	uint32_t b4 = sl->q_buf[4];
	uint32_t b5 = sl->q_buf[5]; //msb

	// b0 b1                       || b2 b3  | b4 b5
	// 4b        | 6b     | 6b     || 2B     | 2B
	// stlink_v  | jtag_v | swim_v || st_vid | stlink_pid

	sl->stlink_v = (b0 & 0xf0) >> 4;
	sl->jtag_v = ((b0 & 0x0f) << 2) | ((b1 & 0xc0) >> 6);
	sl->swim_v = b1 & 0x3f;
	sl->st_vid = (b3 << 8) | b2;
	sl->stlink_pid = (b5 << 8) | b4;

	if (sl->verbose < 2)
		return;

	DD(sl, "st vid         = 0x%04x (expect 0x%04x)\n",
		sl->st_vid, USB_ST_VID);
	DD(sl, "stlink pid     = 0x%04x (expect 0x%04x)\n",
		sl->stlink_pid, USB_STLINK_PID);
	DD(sl, "stlink version = 0x%x\n", sl->stlink_v);
	DD(sl, "jtag version   = 0x%x\n", sl->jtag_v);
	DD(sl, "swim version   = 0x%x\n", sl->swim_v);
	if (sl->jtag_v == 0)
		DD(sl,
			"    notice: the firmware doesn't support a jtag/swd interface\n");
	if (sl->swim_v == 0)
		DD(sl,
			"    notice: the firmware doesn't support a swim interface\n");

}

static int stlink_mode(struct stlink *sl) {
	if (sl->q_len <= 0)
		return STLINK_DEV_UNKNOWN_MODE;

	stlink_print_data(sl);

	switch (sl->q_buf[0]) {
	case STLINK_DEV_DFU_MODE:
		DD(sl, "stlink mode: dfu\n");
		return STLINK_DEV_DFU_MODE;
	case STLINK_DEV_DEBUG_MODE:
		DD(sl, "stlink mode: debug (jtag or swd)\n");
		return STLINK_DEV_DEBUG_MODE;
	case STLINK_DEV_MASS_MODE:
		DD(sl, "stlink mode: mass\n");
		return STLINK_DEV_MASS_MODE;
	}
	return STLINK_DEV_UNKNOWN_MODE;
}

static void stlink_stat(struct stlink *sl, char *txt) {
	if (sl->q_len <= 0)
		return;

	stlink_print_data(sl);

	switch (sl->q_buf[0]) {
	case STLINK_OK:
		DD(sl, "  %s: ok\n", txt);
		return;
	case STLINK_FALSE:
		DD(sl, "  %s: false\n", txt);
		return;
	default:
		DD(sl, "  %s: unknown\n", txt);
	}
}

static void stlink_core_stat(struct stlink *sl) {
	if (sl->q_len <= 0)
		return;

	stlink_print_data(sl);

	switch (sl->q_buf[0]) {
	case STLINK_CORE_RUNNING:
		sl->core_stat = STLINK_CORE_RUNNING;
		DD(sl, "  core status: running\n");
		return;
	case STLINK_CORE_HALTED:
		sl->core_stat = STLINK_CORE_HALTED;
		DD(sl, "  core status: halted\n");
		return;
	default:
		sl->core_stat = STLINK_CORE_STAT_UNKNOWN;
		fprintf(stderr, "  core status: unknown\n");
	}
}

void stlink_version(struct stlink *sl) {
	D(sl, "\n*** stlink_version ***\n");
	clear_cdb(sl);
	sl->cdb_cmd_blk[0] = STLINK_GET_VERSION;
	sl->q_len = 6;
	sl->q_addr = 0;
	stlink_q(sl);
	stlink_parse_version(sl);
}

// Get stlink mode:
// STLINK_DEV_DFU_MODE || STLINK_DEV_MASS_MODE || STLINK_DEV_DEBUG_MODE
// usb dfu             || usb mass             || jtag or swd
int stlink_current_mode(struct stlink *sl) {
	D(sl, "\n*** stlink_current_mode ***\n");
	clear_cdb(sl);
	sl->cdb_cmd_blk[0] = STLINK_GET_CURRENT_MODE;
	sl->q_len = 2;
	sl->q_addr = 0;
	stlink_q(sl);
	return stlink_mode(sl);
}

// Exit the mass mode and enter the swd debug mode.
void stlink_enter_swd_mode(struct stlink *sl) {
	D(sl, "\n*** stlink_enter_swd_mode ***\n");
	clear_cdb(sl);
	sl->cdb_cmd_blk[1] = STLINK_DEBUG_ENTER;
	sl->cdb_cmd_blk[2] = STLINK_DEBUG_ENTER_SWD;
	sl->q_len = 0; // >0 -> aboard
	stlink_q(sl);
}

// Exit the mass mode and enter the jtag debug mode.
// (jtag is disabled in the discovery's stlink firmware)
void stlink_enter_jtag_mode(struct stlink *sl) {
	D(sl, "\n*** stlink_enter_jtag_mode ***\n");
	clear_cdb(sl);
	sl->cdb_cmd_blk[1] = STLINK_DEBUG_ENTER;
	sl->cdb_cmd_blk[2] = STLINK_DEBUG_ENTER_JTAG;
	sl->q_len = 0;
	stlink_q(sl);
}

// Exit the jtag or swd mode and enter the mass mode.
void stlink_exit_debug_mode(struct stlink *sl) {
	D(sl, "\n*** stlink_exit_debug_mode ***\n");
	clear_cdb(sl);
	sl->cdb_cmd_blk[1] = STLINK_DEBUG_EXIT;
	sl->q_len = 0; // >0 -> aboard
	stlink_q(sl);
}

// XXX kernel driver performs reset, the device temporally disappears
static void stlink_exit_dfu_mode(struct stlink *sl) {
	D(sl, "\n*** stlink_exit_dfu_mode ***\n");
	clear_cdb(sl);
	sl->cdb_cmd_blk[0] = STLINK_DFU_COMMAND;
	sl->cdb_cmd_blk[1] = STLINK_DFU_EXIT;
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

void stlink_core_id(struct stlink *sl) {
	D(sl, "\n*** stlink_core_id ***\n");
	clear_cdb(sl);
	sl->cdb_cmd_blk[1] = STLINK_DEBUG_READCOREID;
	sl->q_len = 4;
	sl->q_addr = 0;
	stlink_q(sl);
	sl->core_id = read_uint32(sl->q_buf, 0);
	if (sl->verbose < 2)
		return;
	stlink_print_data(sl);
	DD(sl, "core_id = 0x%08x\n", sl->core_id);
}

// Arm-core reset -> halted state.
void stlink_reset(struct stlink *sl) {
	D(sl, "\n*** stlink_reset ***\n");
	clear_cdb(sl);
	sl->cdb_cmd_blk[1] = STLINK_DEBUG_RESETSYS;
	sl->q_len = 2;
	sl->q_addr = 0;
	stlink_q(sl);
	stlink_stat(sl, "core reset");
}

// Arm-core status: halted or running.
void stlink_status(struct stlink *sl) {
	D(sl, "\n*** stlink_status ***\n");
	clear_cdb(sl);
	sl->cdb_cmd_blk[1] = STLINK_DEBUG_GETSTATUS;
	sl->q_len = 2;
	sl->q_addr = 0;
	stlink_q(sl);
	stlink_core_stat(sl);
}

// Force the core into the debug mode -> halted state.
void stlink_force_debug(struct stlink *sl) {
	D(sl, "\n*** stlink_force_debug ***\n");
	clear_cdb(sl);
	sl->cdb_cmd_blk[1] = STLINK_DEBUG_FORCEDEBUG;
	sl->q_len = 2;
	sl->q_addr = 0;
	stlink_q(sl);
	stlink_stat(sl, "force debug");
}

// Read all arm-core registers.
void stlink_read_all_regs(struct stlink *sl) {
	D(sl, "\n*** stlink_read_all_regs ***\n");
	clear_cdb(sl);
	sl->cdb_cmd_blk[1] = STLINK_DEBUG_READALLREGS;
	sl->q_len = 84;
	sl->q_addr = 0;
	stlink_q(sl);
	stlink_print_data(sl);

	// 0-3 | 4-7 | ... | 60-63 | 64-67 | 68-71   | 72-75      | 76-79 | 80-83
	// r0  | r1  | ... | r15   | xpsr  | main_sp | process_sp | rw    | rw2
	for (int i = 0; i < 16; i++) {
		sl->reg.r[i] = read_uint32(sl->q_buf, 4 * i);
		if (sl->verbose > 1)
			DD(sl, "r%2d = 0x%08x\n", i, sl->reg.r[i]);
	}
	sl->reg.xpsr = read_uint32(sl->q_buf, 64);
	sl->reg.main_sp = read_uint32(sl->q_buf, 68);
	sl->reg.process_sp = read_uint32(sl->q_buf, 72);
	sl->reg.rw = read_uint32(sl->q_buf, 76);
	sl->reg.rw2 = read_uint32(sl->q_buf, 80);
	if (sl->verbose < 2)
		return;

	DD(sl, "xpsr       = 0x%08x\n", sl->reg.xpsr);
	DD(sl, "main_sp    = 0x%08x\n", sl->reg.main_sp);
	DD(sl, "process_sp = 0x%08x\n", sl->reg.process_sp);
	DD(sl, "rw         = 0x%08x\n", sl->reg.rw);
	DD(sl, "rw2        = 0x%08x\n", sl->reg.rw2);
}

// Read an arm-core register, the index must be in the range 0..20.
//  0  |  1  | ... |  15   |  16   |   17    |   18       |  19   |  20
// r0  | r1  | ... | r15   | xpsr  | main_sp | process_sp | rw    | rw2
void stlink_read_reg(struct stlink *sl, int r_idx) {
	D(sl, "\n*** stlink_read_reg");
	DD(sl, " (%d) ***\n", r_idx);

	if (r_idx > 20 || r_idx < 0) {
		fprintf(stderr, "Error: register index must be in [0..20]\n");
		return;
	}
	clear_cdb(sl);
	sl->cdb_cmd_blk[1] = STLINK_DEBUG_READREG;
	sl->cdb_cmd_blk[2] = r_idx;
	sl->q_len = 4;
	sl->q_addr = 0;
	stlink_q(sl);
	//  0  |  1  | ... |  15   |  16   |   17    |   18       |  19   |  20
	// 0-3 | 4-7 | ... | 60-63 | 64-67 | 68-71   | 72-75      | 76-79 | 80-83
	// r0  | r1  | ... | r15   | xpsr  | main_sp | process_sp | rw    | rw2
	stlink_print_data(sl);

	uint32_t r = read_uint32(sl->q_buf, 0);
	DD(sl, "r_idx (%2d) = 0x%08x\n", r_idx, r);

	switch (r_idx) {
	case 16:
		sl->reg.xpsr = r;
		break;
	case 17:
		sl->reg.main_sp = r;
		break;
	case 18:
		sl->reg.process_sp = r;
		break;
	case 19:
		sl->reg.rw = r; //XXX ?(primask, basemask etc.)
		break;
	case 20:
		sl->reg.rw2 = r; //XXX ?(primask, basemask etc.)
		break;
	default:
		sl->reg.r[r_idx] = r;
	}
}

// Write an arm-core register. Index:
//  0  |  1  | ... |  15   |  16   |   17    |   18       |  19   |  20
// r0  | r1  | ... | r15   | xpsr  | main_sp | process_sp | rw    | rw2
void stlink_write_reg(struct stlink *sl, uint32_t reg, int idx) {
	D(sl, "\n*** stlink_write_reg ***\n");
	clear_cdb(sl);
	sl->cdb_cmd_blk[1] = STLINK_DEBUG_WRITEREG;
	//   2: reg index
	// 3-6: reg content
	sl->cdb_cmd_blk[2] = idx;
	write_uint32(sl->cdb_cmd_blk + 3, reg);
	sl->q_len = 2;
	sl->q_addr = 0;
	stlink_q(sl);
	stlink_stat(sl, "write reg");
}

// Write a register of the debug module of the core.
// XXX ?(atomic writes)
// TODO test
void stlink_write_dreg(struct stlink *sl, uint32_t reg, uint32_t addr) {
	D(sl, "\n*** stlink_write_dreg ***\n");
	clear_cdb(sl);
	sl->cdb_cmd_blk[1] = STLINK_DEBUG_WRITEDEBUGREG;
	// 2-5: address of reg of the debug module
	// 6-9: reg content
	write_uint32(sl->cdb_cmd_blk + 2, addr);
	write_uint32(sl->cdb_cmd_blk + 6, reg);
	sl->q_len = 2;
	sl->q_addr = addr;
	stlink_q(sl);
	stlink_stat(sl, "write debug reg");
}

// Force the core exit the debug mode.
void stlink_run(struct stlink *sl) {
	D(sl, "\n*** stlink_run ***\n");
	clear_cdb(sl);
	sl->cdb_cmd_blk[1] = STLINK_DEBUG_RUNCORE;
	sl->q_len = 2;
	sl->q_addr = 0;
	stlink_q(sl);
	stlink_stat(sl, "run core");
}

// same as above with entrypoint.
static unsigned int is_core_halted(struct stlink*);
void stlink_run_at(struct stlink *sl, stm32_addr_t addr) {
	stlink_write_reg(sl, addr, 15); /* pc register */

	stlink_run(sl);

	while (is_core_halted(sl) == 0)
	  usleep(3000000);
}

// Step the arm-core.
void stlink_step(struct stlink *sl) {
	D(sl, "\n*** stlink_step ***\n");
	clear_cdb(sl);
	sl->cdb_cmd_blk[1] = STLINK_DEBUG_STEPCORE;
	sl->q_len = 2;
	sl->q_addr = 0;
	stlink_q(sl);
	stlink_stat(sl, "step core");
}

// TODO test
// see Cortex-M3 Technical Reference Manual
void stlink_set_hw_bp(struct stlink *sl, int fp_nr, uint32_t addr, int fp) {
	D(sl, "\n*** stlink_set_hw_bp ***\n");
	clear_cdb(sl);
	sl->cdb_cmd_blk[1] = STLINK_DEBUG_SETFP;
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
void stlink_clr_hw_bp(struct stlink *sl, int fp_nr) {
	D(sl, "\n*** stlink_clr_hw_bp ***\n");
	clear_cdb(sl);
	sl->cdb_cmd_blk[1] = STLINK_DEBUG_CLEARFP;
	sl->cdb_cmd_blk[2] = fp_nr;

	sl->q_len = 2;
	stlink_q(sl);
	stlink_stat(sl, "clear flash breakpoint");
}

// Read a "len" bytes to the sl->q_buf from the memory, max 6kB (6144 bytes)
void stlink_read_mem32(struct stlink *sl, uint32_t addr, uint16_t len) {
	D(sl, "\n*** stlink_read_mem32 ***\n");
	if (len % 4 != 0) { // !!! never ever: fw gives just wrong values
		fprintf(
			stderr,
			"Error: Data length doesn't have a 32 bit alignment: +%d byte.\n",
			len % 4);
		return;
	}
	clear_cdb(sl);
	sl->cdb_cmd_blk[1] = STLINK_DEBUG_READMEM_32BIT;
	// 2-5: addr
	// 6-7: len
	write_uint32(sl->cdb_cmd_blk + 2, addr);
	write_uint16(sl->cdb_cmd_blk + 6, len);

	// data_in 0-0x40-len
	// !!! len _and_ q_len must be max 6k,
	//     i.e. >1024 * 6 = 6144 -> aboard)
	// !!! if len < q_len: 64*k, 1024*n, n=1..5  -> aboard
	//     (broken residue issue)
	sl->q_len = len;
	sl->q_addr = addr;
	stlink_q(sl);
	stlink_print_data(sl);
}

// Write a "len" bytes from the sl->q_buf to the memory, max 64 Bytes.
void stlink_write_mem8(struct stlink *sl, uint32_t addr, uint16_t len) {
	D(sl, "\n*** stlink_write_mem8 ***\n");
	clear_cdb(sl);
	sl->cdb_cmd_blk[1] = STLINK_DEBUG_WRITEMEM_8BIT;
	// 2-5: addr
	// 6-7: len (>0x40 (64) -> aboard)
	write_uint32(sl->cdb_cmd_blk + 2, addr);
	write_uint16(sl->cdb_cmd_blk + 6, len);

	// data_out 0-len
	sl->q_len = len;
	sl->q_addr = addr;
	sl->q_data_dir = Q_DATA_OUT;
	stlink_q(sl);
	stlink_print_data(sl);
}

// Write a "len" bytes from the sl->q_buf to the memory, max Q_BUF_LEN bytes.
void stlink_write_mem32(struct stlink *sl, uint32_t addr, uint16_t len) {
	D(sl, "\n*** stlink_write_mem32 ***\n");
	if (len % 4 != 0) {
		fprintf(
			stderr,
			"Error: Data length doesn't have a 32 bit alignment: +%d byte.\n",
			len % 4);
		return;
	}
	clear_cdb(sl);
	sl->cdb_cmd_blk[1] = STLINK_DEBUG_WRITEMEM_32BIT;
	// 2-5: addr
	// 6-7: len "unlimited"
	write_uint32(sl->cdb_cmd_blk + 2, addr);
	write_uint16(sl->cdb_cmd_blk + 6, len);

	// data_out 0-0x40-...-len
	sl->q_len = len;
	sl->q_addr = addr;
	sl->q_data_dir = Q_DATA_OUT;
	stlink_q(sl);
	stlink_print_data(sl);
}

/* FPEC flash controller interface, pm0063 manual
 */

#define FLASH_REGS_ADDR 0x40022000
#define FLASH_REGS_SIZE 0x28

#define FLASH_ACR (FLASH_REGS_ADDR + 0x00)
#define FLASH_KEYR (FLASH_REGS_ADDR + 0x04)
#define FLASH_SR (FLASH_REGS_ADDR + 0x0c)
#define FLASH_CR (FLASH_REGS_ADDR + 0x10)
#define FLASH_AR (FLASH_REGS_ADDR + 0x14)
#define FLASH_OBR (FLASH_REGS_ADDR + 0x1c)
#define FLASH_WRPR (FLASH_REGS_ADDR + 0x20)

#define FLASH_RDPTR_KEY 0x00a5
#define FLASH_KEY1 0x45670123
#define FLASH_KEY2 0xcdef89ab

#define FLASH_SR_BSY 0
#define FLASH_SR_EOP 5

#define FLASH_CR_PG 0
#define FLASH_CR_PER 1
#define FLASH_CR_MER 2
#define FLASH_CR_STRT 6
#define FLASH_CR_LOCK 7

static uint32_t __attribute__((unused)) read_flash_rdp(struct stlink* sl)
{
  stlink_read_mem32(sl, FLASH_WRPR, sizeof(uint32_t));
  return (*(uint32_t*)sl->q_buf) & 0xff;
}

static inline uint32_t read_flash_wrpr(struct stlink* sl)
{
  stlink_read_mem32(sl, FLASH_WRPR, sizeof(uint32_t));
  return *(uint32_t*)sl->q_buf;
}

static inline uint32_t read_flash_obr(struct stlink* sl)
{
  stlink_read_mem32(sl, FLASH_OBR, sizeof(uint32_t));
  return *(uint32_t*)sl->q_buf;
}

static inline uint32_t read_flash_cr(struct stlink* sl)
{
  stlink_read_mem32(sl, FLASH_CR, sizeof(uint32_t));
  return *(uint32_t*)sl->q_buf;
}

static inline unsigned int is_flash_locked(struct stlink* sl)
{
  /* return non zero for true */
  return read_flash_cr(sl) & (1 << FLASH_CR_LOCK);
}

static void unlock_flash(struct stlink* sl)
{
  /* the unlock sequence consists of 2 write cycles where
     2 key values are written to the FLASH_KEYR register.
     an invalid sequence results in a definitive lock of
     the FPEC block until next reset.
  */

  write_uint32(sl->q_buf, FLASH_KEY1);
  stlink_write_mem32(sl, FLASH_KEYR, sizeof(uint32_t));

  write_uint32(sl->q_buf, FLASH_KEY2);
  stlink_write_mem32(sl, FLASH_KEYR, sizeof(uint32_t));
}

static int unlock_flash_if(struct stlink* sl)
{
  /* unlock flash if already locked */

  if (is_flash_locked(sl))
  {
    unlock_flash(sl);
    if (is_flash_locked(sl))
      return -1;
  }

  return 0;
}

static void lock_flash(struct stlink* sl)
{
  /* write to 1 only. reset by hw at unlock sequence */

  const uint32_t n = read_flash_cr(sl) | (1 << FLASH_CR_LOCK);

  write_uint32(sl->q_buf, n);
  stlink_write_mem32(sl, FLASH_CR, sizeof(uint32_t));
}

static void set_flash_cr_pg(struct stlink* sl)
{
  const uint32_t n = 1 << FLASH_CR_PG;
  write_uint32(sl->q_buf, n);
  stlink_write_mem32(sl, FLASH_CR, sizeof(uint32_t));
}

static void __attribute__((unused)) clear_flash_cr_pg(struct stlink* sl)
{
  const uint32_t n = read_flash_cr(sl) & ~(1 << FLASH_CR_PG);
  write_uint32(sl->q_buf, n);
  stlink_write_mem32(sl, FLASH_CR, sizeof(uint32_t));
}

static void set_flash_cr_per(struct stlink* sl)
{
  const uint32_t n = 1 << FLASH_CR_PER;
  write_uint32(sl->q_buf, n);
  stlink_write_mem32(sl, FLASH_CR, sizeof(uint32_t));
}

static void __attribute__((unused)) clear_flash_cr_per(struct stlink* sl)
{
  const uint32_t n = read_flash_cr(sl) & ~(1 << FLASH_CR_PER);
  write_uint32(sl->q_buf, n);
  stlink_write_mem32(sl, FLASH_CR, sizeof(uint32_t));
}

static void set_flash_cr_mer(struct stlink* sl)
{
  const uint32_t n = 1 << FLASH_CR_MER;
  write_uint32(sl->q_buf, n);
  stlink_write_mem32(sl, FLASH_CR, sizeof(uint32_t));
}

static void __attribute__((unused)) clear_flash_cr_mer(struct stlink* sl)
{
  const uint32_t n = read_flash_cr(sl) & ~(1 << FLASH_CR_MER);
  write_uint32(sl->q_buf, n);
  stlink_write_mem32(sl, FLASH_CR, sizeof(uint32_t));
}

static void set_flash_cr_strt(struct stlink* sl)
{
  /* assume come on the flash_cr_per path */
  const uint32_t n = (1 << FLASH_CR_PER) | (1 << FLASH_CR_STRT);
  write_uint32(sl->q_buf, n);
  stlink_write_mem32(sl, FLASH_CR, sizeof(uint32_t));
}

static inline uint32_t read_flash_acr(struct stlink* sl)
{
  stlink_read_mem32(sl, FLASH_ACR, sizeof(uint32_t));
  return *(uint32_t*)sl->q_buf;
}

static inline uint32_t read_flash_sr(struct stlink* sl)
{
  stlink_read_mem32(sl, FLASH_SR, sizeof(uint32_t));
  return *(uint32_t*)sl->q_buf;
}

static inline unsigned int is_flash_busy(struct stlink* sl)
{
  return read_flash_sr(sl) & (1 << FLASH_SR_BSY);
}

static void wait_flash_busy(struct stlink* sl)
{
  /* todo: add some delays here */
  while (is_flash_busy(sl))
    ;
}

static inline unsigned int is_flash_eop(struct stlink* sl)
{
  return read_flash_sr(sl) & (1 << FLASH_SR_EOP);
}

static void __attribute__((unused)) clear_flash_sr_eop(struct stlink* sl)
{
  const uint32_t n = read_flash_sr(sl) & ~(1 << FLASH_SR_EOP);
  write_uint32(sl->q_buf, n);
  stlink_write_mem32(sl, FLASH_SR, sizeof(uint32_t));
}

static void __attribute__((unused)) wait_flash_eop(struct stlink* sl)
{
  /* todo: add some delays here */
  while (is_flash_eop(sl) == 0)
    ;
}

static inline void write_flash_ar(struct stlink* sl, uint32_t n)
{
  write_uint32(sl->q_buf, n);
  stlink_write_mem32(sl, FLASH_AR, sizeof(uint32_t));
}

#if 0 /* todo */
static void disable_flash_read_protection(struct stlink* sl)
{
  /* erase the option byte area */
  /* rdp = 0x00a5; */
  /* reset */
}
#endif /* todo */

#if 0 /* not working */
static int write_flash_mem16
(struct stlink* sl, uint32_t addr, uint16_t val)
{
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
  if (*(const uint16_t*)sl->q_buf != val)
  {
    /* values differ at i * sizeof(uint16_t) */
    return -1;
  }

  /* success */
  return 0;
}
#endif /* not working */

int stlink_erase_flash_page(struct stlink* sl, stm32_addr_t page)
{
  /* page an addr in the page to erase */

  /* wait for ongoing op to finish */
  wait_flash_busy(sl);

  /* unlock if locked */
  unlock_flash_if(sl);

  /* set the page erase bit */
  set_flash_cr_per(sl);

  /* select the page to erase */
  write_flash_ar(sl, page);

  /* start erase operation, reset by hw with bsy bit */
  set_flash_cr_strt(sl);

  /* wait for completion */
  wait_flash_busy(sl);

  /* relock the flash */
  lock_flash(sl);

  /* todo: verify the erased page */

  return 0;
}

int stlink_erase_flash_mass(struct stlink* sl)
{
  /* wait for ongoing op to finish */
  wait_flash_busy(sl);

  /* unlock if locked */
  unlock_flash_if(sl);

  /* set the mass erase bit */
  set_flash_cr_mer(sl);

  /* start erase operation, reset by hw with bsy bit */
  set_flash_cr_strt(sl);

  /* wait for completion */
  wait_flash_busy(sl);

  /* relock the flash */
  lock_flash(sl);

  /* todo: verify the erased memory */

  return 0;
}

static unsigned int is_core_halted(struct stlink* sl)
{
  /* return non zero if core is halted */
  stlink_status(sl);
  return sl->q_buf[0] == STLINK_CORE_HALTED;
}

static int write_loader_to_sram
(struct stlink* sl, stm32_addr_t* addr, size_t* size)
{
  /* from openocd, contrib/loaders/flash/stm32.s */
  static const uint8_t loader_code[] =
  {
    0x08, 0x4c,			/* ldr	r4, STM32_FLASH_BASE */
    0x1c, 0x44,			/* add	r4, r3 */
    /* write_half_word: */
    0x01, 0x23,			/* movs	r3, #0x01 */
    0x23, 0x61,			/* str	r3, [r4, #STM32_FLASH_CR_OFFSET] */
    0x30, 0xf8, 0x02, 0x3b,	/* ldrh	r3, [r0], #0x02 */
    0x21, 0xf8, 0x02, 0x3b,	/* strh	r3, [r1], #0x02 */
    /* busy: */
    0xe3, 0x68,			/* ldr	r3, [r4, #STM32_FLASH_SR_OFFSET] */
    0x13, 0xf0, 0x01, 0x0f,	/* tst	r3, #0x01 */
    0xfb, 0xd0,			/* beq	busy */
    0x13, 0xf0, 0x14, 0x0f,	/* tst	r3, #0x14 */
    0x01, 0xd1,			/* bne	exit */
    0x01, 0x3a,			/* subs	r2, r2, #0x01 */
    0xf0, 0xd1,			/* bne	write_half_word */
    /* exit: */
    0x00, 0xbe,			/* bkpt	#0x00 */
    0x00, 0x20, 0x02, 0x40,	/* STM32_FLASH_BASE: .word 0x40022000 */
  };

  memcpy(sl->q_buf, loader_code, sizeof(loader_code));
  stlink_write_mem32(sl, sl->sram_base, sizeof(loader_code));

  *addr = sl->sram_base;
  *size = sizeof(loader_code);

  /* success */
  return 0;
}

typedef struct flash_loader
{
  stm32_addr_t loader_addr; /* loader sram adddr */
  stm32_addr_t buf_addr; /* buffer sram address */
} flash_loader_t;

static int write_buffer_to_sram
(struct stlink* sl, flash_loader_t* fl, const uint8_t* buf, size_t size)
{
  /* write the buffer right after the loader */
  memcpy(sl->q_buf, buf, size);
  stlink_write_mem8(sl, fl->buf_addr, size);
  return 0;
}

static int init_flash_loader
(struct stlink* sl, flash_loader_t* fl)
{
  size_t size;

  /* allocate the loader in sram */
  if (write_loader_to_sram(sl, &fl->loader_addr, &size) == -1)
  {
    fprintf(stderr, "write_loader_to_sram() == -1\n");
    return -1;
  }

  /* allocate a one page buffer in sram right after loader */
  fl->buf_addr = fl->loader_addr + size;

  return 0;
}

static int run_flash_loader
(struct stlink* sl, flash_loader_t* fl, stm32_addr_t target, const uint8_t* buf, size_t size)
{
  const size_t count = size / sizeof(uint16_t);

  if (write_buffer_to_sram(sl, fl, buf, size) == -1)
  {
    fprintf(stderr, "write_buffer_to_sram() == -1\n");
    return -1;
  }

  /* setup core */
  stlink_write_reg(sl, fl->buf_addr, 0); /* source */
  stlink_write_reg(sl, target, 1); /* target */
  stlink_write_reg(sl, count, 2); /* count (16 bits half words) */
  stlink_write_reg(sl, 0, 3); /* flash bank 0 (input) */
  stlink_write_reg(sl, fl->loader_addr, 15); /* pc register */

  /* unlock and set programming mode */
  unlock_flash_if(sl);
  set_flash_cr_pg(sl);

  /* run loader */
  stlink_run(sl);

  while (is_core_halted(sl) == 0)
    ;

  lock_flash(sl);

  /* not all bytes have been written */
  stlink_read_reg(sl, 2);
  if (sl->reg.r[2] != 0)
  {
    fprintf(stderr, "write error, count == %u\n", sl->reg.r[2]);
    return -1;
  }

  return 0;
}

/* memory mapped file */

typedef struct mapped_file
{
  uint8_t* base;
  size_t len;
} mapped_file_t;

#define MAPPED_FILE_INITIALIZER { NULL, 0 }

static int map_file(mapped_file_t* mf, const char* path)
{
  int error = -1;
  struct stat st;

  const int fd = open(path, O_RDONLY);
  if (fd == -1)
  {
    fprintf(stderr, "open(%s) == -1\n", path);
    return -1;
  }

  if (fstat(fd, &st) == -1)
  {
    fprintf(stderr, "fstat() == -1\n");
    goto on_error;
  }

  mf->base = (uint8_t*)mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if (mf->base == MAP_FAILED)
  {
    fprintf(stderr, "mmap() == MAP_FAILED\n");
    goto on_error;
  }

  mf->len = st.st_size;

  /* success */
  error = 0;

 on_error:
  close(fd);

  return error;
}

static void unmap_file(mapped_file_t* mf)
{
  munmap((void*)mf->base, mf->len);
  mf->base = (unsigned char*)MAP_FAILED;
  mf->len = 0;
}

static int check_file
(struct stlink* sl, mapped_file_t* mf, stm32_addr_t addr)
{
  size_t off;

  for (off = 0; off < mf->len; off += sl->flash_pgsz)
  {
    size_t aligned_size;

    /* adjust last page size */
    size_t cmp_size = sl->flash_pgsz;
    if ((off + sl->flash_pgsz) > mf->len)
      cmp_size = mf->len - off;

    aligned_size = cmp_size;
    if (aligned_size & (4 - 1))
      aligned_size = (cmp_size + 4) & ~(4 - 1);

    stlink_read_mem32(sl, addr + off, aligned_size);

    if (memcmp(sl->q_buf, mf->base + off, cmp_size))
      return -1;
  }

  return 0;
}

static int stlink_fcheck_flash
(struct stlink* sl, const char* path, stm32_addr_t addr)
{
  /* check the contents of path are at addr */

  int res;
  mapped_file_t mf = MAPPED_FILE_INITIALIZER;

  if (map_file(&mf, path) == -1)
    return -1;

  res = check_file(sl, &mf, addr);

  unmap_file(&mf);

  return res;
}

// The stlink_fwrite_flash should not muck with mmapped files inside itself,
// and should use this function instead. (Hell, what's the reason behind mmap
// there?!) But, as it is not actually used anywhere, nobody cares.

#define WRITE_BLOCK_SIZE 0x40
int stlink_write_flash(struct stlink* sl, stm32_addr_t addr, uint8_t* base, unsigned len) {
  int error = -1;
  size_t off;
  flash_loader_t fl;

  /* check addr range is inside the flash */
  if (addr < sl->flash_base) {
    fprintf(stderr, "addr too low\n");
    return -1;
  } else if ((addr + len) < addr) {
    fprintf(stderr, "addr overruns\n");
    return -1;
  } else if ((addr + len) > (sl->flash_base + sl->flash_size)) {
    fprintf(stderr, "addr too high\n");
    return -1;
  } else if ((addr & 1) || (len & 1)) {
    fprintf(stderr, "unaligned addr or size\n");
    return -1;
  }

  /* flash loader initialization */
  if (init_flash_loader(sl, &fl) == -1) {
    fprintf(stderr, "init_flash_loader() == -1\n");
    return -1;
  }

  /* write each page. above WRITE_BLOCK_SIZE fails? */
  for (off = 0; off < len; off += WRITE_BLOCK_SIZE) {
    /* adjust last write size */
    size_t size = WRITE_BLOCK_SIZE;
    if((off + WRITE_BLOCK_SIZE) > len)
      size = len - off;

    if(run_flash_loader(sl, &fl, addr + off, base + off, size) == -1) {
      fprintf(stderr, "run_flash_loader(0x%zx) == -1\n", addr + off);
      return -1;
    }
  }

  for(off = 0; off < len; off += sl->flash_pgsz) {
    size_t aligned_size;

    /* adjust last page size */
    size_t cmp_size = sl->flash_pgsz;
    if ((off + sl->flash_pgsz) > len)
      cmp_size = len - off;

    aligned_size = cmp_size;
    if (aligned_size & (4 - 1))
      aligned_size = (cmp_size + 4) & ~(4 - 1);

    stlink_read_mem32(sl, addr + off, aligned_size);

    if (memcmp(sl->q_buf, base + off, cmp_size))
      return -1;
  }

  return 0;
}

static int stlink_fwrite_flash
(struct stlink* sl, const char* path, stm32_addr_t addr)
{
  /* write the file in flash at addr */

  int error = -1;
  size_t off;
  mapped_file_t mf = MAPPED_FILE_INITIALIZER;
  flash_loader_t fl;

  if (map_file(&mf, path) == -1)
  {
    fprintf(stderr, "map_file() == -1\n");
    return -1;
  }

  /* check addr range is inside the flash */
  if (addr < sl->flash_base)
  {
    fprintf(stderr, "addr too low\n");
    goto on_error;
  }
  else if ((addr + mf.len) < addr)
  {
    fprintf(stderr, "addr overruns\n");
    goto on_error;
  }
  else if ((addr + mf.len) > (sl->flash_base + sl->flash_size))
  {
    fprintf(stderr, "addr too high\n");
    goto on_error;
  }
  else if ((addr & 1) || (mf.len & 1))
  {
    /* todo */
    fprintf(stderr, "unaligned addr or size\n");
    goto on_error;
  }

  /* erase each page. todo: mass erase faster? */
  for (off = 0; off < mf.len; off += sl->flash_pgsz)
  {
    /* addr must be an addr inside the page */
    if (stlink_erase_flash_page(sl, addr + off) == -1)
    {
      fprintf(stderr, "erase_flash_page(0x%zx) == -1\n", addr + off);
      goto on_error;
    }
  }

  /* flash loader initialization */
  if (init_flash_loader(sl, &fl) == -1)
  {
    fprintf(stderr, "init_flash_loader() == -1\n");
    goto on_error;
  }

  /* write each page. above WRITE_BLOCK_SIZE fails? */
#define WRITE_BLOCK_SIZE 0x40
  for (off = 0; off < mf.len; off += WRITE_BLOCK_SIZE)
  {
    /* adjust last write size */
    size_t size = WRITE_BLOCK_SIZE;
    if ((off + WRITE_BLOCK_SIZE) > mf.len)
      size = mf.len - off;

    if (run_flash_loader(sl, &fl, addr + off, mf.base + off, size) == -1)
    {
      fprintf(stderr, "run_flash_loader(0x%zx) == -1\n", addr + off);
      goto on_error;
    }
  }

  /* check the file ha been written */
  if (check_file(sl, &mf, addr) == -1)
  {
    fprintf(stderr, "check_file() == -1\n");
    goto on_error;
  }

  /* success */
  error = 0;

 on_error:
  unmap_file(&mf);
  return error;
}

static int stlink_fwrite_sram
(struct stlink* sl, const char* path, stm32_addr_t addr)
{
  /* write the file in sram at addr */

  int error = -1;
  size_t off;
  mapped_file_t mf = MAPPED_FILE_INITIALIZER;

  if (map_file(&mf, path) == -1)
  {
    fprintf(stderr, "map_file() == -1\n");
    return -1;
  }

  /* check addr range is inside the sram */
  if (addr < sl->sram_base)
  {
    fprintf(stderr, "addr too low\n");
    goto on_error;
  }
  else if ((addr + mf.len) < addr)
  {
    fprintf(stderr, "addr overruns\n");
    goto on_error;
  }
  else if ((addr + mf.len) > (sl->sram_base + sl->sram_size))
  {
    fprintf(stderr, "addr too high\n");
    goto on_error;
  }
  else if ((addr & 3) || (mf.len & 3))
  {
    /* todo */
    fprintf(stderr, "unaligned addr or size\n");
    goto on_error;
  }

  /* do the copy by 1k blocks */
  for (off = 0; off < mf.len; off += 1024)
  {
    size_t size = 1024;
    if ((off + size) > mf.len)
      size = mf.len - off;

    memcpy(sl->q_buf, mf.base + off, size);

    /* round size if needed */
    if (size & 3)
      size += 2;

    stlink_write_mem32(sl, addr + off, size);
  }

  /* check the file ha been written */
  if (check_file(sl, &mf, addr) == -1)
  {
    fprintf(stderr, "check_file() == -1\n");
    goto on_error;
  }

  /* success */
  error = 0;

 on_error:
  unmap_file(&mf);
  return error;
}


static int stlink_fread
(struct stlink* sl, const char* path, stm32_addr_t addr, size_t size)
{
  /* read size bytes from addr to file */

  int error = -1;
  size_t off;

  const int fd = open(path, O_RDWR | O_TRUNC | O_CREAT, 00700);
  if (fd == -1)
  {
    fprintf(stderr, "open(%s) == -1\n", path);
    return -1;
  }

  /* do the copy by 1k blocks */
  for (off = 0; off < size; off += 1024)
  {
    size_t read_size = 1024;
    if ((off + read_size) > size)
      read_size = off + read_size;

    /* round size if needed */
    if (read_size & 3)
      read_size = (read_size + 4) & ~(3);

    stlink_read_mem32(sl, addr + off, read_size);

    if (write(fd, sl->q_buf, read_size) != (ssize_t)read_size)
    {
      fprintf(stderr, "write() != read_size\n");
      goto on_error;
    }
  }

  /* success */
  error = 0;

 on_error:
  close(fd);

  return error;
}

// 1) open a sg device, switch the stlink from dfu to mass mode
// 2) wait 5s until the kernel driver stops reseting the broken device
// 3) reopen the device
// 4) the device driver is now ready for a switch to jtag/swd mode
// TODO thinking, better error handling, wait until the kernel driver stops reseting the plugged-in device
struct stlink* stlink_quirk_open(const char *dev_name, const int verbose) {
	struct stlink *sl = stlink_open(dev_name, verbose);
	if (sl == NULL) {
		fputs("Error: could not open stlink device\n", stderr);
		return NULL;
	}

	stlink_version(sl);

	if (sl->st_vid != USB_ST_VID || sl->stlink_pid != USB_STLINK_PID) {
		fprintf(stderr, "Error: the device %s is not a stlink\n",
			dev_name);
		fprintf(stderr, "       VID: got %04x expect %04x \n",
			sl->st_vid, USB_ST_VID);
		fprintf(stderr, "       PID: got %04x expect %04x \n",
			sl->stlink_pid, USB_STLINK_PID);
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
	stlink_exit_dfu_mode(sl);
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

static void __attribute__((unused)) mark_buf(struct stlink *sl) {
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

#if 0
int main(int argc, char *argv[]) {
	// set scpi lib debug level: 0 for no debug info, 10 for lots
	const int scsi_verbose = 2;
	char *dev_name;

	switch (argc) {
	case 1:
		fputs(
			"\nUsage: stlink-access-test /dev/sg0, sg1, ...\n"
				"\n*** Notice: The stlink firmware violates the USB standard.\n"
				"*** If you plug-in the discovery's stlink, wait a several\n"
				"*** minutes to let the kernel driver swallow the broken device.\n"
				"*** Watch:\ntail -f /var/log/messages\n"
				"*** This command sequence can shorten the waiting time and fix some issues.\n"
				"*** Unplug the stlink and execute once as root:\n"
				"modprobe -r usb-storage && modprobe usb-storage quirks=483:3744:lrwsro\n\n",
			stderr);
		return EXIT_FAILURE;
	case 2:
		dev_name = argv[1];
		break;
	default:
		return EXIT_FAILURE;
	}

	fputs("*** stlink access test ***\n", stderr);
	DD(sl, "Using sg_lib %s : scsi_pt %s\n", sg_lib_version(),
		scsi_pt_version());

	struct stlink *sl = stlink_force_open(dev_name, scsi_verbose);
	if (sl == NULL)
		return EXIT_FAILURE;

	// we are in mass mode, go to swd
	stlink_enter_swd_mode(sl);
	stlink_current_mode(sl);
	stlink_core_id(sl);
	//----------------------------------------------------------------------

	stlink_status(sl);
	//stlink_force_debug(sl);
	stlink_reset(sl);
	stlink_status(sl);
#if 0
	// core system control block
	stlink_read_mem32(sl, 0xe000ed00, 4);
	DD(sl, "cpu id base register: SCB_CPUID = got 0x%08x expect 0x411fc231", read_uint32(sl->q_buf, 0));
	// no MPU
	stlink_read_mem32(sl, 0xe000ed90, 4);
	DD(sl, "mpu type register: MPU_TYPER = got 0x%08x expect 0x0", read_uint32(sl->q_buf, 0));

	stlink_read_mem32(sl, 0xe000edf0, 4);
	DD(sl, "DHCSR = 0x%08x", read_uint32(sl->q_buf, 0));

	stlink_read_mem32(sl, 0x4001100c, 4);
	DD(sl, "GPIOC_ODR = 0x%08x", read_uint32(sl->q_buf, 0));
#endif
#if 0
	// happy new year 2011: let blink all the leds
	// see "RM0041 Reference manual - STM32F100xx advanced ARM-based 32-bit MCUs"

#define GPIOC		0x40011000 // port C
#define GPIOC_CRH	(GPIOC + 0x04) // port configuration register high
#define GPIOC_ODR	(GPIOC + 0x0c) // port output data register
#define LED_BLUE	(1<<8) // pin 8
#define LED_GREEN	(1<<9) // pin 9
	stlink_read_mem32(sl, GPIOC_CRH, 4);
	uint32_t io_conf = read_uint32(sl->q_buf, 0);
	DD(sl, "GPIOC_CRH = 0x%08x", io_conf);

	// set: general purpose output push-pull, output mode, max speed 10 MHz.
	write_uint32(sl->q_buf, 0x44444411);
	stlink_write_mem32(sl, GPIOC_CRH, 4);

	clear_buf(sl);
	for (int i = 0; i < 100; i++) {
		write_uint32(sl->q_buf, LED_BLUE | LED_GREEN);
		stlink_write_mem32(sl, GPIOC_ODR, 4);
		/* stlink_read_mem32(sl, 0x4001100c, 4); */
		/* DD(sl, "GPIOC_ODR = 0x%08x", read_uint32(sl->q_buf, 0)); */
		delay(100);

		clear_buf(sl);
		stlink_write_mem32(sl, GPIOC_ODR, 4); // PC lo
		delay(100);
	}
	write_uint32(sl->q_buf, io_conf); // set old state

#endif
#if 0
	// TODO rtfm: stlink doesn't have flash write routines
	// writing to the flash area confuses the fw for the next read access

	//stlink_read_mem32(sl, 0, 1024*6);
	// flash 0x08000000 128kB
	fputs("++++++++++ read a flash at 0x0800 0000\n", stderr);
	stlink_read_mem32(sl, 0x08000000, 1024 * 6); //max 6kB
	clear_buf(sl);
	stlink_read_mem32(sl, 0x08000c00, 5);
	stlink_read_mem32(sl, 0x08000c00, 4);
	mark_buf(sl);
	stlink_write_mem32(sl, 0x08000c00, 4);
	stlink_read_mem32(sl, 0x08000c00, 256);
	stlink_read_mem32(sl, 0x08000c00, 256);
#endif
#if 0
	// sram 0x20000000 8kB
	fputs("\n++++++++++ read/write 8bit, sram at 0x2000 0000 ++++++++++++++++\n\n", stderr);
	clear_buf(sl);
	stlink_write_mem8(sl, 0x20000000, 16);

	mark_buf(sl);
	stlink_write_mem8(sl, 0x20000000, 1);
	stlink_write_mem8(sl, 0x20000001, 1);
	stlink_write_mem8(sl, 0x2000000b, 3);
	stlink_read_mem32(sl, 0x20000000, 16);
#endif
#if 0
	// a not aligned mem32 access doesn't work indeed
	fputs("\n++++++++++ read/write 32bit, sram at 0x2000 0000 ++++++++++++++++\n\n", stderr);
	clear_buf(sl);
	stlink_write_mem8(sl, 0x20000000, 32);

	mark_buf(sl);
	stlink_write_mem32(sl, 0x20000000, 1);
	stlink_read_mem32(sl, 0x20000000, 16);
	mark_buf(sl);
	stlink_write_mem32(sl, 0x20000001, 1);
	stlink_read_mem32(sl, 0x20000000, 16);
	mark_buf(sl);
	stlink_write_mem32(sl, 0x2000000b, 3);
	stlink_read_mem32(sl, 0x20000000, 16);

	mark_buf(sl);
	stlink_write_mem32(sl, 0x20000000, 17);
	stlink_read_mem32(sl, 0x20000000, 32);
#endif
#if 0
	// sram 0x20000000 8kB
	fputs("++++++++++ read/write 32bit, sram at 0x2000 0000 ++++++++++++\n", stderr);
	mark_buf(sl);
	stlink_write_mem8(sl, 0x20000000, 64);
	stlink_read_mem32(sl, 0x20000000, 64);

	mark_buf(sl);
	stlink_write_mem32(sl, 0x20000000, 1024 * 8); //8kB
	stlink_read_mem32(sl, 0x20000000, 1024 * 6);
	stlink_read_mem32(sl, 0x20000000 + 1024 * 6, 1024 * 2);
#endif
#if 0
	stlink_read_all_regs(sl);
	stlink_step(sl);
	fputs("++++++++++ write r0 = 0x12345678\n", stderr);
	stlink_write_reg(sl, 0x12345678, 0);
	stlink_read_reg(sl, 0);
	stlink_read_all_regs(sl);
#endif
#if 0
	stlink_run(sl);
	stlink_status(sl);

	stlink_force_debug(sl);
	stlink_status(sl);
#endif
#if 1 /* read the system bootloader */
	fputs("\n++++++++++ reading bootloader ++++++++++++++++\n\n", stderr);
	stlink_fread(sl, "/tmp/barfoo", sl->sys_base, sl->sys_size);
#endif
#if 0 /* read the flash memory */
	fputs("\n+++++++ read flash memory\n\n", stderr);
	/* mark_buf(sl); */
	stlink_read_mem32(sl, 0x08000000, 4);
#endif
#if 0 /* flash programming */
	fputs("\n+++++++ program flash memory\n\n", stderr);
	stlink_fwrite_flash(sl, "/tmp/foobar", 0x08000000);
#endif
#if 0 /* check file contents */
	fputs("\n+++++++ check flash memory\n\n", stderr);
	{
	  const int res = stlink_fcheck_flash(sl, "/tmp/foobar", 0x08000000);
	  printf("_____ stlink_fcheck_flash() == %d\n", res);
	}
#endif
#if 0
	fputs("\n+++++++ sram write and execute\n\n", stderr);
	stlink_fwrite_sram(sl, "/tmp/foobar", sl->sram_base);
	stlink_run_at(sl, sl->sram_base);
#endif

	stlink_run(sl);
	stlink_status(sl);
	//----------------------------------------------------------------------
	// back to mass mode, just in case ...
	stlink_exit_debug_mode(sl);
	stlink_current_mode(sl);
	stlink_close(sl);

	//fflush(stderr); fflush(stdout);
	return EXIT_SUCCESS;
}
#endif
