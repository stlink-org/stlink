#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/types.h>
#include <libusb.h>

#include "stlink-common.h"
#include "stlink-usb.h"
#include "uglylogging.h"

#define LOG_TAG __FILE__
#define DLOG(format, args...)         ugly_log(UDEBUG, LOG_TAG, format, ## args)
#define ILOG(format, args...)         ugly_log(UINFO, LOG_TAG, format, ## args)
#define WLOG(format, args...)         ugly_log(UWARN, LOG_TAG, format, ## args)
#define fatal(format, args...)        ugly_log(UFATAL, LOG_TAG, format, ## args)

/* code from bsd timersub.h 
http://www.gnu-darwin.org/www001/src/ports/net/libevnet/work/libevnet-0.3.8/libnostd/bsd/sys/time/timersub.h.html
*/
#if !defined timersub
#define	timersub(a, b, r) do {					\
	(r)->tv_sec	= (a)->tv_sec - (b)->tv_sec;		\
	(r)->tv_usec	= (a)->tv_usec - (b)->tv_usec;		\
	if ((r)->tv_usec < 0) {					\
		--(r)->tv_sec;					\
		(r)->tv_usec += 1000000;			\
	}							\
} while (0)
#endif

enum SCSI_Generic_Direction {SG_DXFER_TO_DEV=0, SG_DXFER_FROM_DEV=0x80};

void _stlink_usb_close(stlink_t* sl) {
    struct stlink_libusb * const handle = sl->backend_data;
    // maybe we couldn't even get the usb device?
    if (handle != NULL) {
        if (handle->req_trans != NULL)
            libusb_free_transfer(handle->req_trans);

        if (handle->rep_trans != NULL)
            libusb_free_transfer(handle->rep_trans);

        if (handle->usb_handle != NULL) {
            libusb_close(handle->usb_handle);
        }

        libusb_exit(handle->libusb_ctx);
        free(handle);
    }
}


struct trans_ctx {
#define TRANS_FLAGS_IS_DONE (1 << 0)
#define TRANS_FLAGS_HAS_ERROR (1 << 1)
    volatile unsigned long flags;
};

#ifndef LIBUSB_CALL
# define LIBUSB_CALL
#endif

static void LIBUSB_CALL on_trans_done(struct libusb_transfer * trans) {
    struct trans_ctx * const ctx = trans->user_data;

    if (trans->status != LIBUSB_TRANSFER_COMPLETED)
        ctx->flags |= TRANS_FLAGS_HAS_ERROR;

    ctx->flags |= TRANS_FLAGS_IS_DONE;
}

int submit_wait(struct stlink_libusb *slu, struct libusb_transfer * trans) {
    struct timeval start;
    struct timeval now;
    struct timeval diff;
    struct trans_ctx trans_ctx;
    enum libusb_error error;

    trans_ctx.flags = 0;

    /* brief intrusion inside the libusb interface */
    trans->callback = on_trans_done;
    trans->user_data = &trans_ctx;

    if ((error = libusb_submit_transfer(trans))) {
        printf("libusb_submit_transfer(%d)\n", error);
        return -1;
    }

    gettimeofday(&start, NULL);

    while (trans_ctx.flags == 0) {
        struct timeval timeout;
        timeout.tv_sec = 3;
        timeout.tv_usec = 0;
        if (libusb_handle_events_timeout(slu->libusb_ctx, &timeout)) {
            printf("libusb_handle_events()\n");
            return -1;
        }

        gettimeofday(&now, NULL);
        timersub(&now, &start, &diff);
        if (diff.tv_sec >= 3) {
            printf("libusb_handle_events() timeout\n");
            return -1;
        }
    }

    if (trans_ctx.flags & TRANS_FLAGS_HAS_ERROR) {
        printf("libusb_handle_events() | has_error\n");
        return -1;
    }

    return 0;
}

ssize_t send_recv(struct stlink_libusb* handle, int terminate,
        unsigned char* txbuf, size_t txsize,
        unsigned char* rxbuf, size_t rxsize) {
    /* note: txbuf and rxbuf can point to the same area */
    int res = 0;

    libusb_fill_bulk_transfer(handle->req_trans, handle->usb_handle,
            handle->ep_req,
            txbuf, txsize,
            NULL, NULL,
            0
            );

    if (submit_wait(handle, handle->req_trans)) return -1;

    /* send_only */
    if (rxsize != 0) {

        /* read the response */
        
        libusb_fill_bulk_transfer(handle->rep_trans, handle->usb_handle,
                                  handle->ep_rep, rxbuf, rxsize, NULL, NULL, 0);
        
        if (submit_wait(handle, handle->rep_trans)) return -1;
        res = handle->rep_trans->actual_length;
    }
    
    if ((handle->protocoll == 1) && terminate) {
        /* Read the SG reply */
        unsigned char sg_buf[13];
        libusb_fill_bulk_transfer
            (handle->rep_trans, handle->usb_handle,
             handle->ep_rep, sg_buf, 13, NULL, NULL, 0);
        res = submit_wait(handle, handle->rep_trans);
        /* The STLink doesn't seem to evaluate the sequence number */
        handle->sg_transfer_idx++;
        if (res ) return -1;
    }

    return handle->rep_trans->actual_length;
}

static inline int send_only
(struct stlink_libusb* handle, int terminate,
 unsigned char* txbuf, size_t txsize) {
    return send_recv(handle, terminate, txbuf, txsize, NULL, 0);
}


static int fill_command
(stlink_t * sl, enum SCSI_Generic_Direction dir, uint32_t len) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const cmd = sl->c_buf;
    int i = 0;
    memset(cmd, 0, sizeof (sl->c_buf));
    if(slu->protocoll == 1) {
        cmd[i++] = 'U';
        cmd[i++] = 'S';
        cmd[i++] = 'B';
        cmd[i++] = 'C';
        write_uint32(&cmd[i], slu->sg_transfer_idx);
        write_uint32(&cmd[i + 4], len);
        i += 8;
        cmd[i++] = (dir == SG_DXFER_FROM_DEV)?0x80:0;
        cmd[i++] = 0; /* Logical unit */
        cmd[i++] = 0xa; /* Command length */
    }
    return i;
}

void _stlink_usb_version(stlink_t *sl) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const data = sl->q_buf;
    unsigned char* const cmd  = sl->c_buf;
    ssize_t size;
    uint32_t rep_len = 6;
    int i = fill_command(sl, SG_DXFER_FROM_DEV, rep_len);

    cmd[i++] = STLINK_GET_VERSION;

    size = send_recv(slu, 1, cmd, slu->cmd_len, data, rep_len);
    if (size == -1) {
        printf("[!] send_recv\n");
        return;
    }
}

int32_t _stlink_usb_target_voltage(stlink_t *sl) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const rdata = sl->q_buf;
    unsigned char* const cmd  = sl->c_buf;
    ssize_t size;
    uint32_t rep_len = 8;
    int i = fill_command(sl, SG_DXFER_FROM_DEV, rep_len);
    uint32_t factor, reading;
    int voltage;

    cmd[i++] = STLINK_GET_TARGET_VOLTAGE;

    size = send_recv(slu, 1, cmd, slu->cmd_len, rdata, rep_len);
    if (size == -1) {
        printf("[!] send_recv\n");
        return -1;
    } else if (size != 8) {
        printf("[!] wrong length\n");
        return -1;
    }

    factor = (rdata[3] << 24) | (rdata[2] << 16) | (rdata[1] << 8) | (rdata[0] << 0);
    reading = (rdata[7] << 24) | (rdata[6] << 16) | (rdata[5] << 8) | (rdata[4] << 0);
    voltage = 2400 * reading / factor;

    return voltage;
}

uint32_t _stlink_usb_read_debug32(stlink_t *sl, uint32_t addr) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const rdata = sl->q_buf;
    unsigned char* const cmd  = sl->c_buf;
    ssize_t size;
    const int rep_len = 8;
 
    int i = fill_command(sl, SG_DXFER_FROM_DEV, rep_len);
    cmd[i++] = STLINK_DEBUG_COMMAND;
    cmd[i++] = STLINK_JTAG_READDEBUG_32BIT;
    write_uint32(&cmd[i], addr);
    size = send_recv(slu, 1, cmd, slu->cmd_len, rdata, rep_len);
    if (size == -1) {
        printf("[!] send_recv\n");
        return 0;
    }
    return read_uint32(rdata, 4);
}

void _stlink_usb_write_debug32(stlink_t *sl, uint32_t addr, uint32_t data) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const rdata = sl->q_buf;
    unsigned char* const cmd  = sl->c_buf;
    ssize_t size;
    const int rep_len = 2;
 
    int i = fill_command(sl, SG_DXFER_FROM_DEV, rep_len);
    cmd[i++] = STLINK_DEBUG_COMMAND;
    cmd[i++] = STLINK_JTAG_WRITEDEBUG_32BIT;
    write_uint32(&cmd[i], addr);
    write_uint32(&cmd[i + 4], data);
    size = send_recv(slu, 1, cmd, slu->cmd_len, rdata, rep_len);
    if (size == -1) {
        printf("[!] send_recv\n");
        return;
    }
}

void _stlink_usb_write_mem32(stlink_t *sl, uint32_t addr, uint16_t len) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const data = sl->q_buf;
    unsigned char* const cmd  = sl->c_buf;

    int i = fill_command(sl, SG_DXFER_TO_DEV, len);
    cmd[i++] = STLINK_DEBUG_COMMAND;
    cmd[i++] = STLINK_DEBUG_WRITEMEM_32BIT;
    write_uint32(&cmd[i], addr);
    write_uint16(&cmd[i + 4], len);
    send_only(slu, 0, cmd, slu->cmd_len);

    send_only(slu, 1, data, len);
}

void _stlink_usb_write_mem8(stlink_t *sl, uint32_t addr, uint16_t len) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const data = sl->q_buf;
    unsigned char* const cmd  = sl->c_buf;

    int i = fill_command(sl, SG_DXFER_TO_DEV, 0);
    cmd[i++] = STLINK_DEBUG_COMMAND;
    cmd[i++] = STLINK_DEBUG_WRITEMEM_8BIT;
    write_uint32(&cmd[i], addr);
    write_uint16(&cmd[i + 4], len);
    send_only(slu, 0, cmd, slu->cmd_len);
    send_only(slu, 1, data, len);
}


int _stlink_usb_current_mode(stlink_t * sl) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const cmd  = sl->c_buf;
    unsigned char* const data = sl->q_buf;
    ssize_t size;
    int rep_len = 2;
    int i = fill_command(sl, SG_DXFER_FROM_DEV, rep_len);
    
    cmd[i++] = STLINK_GET_CURRENT_MODE;
    size = send_recv(slu, 1, cmd,  slu->cmd_len, data, rep_len);
    if (size == -1) {
        printf("[!] send_recv\n");
        return -1;
    }
    return sl->q_buf[0];
}

void _stlink_usb_core_id(stlink_t * sl) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const cmd  = sl->c_buf;
    unsigned char* const data = sl->q_buf;
    ssize_t size;
    int rep_len = 4;
    int i = fill_command(sl, SG_DXFER_FROM_DEV, rep_len);

    cmd[i++] = STLINK_DEBUG_COMMAND;
    cmd[i++] = STLINK_DEBUG_READCOREID;

    size = send_recv(slu, 1, cmd, slu->cmd_len, data, rep_len);
    if (size == -1) {
        printf("[!] send_recv\n");
        return;
    }

    sl->core_id = read_uint32(data, 0);
}

void _stlink_usb_status(stlink_t * sl) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const data = sl->q_buf;
    unsigned char* const cmd  = sl->c_buf;
    ssize_t size;
    int rep_len = 2;
    int i = fill_command(sl, SG_DXFER_FROM_DEV, rep_len);

    cmd[i++] = STLINK_DEBUG_COMMAND;
    cmd[i++] = STLINK_DEBUG_GETSTATUS;

    size = send_recv(slu, 1, cmd, slu->cmd_len, data, rep_len);
    if (size == -1) {
        printf("[!] send_recv\n");
        return;
    }
}

void _stlink_usb_force_debug(stlink_t *sl) {
    struct stlink_libusb *slu = sl->backend_data;
    unsigned char* const data = sl->q_buf;
    unsigned char* const cmd  = sl->c_buf;
    ssize_t size;
    int rep_len = 2;
    int i = fill_command(sl, SG_DXFER_FROM_DEV, rep_len);

    cmd[i++] = STLINK_DEBUG_COMMAND;
    cmd[i++] = STLINK_DEBUG_FORCEDEBUG;
    size = send_recv(slu, 1, cmd, slu->cmd_len, data, rep_len);
    if (size == -1) {
        printf("[!] send_recv\n");
        return;
    }
}

void _stlink_usb_enter_swd_mode(stlink_t * sl) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const cmd  = sl->c_buf;
    ssize_t size;
    const int rep_len = 0;
    int i = fill_command(sl, SG_DXFER_FROM_DEV, rep_len);

    cmd[i++] = STLINK_DEBUG_COMMAND;
    cmd[i++] = STLINK_DEBUG_ENTER;
    cmd[i++] = STLINK_DEBUG_ENTER_SWD;

    size = send_only(slu, 1, cmd, slu->cmd_len);
    if (size == -1) {
        printf("[!] send_recv\n");
        return;
    }
}

void _stlink_usb_exit_dfu_mode(stlink_t* sl) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const cmd = sl->c_buf;
    ssize_t size;
    int i = fill_command(sl, SG_DXFER_FROM_DEV, 0);

    cmd[i++] = STLINK_DFU_COMMAND;
    cmd[i++] = STLINK_DFU_EXIT;

    size = send_only(slu, 1, cmd, slu->cmd_len);
    if (size == -1) {
        printf("[!] send_recv\n");
        return;
    }
}

/**
 * TODO - not convinced this does anything...
 * @param sl
 */
void _stlink_usb_reset(stlink_t * sl) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const data = sl->q_buf;
    unsigned char* const cmd = sl->c_buf;
    ssize_t size;
    int rep_len = 2;
    int i = fill_command(sl, SG_DXFER_FROM_DEV, rep_len);

    cmd[i++] = STLINK_DEBUG_COMMAND;
    cmd[i++] = STLINK_DEBUG_RESETSYS;

    size = send_recv(slu, 1, cmd, slu->cmd_len, data, rep_len);
    if (size == -1) {
        printf("[!] send_recv\n");
        return;
    }
}


void _stlink_usb_jtag_reset(stlink_t * sl, int value) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const data = sl->q_buf;
    unsigned char* const cmd = sl->c_buf;
    ssize_t size;
    int rep_len = 2;
    int i = fill_command(sl, SG_DXFER_FROM_DEV, rep_len);

    cmd[i++] = STLINK_DEBUG_COMMAND;
    cmd[i++] = STLINK_JTAG_DRIVE_NRST;
    cmd[i++] = (value)?0:1;

    size = send_recv(slu, 1, cmd, slu->cmd_len, data, rep_len);
    if (size == -1) {
        printf("[!] send_recv\n");
        return;
    }
}


void _stlink_usb_step(stlink_t* sl) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const data = sl->q_buf;
    unsigned char* const cmd = sl->c_buf;
    ssize_t size;
    int rep_len = 2;
    int i = fill_command(sl, SG_DXFER_FROM_DEV, rep_len);

    cmd[i++] = STLINK_DEBUG_COMMAND;
    cmd[i++] = STLINK_DEBUG_STEPCORE;

    size = send_recv(slu, 1, cmd, slu->cmd_len, data, rep_len);
    if (size == -1) {
        printf("[!] send_recv\n");
        return;
    }
}

/**
 * This seems to do a good job of restarting things from the beginning?
 * @param sl
 */
void _stlink_usb_run(stlink_t* sl) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const data = sl->q_buf;
    unsigned char* const cmd = sl->c_buf;
    ssize_t size;
    int rep_len = 2;
    int i = fill_command(sl, SG_DXFER_FROM_DEV, rep_len);

    cmd[i++] = STLINK_DEBUG_COMMAND;
    cmd[i++] = STLINK_DEBUG_RUNCORE;

    size = send_recv(slu, 1, cmd, slu->cmd_len, data, rep_len);
    if (size == -1) {
        printf("[!] send_recv\n");
        return;
    }
}

void _stlink_usb_exit_debug_mode(stlink_t *sl) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const cmd = sl->c_buf;
    ssize_t size;
    int i = fill_command(sl, SG_DXFER_FROM_DEV, 0);

    cmd[i++] = STLINK_DEBUG_COMMAND;
    cmd[i++] = STLINK_DEBUG_EXIT;

    size = send_only(slu, 1, cmd, slu->cmd_len);
    if (size == -1) {
        printf("[!] send_only\n");
        return;
    }
}

void _stlink_usb_read_mem32(stlink_t *sl, uint32_t addr, uint16_t len) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const data = sl->q_buf;
    unsigned char* const cmd = sl->c_buf;
    ssize_t size;
    int i = fill_command(sl, SG_DXFER_FROM_DEV, len);

    cmd[i++] = STLINK_DEBUG_COMMAND;
    cmd[i++] = STLINK_DEBUG_READMEM_32BIT;
    write_uint32(&cmd[i], addr);
    write_uint16(&cmd[i + 4], len);

    size = send_recv(slu, 1, cmd, slu->cmd_len, data, len);
    if (size == -1) {
        printf("[!] send_recv\n");
        return;
    }

    sl->q_len = (size_t) size;

    stlink_print_data(sl);
}

void _stlink_usb_read_all_regs(stlink_t *sl, reg *regp) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const cmd = sl->c_buf;
    unsigned char* const data = sl->q_buf;
    ssize_t size;
    uint32_t rep_len = 84;
    int i = fill_command(sl, SG_DXFER_FROM_DEV, rep_len);

    cmd[i++] = STLINK_DEBUG_COMMAND;
    cmd[i++] = STLINK_DEBUG_READALLREGS;
    size = send_recv(slu, 1, cmd, slu->cmd_len, data, rep_len);
    if (size == -1) {
        printf("[!] send_recv\n");
        return;
    }
    sl->q_len = (size_t) size;
    stlink_print_data(sl);
    for(i=0; i<16; i++)
        regp->r[i]= read_uint32(sl->q_buf, i*4);
    regp->xpsr       = read_uint32(sl->q_buf, 64);
    regp->main_sp    = read_uint32(sl->q_buf, 68);
    regp->process_sp = read_uint32(sl->q_buf, 72);
    regp->rw         = read_uint32(sl->q_buf, 76);
    regp->rw2        = read_uint32(sl->q_buf, 80);
    if (sl->verbose < 2)
        return;

    DLOG("xpsr       = 0x%08x\n", read_uint32(sl->q_buf, 64));
    DLOG("main_sp    = 0x%08x\n", read_uint32(sl->q_buf, 68));
    DLOG("process_sp = 0x%08x\n", read_uint32(sl->q_buf, 72));
    DLOG("rw         = 0x%08x\n", read_uint32(sl->q_buf, 76));
    DLOG("rw2        = 0x%08x\n", read_uint32(sl->q_buf, 80));
}

void _stlink_usb_read_reg(stlink_t *sl, int r_idx, reg *regp) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const data = sl->q_buf;
    unsigned char* const cmd  = sl->c_buf;
    ssize_t size;
    uint32_t r;
    uint32_t rep_len = 4;
    int i = fill_command(sl, SG_DXFER_FROM_DEV, rep_len);

    cmd[i++] = STLINK_DEBUG_COMMAND;
    cmd[i++] = STLINK_DEBUG_READREG;
    cmd[i++] = (uint8_t) r_idx;
    size = send_recv(slu, 1, cmd, slu->cmd_len, data, rep_len);
    if (size == -1) {
        printf("[!] send_recv\n");
        return;
    }
    sl->q_len = (size_t) size;
    stlink_print_data(sl);
    r = read_uint32(sl->q_buf, 0);
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
        regp->rw = r; /* XXX ?(primask, basemask etc.) */
        break;
    case 20:
        regp->rw2 = r; /* XXX ?(primask, basemask etc.) */
        break;
    default:
        regp->r[r_idx] = r;
    }
}

/* See section C1.6 of the ARMv7-M Architecture Reference Manual */
void _stlink_usb_read_unsupported_reg(stlink_t *sl, int r_idx, reg *regp) {
    uint32_t r;

    sl->q_buf[0] = (unsigned char) r_idx;
    for (int i = 1; i < 4; i++) {
        sl->q_buf[i] = 0;
    }

    _stlink_usb_write_mem32(sl, DCRSR, 4);
    _stlink_usb_read_mem32(sl, DCRDR, 4);

    r = read_uint32(sl->q_buf, 0);
    DLOG("r_idx (%2d) = 0x%08x\n", r_idx, r);

    switch (r_idx) {
        case 0x14:
            regp->primask = (uint8_t) (r & 0xFF);
            regp->basepri = (uint8_t) ((r>>8) & 0xFF);
            regp->faultmask = (uint8_t) ((r>>16) & 0xFF);
            regp->control = (uint8_t) ((r>>24) & 0xFF);
            break;
        case 0x21:
            regp->fpscr = r;
            break;
        default:
            regp->s[r_idx - 0x40] = r;
            break;
    }
}

void _stlink_usb_read_all_unsupported_regs(stlink_t *sl, reg *regp) {
    _stlink_usb_read_unsupported_reg(sl, 0x14, regp);
    _stlink_usb_read_unsupported_reg(sl, 0x21, regp);

    for (int i = 0; i < 32; i++) {
        _stlink_usb_read_unsupported_reg(sl, 0x40+i, regp);
    }
}

/* See section C1.6 of the ARMv7-M Architecture Reference Manual */
void _stlink_usb_write_unsupported_reg(stlink_t *sl, uint32_t val, int r_idx, reg *regp) {
    if (r_idx >= 0x1C && r_idx <= 0x1F) { /* primask, basepri, faultmask, or control */
        /* These are held in the same register */
        _stlink_usb_read_unsupported_reg(sl, 0x14, regp);

        val = (uint8_t) (val>>24);

        switch (r_idx) {
            case 0x1C:  /* control */
                val = (((uint32_t) val) << 24) | (((uint32_t) regp->faultmask) << 16) | (((uint32_t) regp->basepri) << 8) | ((uint32_t) regp->primask);
                break;
            case 0x1D:  /* faultmask */
                val = (((uint32_t) regp->control) << 24) | (((uint32_t) val) << 16) | (((uint32_t) regp->basepri) << 8) | ((uint32_t) regp->primask);
                break;
            case 0x1E:  /* basepri */
                val = (((uint32_t) regp->control) << 24) | (((uint32_t) regp->faultmask) << 16) | (((uint32_t) val) << 8) | ((uint32_t) regp->primask);
                break;
            case 0x1F:  /* primask */
                val = (((uint32_t) regp->control) << 24) | (((uint32_t) regp->faultmask) << 16) | (((uint32_t) regp->basepri) << 8) | ((uint32_t) val);
                break;
        }

        r_idx = 0x14;
    }

    write_uint32(sl->q_buf, val);

    _stlink_usb_write_mem32(sl, DCRDR, 4);

    sl->q_buf[0] = (unsigned char) r_idx;
    sl->q_buf[1] = 0;
    sl->q_buf[2] = 0x01;
    sl->q_buf[3] = 0;

    _stlink_usb_write_mem32(sl, DCRSR, 4);
}

void _stlink_usb_write_reg(stlink_t *sl, uint32_t reg, int idx) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const data = sl->q_buf;
    unsigned char* const cmd  = sl->c_buf;
    ssize_t size;
    uint32_t rep_len = 2;
    int i = fill_command(sl, SG_DXFER_FROM_DEV, rep_len);

    cmd[i++] = STLINK_DEBUG_COMMAND;
    cmd[i++] = STLINK_DEBUG_WRITEREG;
    cmd[i++] = idx;
    write_uint32(&cmd[i], reg);
    size = send_recv(slu, 1, cmd, slu->cmd_len, data, rep_len);
    if (size == -1) {
        printf("[!] send_recv\n");
        return;
    }
    sl->q_len = (size_t) size;
    stlink_print_data(sl);
}

stlink_backend_t _stlink_usb_backend = {
    _stlink_usb_close,
    _stlink_usb_exit_debug_mode,
    _stlink_usb_enter_swd_mode,
    NULL,  // no enter_jtag_mode here...
    _stlink_usb_exit_dfu_mode,
    _stlink_usb_core_id,
    _stlink_usb_reset,
    _stlink_usb_jtag_reset,
    _stlink_usb_run,
    _stlink_usb_status,
    _stlink_usb_version,
    _stlink_usb_read_debug32,
    _stlink_usb_read_mem32,
    _stlink_usb_write_debug32,
    _stlink_usb_write_mem32,
    _stlink_usb_write_mem8,
    _stlink_usb_read_all_regs,
    _stlink_usb_read_reg,
    _stlink_usb_read_all_unsupported_regs,
    _stlink_usb_read_unsupported_reg,
    _stlink_usb_write_unsupported_reg,
    _stlink_usb_write_reg,
    _stlink_usb_step,
    _stlink_usb_current_mode,
    _stlink_usb_force_debug,
    _stlink_usb_target_voltage
};


stlink_t* stlink_open_usb(const int verbose, int reset) {
    stlink_t* sl = NULL;
    struct stlink_libusb* slu = NULL;
    int error = -1;
    libusb_device** devs = NULL;
    int config;

    sl = malloc(sizeof (stlink_t));
    slu = malloc(sizeof (struct stlink_libusb));
    if (sl == NULL) goto on_error;
    if (slu == NULL) goto on_error;
    memset(sl, 0, sizeof (stlink_t));
    memset(slu, 0, sizeof (struct stlink_libusb));

    ugly_init(verbose);
    sl->backend = &_stlink_usb_backend;
    sl->backend_data = slu;
    
    sl->core_stat = STLINK_CORE_STAT_UNKNOWN;

    if (libusb_init(&(slu->libusb_ctx))) {
        WLOG("failed to init libusb context, wrong version of libraries?\n");
        goto on_error;
    }
    
    libusb_device **list;
    int cnt = libusb_get_device_list(slu->libusb_ctx, &list);
    struct libusb_device_descriptor desc;
    int devBus =0;
    int devAddr=0;
    
    char *device = getenv("STLINK_DEVICE");
    if (device) {
        char *c = strchr(device,':');
        if (c==NULL) {
            WLOG("STLINK_DEVICE must be <USB_BUS>:<USB_ADDR> format\n");
            goto on_error;
        }
        devBus=atoi(device);
        *c++=0;
        devAddr=atoi(c);
        ILOG("bus %03d dev %03d\n",devBus, devAddr);
    }
    while (cnt){
        cnt--;
        libusb_get_device_descriptor( list[cnt], &desc );
        if (desc.idVendor!=USB_ST_VID) continue;
        if (devBus && devAddr)
            if ((libusb_get_bus_number(list[cnt])!=devBus) || (libusb_get_device_address(list[cnt])!=devAddr)) continue;
        if ( (desc.idProduct == USB_STLINK_32L_PID) || (desc.idProduct == USB_STLINK_NUCLEO_PID) ) break;
        if (desc.idProduct == USB_STLINK_PID) {
            slu->protocoll = 1;
            break;
        }
    }
    
    if (cnt < 0) {
        WLOG ("Couldn't find %s ST-Link/V2 devices\n",(devBus && devAddr)?"matched":"any");
        goto on_error;
    } else {
        int error = libusb_open(list[cnt], &slu->usb_handle);
        if( error !=0 ) {
            WLOG("Error %d opening ST-Link/V2 device %03d:%03d\n", error, libusb_get_bus_number(list[cnt]), libusb_get_device_address(list[cnt]));
            goto on_error;
        }
    }
    
    libusb_free_device_list(list, 1);
    

    if (libusb_kernel_driver_active(slu->usb_handle, 0) == 1) {
        int r;
        
        r = libusb_detach_kernel_driver(slu->usb_handle, 0);
        if (r<0) {
            WLOG("libusb_detach_kernel_driver(() error %s\n", strerror(-r));
            goto on_libusb_error;
        }
    }

    if (libusb_get_configuration(slu->usb_handle, &config)) {
        /* this may fail for a previous configured device */
        WLOG("libusb_get_configuration()\n");
        goto on_libusb_error;
    }

    if (config != 1) {
        printf("setting new configuration (%d -> 1)\n", config);
        if (libusb_set_configuration(slu->usb_handle, 1)) {
            /* this may fail for a previous configured device */
            WLOG("libusb_set_configuration() failed\n");
            goto on_libusb_error;
        }
    }

    if (libusb_claim_interface(slu->usb_handle, 0)) {
        WLOG("libusb_claim_interface() failed\n");
        goto on_libusb_error;
    }

    slu->req_trans = libusb_alloc_transfer(0);
    if (slu->req_trans == NULL) {
        WLOG("libusb_alloc_transfer failed\n");
        goto on_libusb_error;
    }

    slu->rep_trans = libusb_alloc_transfer(0);
    if (slu->rep_trans == NULL) {
        WLOG("libusb_alloc_transfer failed\n");
        goto on_libusb_error;
    }

    // TODO - could use the scanning techniq from stm8 code here...
    slu->ep_rep = 1 /* ep rep */ | LIBUSB_ENDPOINT_IN;
    if (desc.idProduct == USB_STLINK_NUCLEO_PID) {
    	slu->ep_req = 1 /* ep req */ | LIBUSB_ENDPOINT_OUT;
    } else {
    	slu->ep_req = 2 /* ep req */ | LIBUSB_ENDPOINT_OUT;
    }

    slu->sg_transfer_idx = 0;
    // TODO - never used at the moment, always CMD_SIZE
    slu->cmd_len = (slu->protocoll == 1)? STLINK_SG_SIZE: STLINK_CMD_SIZE;

    /* success */

    if (stlink_current_mode(sl) == STLINK_DEV_DFU_MODE) {
      ILOG("-- exit_dfu_mode\n");
      stlink_exit_dfu_mode(sl);
    }

    if (stlink_current_mode(sl) != STLINK_DEV_DEBUG_MODE) {
      stlink_enter_swd_mode(sl);
    }

    if (reset) {
        stlink_reset(sl);
    }
    stlink_load_device_params(sl);
    stlink_version(sl);

    error = 0;

on_libusb_error:
    if (devs != NULL) {
        libusb_free_device_list(devs, 1);
    }

    if (error == -1) {
        stlink_close(sl);
        return NULL;
    }

    /* success */
    return sl;

on_error:
    if( slu->libusb_ctx)
	libusb_exit(slu->libusb_ctx);
    if (sl != NULL) free(sl);
    if (slu != NULL) free(slu);
    return 0;
}

