/* 
 * File:   stlink-sg.h
 * Author: karl
 *
 * Created on October 1, 2011, 11:29 PM
 */

#ifndef STLINK_SG_H
#define	STLINK_SG_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#include <libusb-1.0/libusb.h>
#include "stlink-common.h"
    
        // device access
#define RDWR		0
#define RO		1
#define SG_TIMEOUT_SEC	1 // actually 1 is about 2 sec
    // Each CDB can be a total of 6, 10, 12, or 16 bytes, later version
    // of the SCSI standard also allow for variable-length CDBs (min. CDB is 6).
    // the stlink needs max. 10 bytes.
#define CDB_6		6
#define CDB_10		10
#define CDB_12		12
#define CDB_16		16

#define CDB_SL		10

    // Query data flow direction.
#define Q_DATA_OUT	0
#define Q_DATA_IN	1

    // The SCSI Request Sense command is used to obtain sense data
    // (error information) from a target device.
    // http://en.wikipedia.org/wiki/SCSI_Request_Sense_Command
#define SENSE_BUF_LEN		32



#if defined(CONFIG_USE_LIBUSB)
    struct stlink_libsg {
        int sg_fd;
        int do_scsi_pt_err;

        unsigned char cdb_cmd_blk[CDB_SL];

        int q_data_dir; // Q_DATA_IN, Q_DATA_OUT
        // the start of the query data in the device memory space
        uint32_t q_addr;

        // Sense (error information) data
        unsigned char sense_buf[SENSE_BUF_LEN];

        uint32_t st_vid;
        uint32_t stlink_pid;
        uint32_t stlink_v;
        uint32_t jtag_v;
        uint32_t swim_v;
        uint32_t core_id;

        reg reg;
    };
#else
    struct stlink_libsg {};
#endif

    stlink_t* stlink_quirk_open(const char *dev_name, const int verbose);

#ifdef	__cplusplus
}
#endif

#endif	/* STLINK_SG_H */

