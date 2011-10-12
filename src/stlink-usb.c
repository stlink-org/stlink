#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <sys/types.h>
#include <libusb-1.0/libusb.h>
#include "stlink-common.h"
#include "stlink-usb.h"

void _stlink_usb_close(stlink_t* sl) {
    struct stlink_libusb * const handle = sl->backend_data;
    // maybe we couldn't even get the usb device?
    if (handle != NULL) {
        if (handle->req_trans != NULL)
            libusb_free_transfer(handle->req_trans);

        if (handle->rep_trans != NULL)
            libusb_free_transfer(handle->rep_trans);

        if (handle->usb_handle != NULL)
            libusb_close(handle->usb_handle);

        libusb_exit(handle->libusb_ctx);
        free(handle);
    }
}


struct trans_ctx {
#define TRANS_FLAGS_IS_DONE (1 << 0)
#define TRANS_FLAGS_HAS_ERROR (1 << 1)
    volatile unsigned long flags;
};

static void on_trans_done(struct libusb_transfer * trans) {
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

ssize_t send_recv(struct stlink_libusb* handle,
        unsigned char* txbuf, size_t txsize,
        unsigned char* rxbuf, size_t rxsize) {
    /* note: txbuf and rxbuf can point to the same area */

    libusb_fill_bulk_transfer(handle->req_trans, handle->usb_handle,
            handle->ep_req,
            txbuf, txsize,
            NULL, NULL,
            0
            );

    printf("submit_wait(req)\n");

    if (submit_wait(handle, handle->req_trans)) return -1;

    /* send_only */
    if (rxsize == 0) return 0;

    /* read the response */

    libusb_fill_bulk_transfer(handle->rep_trans, handle->usb_handle,
            handle->ep_rep, rxbuf, rxsize, NULL, NULL, 0);

    printf("submit_wait(rep)\n");

    if (submit_wait(handle, handle->rep_trans)) return -1;

    return handle->rep_trans->actual_length;
}

static inline int send_only
(struct stlink_libusb* handle, unsigned char* txbuf, size_t txsize) {
    return send_recv(handle, txbuf, txsize, NULL, 0);
}


// KARL - fixme, common code! (or, one per backend)
// candidate for common code...


static int is_stlink_device(libusb_device * dev) {
    struct libusb_device_descriptor desc;

    if (libusb_get_device_descriptor(dev, &desc))
        return 0;

    printf("device: 0x%04x, 0x%04x\n", desc.idVendor, desc.idProduct);

    if (desc.idVendor != USB_ST_VID)
        return 0;

    if (desc.idProduct != USB_STLINK_32L_PID)
        return 0;

    return 1;
}

void _stlink_usb_version(stlink_t *sl) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const buf = sl->q_buf;
    ssize_t size;

    memset(buf, 0, sizeof (sl->q_buf));
    buf[0] = STLINK_GET_VERSION;
    buf[1] = 0x80;

    size = send_recv(slu, buf, STLINK_CMD_SIZE, buf, sizeof (sl->q_buf));
    if (size == -1) {
        printf("[!] send_recv\n");
        return;
    }

#if 1 /* DEBUG */
    {
        unsigned int i;
        for (i = 0; i < size; ++i) printf("%02x", buf[i]);
        printf("\n");
    }
#endif /* DEBUG */
}

void _stlink_usb_write_mem32(stlink_t *sl, uint32_t addr, uint16_t len) {
    DD(sl, "oops! no write32 support yet, wanted to write %d bytes to %#x\n",
            len, addr);
}

void _stlink_usb_write_mem8(stlink_t *sl, uint32_t addr, uint16_t len) {
    DD(sl, "oops! no write8 support yet, wanted to write %d bytes to %#x\n",
            len, addr);
}


int _stlink_usb_current_mode(stlink_t * sl) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const buf = sl->q_buf;
    ssize_t size;
    memset(buf, 0, sizeof (sl->q_buf));
    buf[0] = STLINK_GET_CURRENT_MODE;
    size = send_recv(slu, buf, STLINK_CMD_SIZE, buf, sizeof (sl->q_buf));
    if (size == -1) {
        printf("[!] send_recv\n");
        return -1;
    }
    return sl->q_buf[0];
}

void _stlink_usb_core_id(stlink_t * sl) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const buf = sl->q_buf;
    ssize_t size;

    memset(buf, 0, sizeof (sl->q_buf));
    buf[0] = STLINK_DEBUG_COMMAND;
    buf[1] = STLINK_DEBUG_READCOREID;

    size = send_recv(slu, buf, STLINK_CMD_SIZE, buf, sizeof (sl->q_buf));
    if (size == -1) {
        printf("[!] send_recv\n");
        return;
    }

    sl->core_id = read_uint32(buf, 0);
}

void _stlink_usb_status(stlink_t * sl) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const buf = sl->q_buf;
    ssize_t size;

    memset(buf, 0, sizeof (sl->q_buf));

    buf[0] = STLINK_DEBUG_COMMAND;
    buf[1] = STLINK_DEBUG_GETSTATUS;

    size = send_recv(slu, buf, STLINK_CMD_SIZE, buf, sizeof (sl->q_buf));
    if (size == -1) {
        printf("[!] send_recv\n");
        return;
    }

    /* todo: stlink_core_stat */

    // FIXME - decode into sl->core_stat
#if 1 /* DEBUG */
    printf("status == 0x%x\n", buf[0]);
#endif /* DEBUG */

}

void _stlink_usb_enter_swd_mode(stlink_t * sl) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const buf = sl->q_buf;
    ssize_t size;

    memset(buf, 0, sizeof (sl->q_buf));

    buf[0] = STLINK_DEBUG_COMMAND;
    buf[1] = STLINK_SWD_ENTER;
    buf[2] = STLINK_DEBUG_ENTER_SWD;

    size = send_recv(slu, buf, STLINK_CMD_SIZE, buf, sizeof (sl->q_buf));
    if (size == -1) {
        printf("[!] send_recv\n");
        return;
    }
}

void _stlink_usb_exit_dfu_mode(stlink_t* sl) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const buf = sl->q_buf;
    ssize_t size;

    memset(buf, 0, sizeof (sl->q_buf));
    buf[0] = STLINK_DFU_COMMAND;
    buf[1] = STLINK_DFU_EXIT;

    size = send_only(slu, buf, 16);
    if (size == -1) {
        printf("[!] send_recv\n");
        return;
    }
}

void _stlink_usb_reset(stlink_t * sl) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const buf = sl->q_buf;
    ssize_t size;

    memset(buf, 0, sizeof (sl->q_buf));
    buf[0] = STLINK_DEBUG_COMMAND;
    buf[1] = STLINK_DEBUG_RESETSYS;

    size = send_recv(slu, buf, STLINK_CMD_SIZE, buf, sizeof (sl->q_buf));
    if (size == -1) {
        printf("[!] send_recv\n");
        return;
    }
}


void _stlink_usb_step(stlink_t* sl) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const buf = sl->q_buf;
    ssize_t size;

    memset(buf, 0, sizeof (sl->q_buf));
    buf[0] = STLINK_DEBUG_COMMAND;
    buf[1] = STLINK_DEBUG_STEPCORE;

    size = send_recv(slu, buf, STLINK_CMD_SIZE, buf, sizeof (sl->q_buf));
    if (size == -1) {
        printf("[!] send_recv\n");
        return;
    }
}

void _stlink_usb_run(stlink_t* sl) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const buf = sl->q_buf;
    ssize_t size;

    memset(buf, 0, sizeof (sl->q_buf));
    buf[0] = STLINK_DEBUG_COMMAND;
    buf[1] = STLINK_DEBUG_RUNCORE;

    size = send_recv(slu, buf, STLINK_CMD_SIZE, buf, sizeof (sl->q_buf));
    if (size == -1) {
        printf("[!] send_recv\n");
        return;
    }

}

void _stlink_usb_exit_debug_mode(stlink_t *sl) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const buf = sl->q_buf;
    ssize_t size;

    memset(buf, 0, sizeof (sl->q_buf));
    buf[0] = STLINK_DEBUG_COMMAND;
    buf[1] = STLINK_DEBUG_EXIT;

    size = send_only(slu, buf, 16);
    if (size == -1) {
        printf("[!] send_only\n");
        return;
    }
}

void _stlink_usb_read_mem32(stlink_t *sl, uint32_t addr, uint16_t len) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const buf = sl->q_buf;
    ssize_t size;

    /* assume len < sizeof(sl->q_buf) */
    assert(len < sizeof(sl->q_buf));  // makes a compiler warning? always true?

    memset(buf, 0, sizeof (sl->q_buf));
    buf[0] = STLINK_DEBUG_COMMAND;
    buf[1] = STLINK_DEBUG_READMEM_32BIT;
    write_uint32(buf + 2, addr);
    /* windows usb logs show only one byte is used for length ... */
    // Presumably, this is because usb transfers can't be 16 bits worth of bytes long...
    assert (len < 256);
    buf[6] = (uint8_t) len;

    size = send_recv(slu, buf, STLINK_CMD_SIZE, buf, sizeof (sl->q_buf));
    if (size == -1) {
        printf("[!] send_recv\n");
        return;
    }

    sl->q_len = (size_t) size;

    stlink_print_data(sl);
}

void _stlink_usb_read_all_regs(stlink_t *sl, reg *regp) {
    DD(sl, "oops! read_all_regs not implemented for USB!\n");
}

void _stlink_usb_read_reg(stlink_t *sl, int r_idx, reg *regp) {
    DD(sl, "oops! read_reg not implemented for USB! Wanted to read reg %d\n",
            r_idx);
}

void _stlink_usb_write_reg(stlink_t *sl, uint32_t reg, int idx) {
    DD(sl, "oops! write_reg not implemented for USB! Wanted to write %#x to %d\n",
            reg, idx);
}



stlink_backend_t _stlink_usb_backend = {
    _stlink_usb_close,
    _stlink_usb_exit_debug_mode,
    _stlink_usb_enter_swd_mode,
    NULL,  // no enter_jtag_mode here...
    _stlink_usb_exit_dfu_mode,
    _stlink_usb_core_id,
    _stlink_usb_reset,
    _stlink_usb_run,
    _stlink_usb_status,
    _stlink_usb_version,
    _stlink_usb_read_mem32,
    _stlink_usb_write_mem32,
    _stlink_usb_write_mem8,
    _stlink_usb_read_all_regs,
    _stlink_usb_read_reg,
    _stlink_usb_write_reg,
    _stlink_usb_step,
    _stlink_usb_current_mode
};


stlink_t* stlink_open_usb(const char *dev_name, const int verbose) {
    stlink_t* sl = NULL;
    struct stlink_libusb* slu = NULL;

    sl = malloc(sizeof (stlink_t));
    slu = malloc(sizeof (struct stlink_libusb));
    if (sl == NULL) goto on_error;
    if (slu == NULL) goto on_error;

    sl->verbose = verbose;
    
    if (slu->libusb_ctx != NULL) {
        fprintf(stderr, "reopening with an existing context? undefined behaviour!\n");
        goto on_error;
    } else {
        if (libusb_init(&(slu->libusb_ctx))) {
            fprintf(stderr, "failed to init libusb context, wrong version of libraries?\n");
            goto on_error;
        }
    }

    int error = -1;

    libusb_device** devs = NULL;
    libusb_device* dev;
    ssize_t i;
    ssize_t count;
    int config;

    count = libusb_get_device_list(slu->libusb_ctx, &devs);
    if (count < 0) {
        printf("libusb_get_device_list\n");
        goto on_libusb_error;
    }

    for (i = 0; i < count; ++i) {
        dev = devs[i];
        if (is_stlink_device(dev)) break;
    }
    if (i == count) return NULL;

    if (libusb_open(dev, &(slu->usb_handle))) {
        printf("libusb_open()\n");
        goto on_libusb_error;
    }

    if (libusb_get_configuration(slu->usb_handle, &config)) {
        /* this may fail for a previous configured device */
        printf("libusb_get_configuration()\n");
        goto on_libusb_error;
    }

    if (config != 1) {
        printf("setting new configuration (%d -> 1)\n", config);
        if (libusb_set_configuration(slu->usb_handle, 1)) {
            /* this may fail for a previous configured device */
            printf("libusb_set_configuration()\n");
            goto on_libusb_error;
        }
    }

    if (libusb_claim_interface(slu->usb_handle, 0)) {
        printf("libusb_claim_interface()\n");
        goto on_libusb_error;
    }

    slu->req_trans = libusb_alloc_transfer(0);
    if (slu->req_trans == NULL) {
        printf("libusb_alloc_transfer\n");
        goto on_libusb_error;
    }

    slu->rep_trans = libusb_alloc_transfer(0);
    if (slu->rep_trans == NULL) {
        printf("libusb_alloc_transfer\n");
        goto on_libusb_error;
    }

    slu->ep_rep = 1 /* ep rep */ | LIBUSB_ENDPOINT_IN;
    slu->ep_req = 2 /* ep req */ | LIBUSB_ENDPOINT_OUT;

    /* libusb_reset_device(slu->usb_handle); */

    /* success */
    error = 0;

on_libusb_error:
    if (devs != NULL) {
        libusb_free_device_list(devs, 1);
        fprintf(stderr, "freed libusb device list\n");
    }

    if (error == -1) {
        stlink_close(sl);
        return NULL;
    }

    sl->backend = &_stlink_usb_backend;
    sl->backend_data = slu;
    /* success */
    return sl;

on_error:
    if (sl != NULL) free(sl);
    if (slu != NULL) free(slu);
    return 0;
}