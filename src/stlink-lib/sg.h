/* == nightwalker-87: TODO: CONTENT AND USE OF THIS SOURCE FILE IS TO BE VERIFIED (07.06.2023) == */

/*
 * File: sg.h
 *
 *
 */

#ifndef SG_H
#define SG_H

#include <stdint.h>

#include <stlink.h>

#include "libusb_settings.h"

/* Device access */
#define RDWR        0
#define RO      1
#define SG_TIMEOUT_SEC  1 // actually 1 is about 2 sec
#define SG_TIMEOUT_MSEC 3 * 1000

// Each CDB can be a total of 6, 10, 12, or 16 bytes, later version of the SCSI standard
// also allow for variable-length CDBs (min. CDB is 6). The stlink needs max. 10 bytes.
#define CDB_6       6
#define CDB_10      10
#define CDB_12      12
#define CDB_16      16

#define CDB_SL      10

/* Query data flow direction */
#define Q_DATA_OUT  0
#define Q_DATA_IN   1

// The SCSI Request Sense command is used to obtain sense data (error information) from
// a target device. (http://en.wikipedia.org/wiki/SCSI_Request_Sense_Command)
#define SENSE_BUF_LEN       32

struct stlink_libsg {
    libusb_context* libusb_ctx;
    libusb_device_handle *usb_handle;
    uint32_t ep_rep;
    uint32_t ep_req;

    int32_t sg_fd;
    int32_t do_scsi_pt_err;

    unsigned char cdb_cmd_blk[CDB_SL];

    int32_t q_data_dir; // Q_DATA_IN, Q_DATA_OUT
    // the start of the query data in the device memory space
    uint32_t q_addr;

    // Sense (error information) data
    // obsolete, this was fed to the scsi tools
    unsigned char sense_buf[SENSE_BUF_LEN];

    struct stlink_reg reg;
};

// static void clear_cdb(struct stlink_libsg *sl);
void _stlink_sg_close(stlink_t *sl);
// static int32_t get_usb_mass_storage_status(libusb_device_handle *handle, uint8_t endpoint, uint32_t *tag);
// static int32_t dump_CDB_command(uint8_t *cdb, uint8_t cdb_len);
int32_t send_usb_mass_storage_command(libusb_device_handle *handle, uint8_t endpoint_out, uint8_t *cdb, uint8_t cdb_length,
                                        uint8_t lun, uint8_t flags, uint32_t expected_rx_size);
// static void get_sense(libusb_device_handle *handle, uint8_t endpoint_in, uint8_t endpoint_out);
int32_t send_usb_data_only(libusb_device_handle *handle, unsigned char endpoint_out,
                       unsigned char endpoint_in, unsigned char *cbuf, uint32_t length);
int32_t stlink_q(stlink_t *sl);
void stlink_stat(stlink_t *stl, char *txt);
int32_t _stlink_sg_version(stlink_t *stl);
int32_t _stlink_sg_current_mode(stlink_t *stl);
int32_t _stlink_sg_enter_swd_mode(stlink_t *sl);
int32_t _stlink_sg_enter_jtag_mode(stlink_t *sl);
int32_t _stlink_sg_exit_dfu_mode(stlink_t *sl);
int32_t _stlink_sg_core_id(stlink_t *sl);
int32_t _stlink_sg_reset(stlink_t *sl);
int32_t _stlink_sg_jtag_reset(stlink_t *sl, int32_t value);
int32_t _stlink_sg_status(stlink_t *sl);
int32_t _stlink_sg_force_debug(stlink_t *sl);
int32_t _stlink_sg_read_all_regs(stlink_t *sl, struct stlink_reg *regp);
int32_t _stlink_sg_read_reg(stlink_t *sl, int32_t r_idx, struct stlink_reg *regp);
int32_t _stlink_sg_write_reg(stlink_t *sl, uint32_t reg, int32_t idx);
void stlink_write_dreg(stlink_t *sl, uint32_t reg, uint32_t addr);
int32_t _stlink_sg_run(stlink_t *sl, enum run_type type);
int32_t _stlink_sg_step(stlink_t *sl);
void stlink_set_hw_bp(stlink_t *sl, int32_t fp_nr, uint32_t addr, int32_t fp);
void stlink_clr_hw_bp(stlink_t *sl, int32_t fp_nr);
int32_t _stlink_sg_read_mem32(stlink_t *sl, uint32_t addr, uint16_t len);
int32_t _stlink_sg_write_mem8(stlink_t *sl, uint32_t addr, uint16_t len);
int32_t _stlink_sg_write_mem32(stlink_t *sl, uint32_t addr, uint16_t len);
int32_t _stlink_sg_write_debug32(stlink_t *sl, uint32_t addr, uint32_t data);
int32_t _stlink_sg_read_debug32(stlink_t *sl, uint32_t addr, uint32_t *data);
int32_t _stlink_sg_exit_debug_mode(stlink_t *stl);

// static stlink_backend_t _stlink_sg_backend = { };

// static stlink_t* stlink_open(const int32_t verbose);
stlink_t* stlink_v1_open_inner(const int32_t verbose);
stlink_t* stlink_v1_open(const int32_t verbose, int32_t reset);

#endif // SG_H
