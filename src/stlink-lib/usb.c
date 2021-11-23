#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>

#if !defined(_MSC_VER)
#include <sys/time.h>
#endif

#include <sys/types.h>
#include <errno.h>
#include <unistd.h>

#if defined(_WIN32)
#include <win32_socket.h>
#endif

#include <stlink.h>
#include <helper.h>
#include "usb.h"

enum SCSI_Generic_Direction {SG_DXFER_TO_DEV = 0, SG_DXFER_FROM_DEV = 0x80};

static inline uint32_t le_to_h_u32(const uint8_t* buf) {
    return((uint32_t)((uint32_t)buf[0] | (uint32_t)buf[1] << 8 | (uint32_t)buf[2] << 16 | (uint32_t)buf[3] << 24));
}

static int _stlink_match_speed_map(const uint32_t *map, unsigned int map_size, uint32_t khz) {
    unsigned int i;
    int speed_index = -1;
    int speed_diff = INT_MAX;
    int last_valid_speed = -1;
    bool match = true;

    for (i = 0; i < map_size; i++) {
        if (!map[i]) { continue; }

        last_valid_speed = i;

        if (khz == map[i]) {
            speed_index = i;
            break;
        } else {
            int current_diff = khz - map[i];
            // get abs value for comparison
            current_diff = (current_diff > 0) ? current_diff : -current_diff;

            if (current_diff < speed_diff) {
                speed_diff = current_diff;
                speed_index = i;
            }
        }
    }

    if (speed_index == -1) {
        // This will only be here if we cannot match the slow speed.
        // Use the slowest speed we support.
        speed_index = last_valid_speed;
        match = false;
    } else if (i == map_size) {
        match = false;
    }

    if (!match) {
        ILOG("Unable to match requested speed %d kHz, using %d kHz\n", khz, map[speed_index]);
    }

    return(speed_index);
}

void _stlink_usb_close(stlink_t* sl) {
    if (!sl) { return; }

    struct stlink_libusb * const handle = sl->backend_data;

    // maybe we couldn't even get the usb device?
    if (handle != NULL) {
        if (handle->usb_handle != NULL) { libusb_close(handle->usb_handle); }

        libusb_exit(handle->libusb_ctx);
        free(handle);
    }
}

ssize_t send_recv(struct stlink_libusb* handle, int terminate,
                  unsigned char* txbuf, size_t txsize, unsigned char* rxbuf, 
                  size_t rxsize, int check_error, const char *cmd) {
    // Note: txbuf and rxbuf can point to the same area
    int res, t, retry = 0;

    while (1) {
        res = 0;
        t = libusb_bulk_transfer(handle->usb_handle, handle->ep_req, txbuf, (int)txsize, &res, 3000);

        if (t) {
            ELOG("%s send request failed: %s\n", cmd, libusb_error_name(t));
            return(-1);
        } else if ((size_t)res != txsize) {
            ELOG("%s send request wrote %u bytes, instead of %u\n",
                   cmd, (unsigned int)res, (unsigned int)txsize);
        }

        if (rxsize != 0) {
            t = libusb_bulk_transfer(handle->usb_handle, handle->ep_rep, rxbuf, (int)rxsize, &res, 3000);

            if (t) {
                ELOG("%s read reply failed: %s\n", cmd, libusb_error_name(t));
                return(-1);
            }

            /* Checking the command execution status stored in the first byte of the response */
            if (handle->protocoll != 1 && check_error >= CMD_CHECK_STATUS && 
                        rxbuf[0] != STLINK_DEBUG_ERR_OK) {
                switch(rxbuf[0]) {
                case STLINK_DEBUG_ERR_AP_WAIT:
                case STLINK_DEBUG_ERR_DP_WAIT:
                    if (check_error == CMD_CHECK_RETRY && retry < 3) {
                        unsigned int delay_us = (1<<retry) * 1000;
                        DLOG("%s wait error (0x%02X), delaying %u us and retry\n", cmd, rxbuf[0], delay_us);
                        usleep(delay_us);
                        retry++;
                        continue;
                    }
                    DLOG("%s wait error (0x%02X)\n", cmd, rxbuf[0]);
                    break;
                case STLINK_DEBUG_ERR_FAULT: DLOG("%s response fault\n", cmd); break;
                case STLINK_DEBUG_ERR_AP_FAULT: DLOG("%s access port fault\n", cmd); break;
                case STLINK_DEBUG_ERR_DP_FAULT: DLOG("%s debug port fault\n", cmd); break;
                case STLINK_DEBUG_ERR_AP_ERROR: DLOG("%s access port error\n", cmd); break;
                case STLINK_DEBUG_ERR_DP_ERROR: DLOG("%s debug port error\n", cmd); break;
                case STLINK_DEBUG_ERR_WRITE_VERIFY:  DLOG("%s verification error\n", cmd); break;
                case STLINK_DEBUG_ERR_WRITE:  DLOG("%s write error\n", cmd); break;
                default: DLOG("%s error (0x%02X)\n", cmd, rxbuf[0]); break;
                }

                return(-1);
            }

            if (check_error == CMD_CHECK_REP_LEN && res != (int)rxsize) {
                ELOG("%s wrong reply length\n", cmd);
                res = -1;
            }
        }

        if ((handle->protocoll == 1) && terminate) {
            // read the SG reply
            unsigned char sg_buf[13];
            t = libusb_bulk_transfer(handle->usb_handle, handle->ep_rep, sg_buf, 13, &res, 3000);

            if (t) {
                ELOG("%s read storage failed: %s\n", cmd, libusb_error_name(t));
                return(-1);
            }

            // The STLink doesn't seem to evaluate the sequence number.
            handle->sg_transfer_idx++;
        }

        return(res);
    }
}

static inline int send_only(struct stlink_libusb* handle, int terminate,
                            unsigned char* txbuf, size_t txsize,
                            const char *cmd) {
    return((int)send_recv(handle, terminate, txbuf, txsize, NULL, 0, CMD_CHECK_NO, cmd));
}


static int fill_command(stlink_t * sl, enum SCSI_Generic_Direction dir, uint32_t len) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const cmd = sl->c_buf;
    int i = 0;
    memset(cmd, 0, sizeof(sl->c_buf));

    if (slu->protocoll == 1) {
        cmd[i++] = 'U';
        cmd[i++] = 'S';
        cmd[i++] = 'B';
        cmd[i++] = 'C';
        write_uint32(&cmd[i], slu->sg_transfer_idx);
        write_uint32(&cmd[i + 4], len);
        i += 8;
        cmd[i++] = (dir == SG_DXFER_FROM_DEV) ? 0x80 : 0;
        cmd[i++] = 0;   // logical unit
        cmd[i++] = 0xa; // command length
    }
    return(i);
}

int _stlink_usb_version(stlink_t *sl) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const data = sl->q_buf;
    unsigned char* const cmd  = sl->c_buf;
    ssize_t size;
    uint32_t rep_len;
    int i;

    if (sl->version.stlink_v == 3) {
        // STLINK-V3 version is determined by another command 
        rep_len = 12;
        i = fill_command(sl, SG_DXFER_FROM_DEV, 16);
        cmd[i++] = STLINK_GET_VERSION_APIV3;
    } else {
        rep_len = 6;
        i = fill_command(sl, SG_DXFER_FROM_DEV, 6);
        cmd[i++] = STLINK_GET_VERSION;
    }

    size = send_recv(slu, 1, cmd, slu->cmd_len, data, rep_len, CMD_CHECK_REP_LEN, "GET_VERSION");

    return(size<0?-1:0);
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

    size = send_recv(slu, 1, cmd, slu->cmd_len, rdata, rep_len, CMD_CHECK_REP_LEN, "GET_TARGET_VOLTAGE");

    if (size < 0) {
        return(-1);
    }

    factor = (rdata[3] << 24) | (rdata[2] << 16) | (rdata[1] << 8) | (rdata[0] << 0);
    reading = (rdata[7] << 24) | (rdata[6] << 16) | (rdata[5] << 8) | (rdata[4] << 0);
    voltage = 2400 * reading / factor;

    return(voltage);
}

int _stlink_usb_read_debug32(stlink_t *sl, uint32_t addr, uint32_t *data) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const rdata = sl->q_buf;
    unsigned char* const cmd  = sl->c_buf;
    ssize_t size;
    const int rep_len = 8;

    int i = fill_command(sl, SG_DXFER_FROM_DEV, rep_len);
    cmd[i++] = STLINK_DEBUG_COMMAND;
    cmd[i++] = STLINK_DEBUG_APIV2_READDEBUGREG;
    write_uint32(&cmd[i], addr);
    size = send_recv(slu, 1, cmd, slu->cmd_len, rdata, rep_len, CMD_CHECK_RETRY, "READDEBUGREG");

    if (size < 0) {
        return(-1);
    }

    *data = read_uint32(rdata, 4);

    return(0);
}

int _stlink_usb_write_debug32(stlink_t *sl, uint32_t addr, uint32_t data) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const rdata = sl->q_buf;
    unsigned char* const cmd  = sl->c_buf;
    ssize_t size;
    const int rep_len = 2;

    int i = fill_command(sl, SG_DXFER_FROM_DEV, rep_len);
    cmd[i++] = STLINK_DEBUG_COMMAND;
    cmd[i++] = STLINK_DEBUG_APIV2_WRITEDEBUGREG;
    write_uint32(&cmd[i], addr);
    write_uint32(&cmd[i + 4], data);
    size = send_recv(slu, 1, cmd, slu->cmd_len, rdata, rep_len, CMD_CHECK_RETRY, "WRITEDEBUGREG");

    return(size<0?-1:0);
}

int _stlink_usb_get_rw_status(stlink_t *sl) {
    if (sl->version.jtag_api == STLINK_JTAG_API_V1) { return(0); }

    unsigned char* const rdata = sl->q_buf;
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const cmd  = sl->c_buf;
    int i;
    int16_t ret = 0;

    i = fill_command(sl, SG_DXFER_FROM_DEV, 12);
    cmd[i++] = STLINK_DEBUG_COMMAND;

    if (sl->version.flags & STLINK_F_HAS_GETLASTRWSTATUS2) {
        cmd[i++] = STLINK_DEBUG_APIV2_GETLASTRWSTATUS2;
        ret = send_recv(slu, 1, cmd, slu->cmd_len, rdata, 12, CMD_CHECK_STATUS, "GETLASTRWSTATUS2");
    } else {
        cmd[i++] = STLINK_DEBUG_APIV2_GETLASTRWSTATUS;
        ret = send_recv(slu, 1, cmd, slu->cmd_len, rdata, 2, CMD_CHECK_STATUS, "GETLASTRWSTATUS");
    }

    return(ret<0?-1:0);
}

int _stlink_usb_write_mem32(stlink_t *sl, uint32_t addr, uint16_t len) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const data = sl->q_buf;
    unsigned char* const cmd  = sl->c_buf;
    int i, ret;

    i = fill_command(sl, SG_DXFER_TO_DEV, len);
    cmd[i++] = STLINK_DEBUG_COMMAND;
    cmd[i++] = STLINK_DEBUG_WRITEMEM_32BIT;
    write_uint32(&cmd[i], addr);
    write_uint16(&cmd[i + 4], len);
    ret = send_only(slu, 0, cmd, slu->cmd_len, "WRITEMEM_32BIT");

    if (ret == -1) { return(ret); }

    ret = send_only(slu, 1, data, len, "WRITEMEM_32BIT");

    if (ret == -1) { return(ret); }

    return(_stlink_usb_get_rw_status(sl));
}

int _stlink_usb_write_mem8(stlink_t *sl, uint32_t addr, uint16_t len) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const data = sl->q_buf;
    unsigned char* const cmd  = sl->c_buf;
    int i, ret;

    if ((sl->version.jtag_api < STLINK_JTAG_API_V3 && len > 64) ||
        (sl->version.jtag_api >= STLINK_JTAG_API_V3 && len > 512)) {
        ELOG("WRITEMEM_8BIT: bulk packet limits exceeded (data len %d byte)\n", len);
        return (-1);
    }

    i = fill_command(sl, SG_DXFER_TO_DEV, 0);
    cmd[i++] = STLINK_DEBUG_COMMAND;
    cmd[i++] = STLINK_DEBUG_WRITEMEM_8BIT;
    write_uint32(&cmd[i], addr);
    write_uint16(&cmd[i + 4], len);
    ret = send_only(slu, 0, cmd, slu->cmd_len, "WRITEMEM_8BIT");

    if (ret == -1) { return(ret); }

    ret = send_only(slu, 1, data, len, "WRITEMEM_8BIT");

    if (ret == -1) { return(ret); }

    return(0);
}


int _stlink_usb_current_mode(stlink_t * sl) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const cmd  = sl->c_buf;
    unsigned char* const data = sl->q_buf;
    ssize_t size;
    int rep_len = 2;
    int i = fill_command(sl, SG_DXFER_FROM_DEV, rep_len);

    cmd[i++] = STLINK_GET_CURRENT_MODE;
    size = send_recv(slu, 1, cmd,  slu->cmd_len, data, rep_len, CMD_CHECK_NO, "GET_CURRENT_MODE");

    if (size < 0) {
        return(-1);
    }

    return(sl->q_buf[0]);
}

int _stlink_usb_core_id(stlink_t * sl) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const cmd  = sl->c_buf;
    unsigned char* const data = sl->q_buf;
    ssize_t size;
    int offset, rep_len = sl->version.jtag_api == STLINK_JTAG_API_V1 ? 4 : 12;
    int i = fill_command(sl, SG_DXFER_FROM_DEV, rep_len);

    cmd[i++] = STLINK_DEBUG_COMMAND;

    if (sl->version.jtag_api == STLINK_JTAG_API_V1) {
        cmd[i++] = STLINK_DEBUG_READCOREID;
        offset = 0;
    } else {
        cmd[i++] = STLINK_DEBUG_APIV2_READ_IDCODES;
        offset = 4;
    }

    size = send_recv(slu, 1, cmd, slu->cmd_len, data, rep_len, CMD_CHECK_STATUS, "READ_IDCODES");

    if (size < 0) {
        return(-1);
    }

    sl->core_id = read_uint32(data, offset);

    return(0);
}

int _stlink_usb_status_v2(stlink_t *sl) {
    int result;
    uint32_t status = 0;

    result = _stlink_usb_read_debug32(sl, STLINK_REG_DHCSR, &status);
    DLOG("core status: %08X\n", status);

    if (result != 0) {
        sl->core_stat = TARGET_UNKNOWN;
    } else {
        if (status & STLINK_REG_DHCSR_C_HALT) {
            sl->core_stat = TARGET_HALTED;
        } else if (status & STLINK_REG_DHCSR_S_RESET_ST) {
            sl->core_stat = TARGET_RESET;
        } else {
            sl->core_stat = TARGET_RUNNING;
        }
    }

    return(result);
}

int _stlink_usb_status(stlink_t * sl) {
    if (sl->version.jtag_api != STLINK_JTAG_API_V1) { return(_stlink_usb_status_v2(sl)); }

    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const data = sl->q_buf;
    unsigned char* const cmd  = sl->c_buf;
    ssize_t size;
    int rep_len = 2;
    int i = fill_command(sl, SG_DXFER_FROM_DEV, rep_len);

    cmd[i++] = STLINK_DEBUG_COMMAND;
    cmd[i++] = STLINK_DEBUG_GETSTATUS;
    size = send_recv(slu, 1, cmd, slu->cmd_len, data, rep_len, CMD_CHECK_NO, "GETSTATUS");

    if (size > 1) {
        if (sl->q_buf[0] == STLINK_CORE_RUNNING) {
            sl->core_stat = TARGET_RUNNING;
        } else if (sl->q_buf[0] == STLINK_CORE_HALTED) {
            sl->core_stat = TARGET_HALTED;
        } else {
            sl->core_stat = TARGET_UNKNOWN;
        }
    } else {
        sl->core_stat = TARGET_UNKNOWN;
    }

    return(size<0?-1:0);
}

int _stlink_usb_force_debug(stlink_t *sl) {
    struct stlink_libusb *slu = sl->backend_data;

    int res;

    if (sl->version.jtag_api != STLINK_JTAG_API_V1) {
        res = _stlink_usb_write_debug32(sl, STLINK_REG_DHCSR, STLINK_REG_DHCSR_DBGKEY | STLINK_REG_DHCSR_C_HALT | STLINK_REG_DHCSR_C_DEBUGEN);
        return(res);
    }

    unsigned char* const data = sl->q_buf;
    unsigned char* const cmd  = sl->c_buf;
    ssize_t size;
    int rep_len = 2;
    int i = fill_command(sl, SG_DXFER_FROM_DEV, rep_len);

    cmd[i++] = STLINK_DEBUG_COMMAND;
    cmd[i++] = STLINK_DEBUG_FORCEDEBUG;
    size = send_recv(slu, 1, cmd, slu->cmd_len, data, rep_len, CMD_CHECK_RETRY, "FORCEDEBUG");

    return(size<0?-1:0);
}

int _stlink_usb_enter_swd_mode(stlink_t * sl) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const cmd  = sl->c_buf;
    ssize_t size;
    unsigned char* const data = sl->q_buf;
    const uint32_t rep_len = sl->version.jtag_api == STLINK_JTAG_API_V1 ? 0 : 2;
    int i = fill_command(sl, SG_DXFER_FROM_DEV, rep_len);

    cmd[i++] = STLINK_DEBUG_COMMAND;
    // select correct API-Version for entering SWD mode: V1 API (0x20) or V2 API (0x30).
    cmd[i++] = sl->version.jtag_api == STLINK_JTAG_API_V1 ? STLINK_DEBUG_APIV1_ENTER : STLINK_DEBUG_APIV2_ENTER;
    cmd[i++] = STLINK_DEBUG_ENTER_SWD;
    size = send_recv(slu, 1, cmd, slu->cmd_len, data, rep_len, CMD_CHECK_RETRY, "ENTER_SWD");

    return(size<0?-1:0);
}

int _stlink_usb_exit_dfu_mode(stlink_t* sl) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const cmd = sl->c_buf;
    ssize_t size;
    int i = fill_command(sl, SG_DXFER_FROM_DEV, 0);

    cmd[i++] = STLINK_DFU_COMMAND;
    cmd[i++] = STLINK_DFU_EXIT;
    size = send_only(slu, 1, cmd, slu->cmd_len, "DFU_EXIT");

    return(size<0?-1:0);
}


int _stlink_usb_reset(stlink_t * sl) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const data = sl->q_buf;
    unsigned char* const cmd = sl->c_buf;
    ssize_t size;
    int i, rep_len = 2;

    // send reset command
    i = fill_command(sl, SG_DXFER_FROM_DEV, rep_len);
    cmd[i++] = STLINK_DEBUG_COMMAND;

    if (sl->version.jtag_api == STLINK_JTAG_API_V1) {
        cmd[i++] = STLINK_DEBUG_APIV1_RESETSYS;
    } else {
        cmd[i++] = STLINK_DEBUG_APIV2_RESETSYS;
    }

    size = send_recv(slu, 1, cmd, slu->cmd_len, data, rep_len, CMD_CHECK_RETRY, "RESETSYS");

    return(size<0?-1:0);
}

int _stlink_usb_jtag_reset(stlink_t * sl, int value) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const data = sl->q_buf;
    unsigned char* const cmd = sl->c_buf;
    ssize_t size;
    int rep_len = 2;
    int i = fill_command(sl, SG_DXFER_FROM_DEV, rep_len);

    cmd[i++] = STLINK_DEBUG_COMMAND;
    cmd[i++] = STLINK_DEBUG_APIV2_DRIVE_NRST;
    cmd[i++] = value;
    size = send_recv(slu, 1, cmd, slu->cmd_len, data, rep_len, CMD_CHECK_RETRY, "DRIVE_NRST");

    return(size<0?-1:0);
}


int _stlink_usb_step(stlink_t* sl) {
    struct stlink_libusb * const slu = sl->backend_data;

    if (sl->version.jtag_api != STLINK_JTAG_API_V1) {
        // emulates the JTAG v1 API by using DHCSR
        _stlink_usb_write_debug32(sl, STLINK_REG_DHCSR, STLINK_REG_DHCSR_DBGKEY | STLINK_REG_DHCSR_C_HALT |
                                                        STLINK_REG_DHCSR_C_MASKINTS | STLINK_REG_DHCSR_C_DEBUGEN);
        _stlink_usb_write_debug32(sl, STLINK_REG_DHCSR, STLINK_REG_DHCSR_DBGKEY | STLINK_REG_DHCSR_C_STEP |
                                                        STLINK_REG_DHCSR_C_MASKINTS | STLINK_REG_DHCSR_C_DEBUGEN);
        return _stlink_usb_write_debug32(sl, STLINK_REG_DHCSR, STLINK_REG_DHCSR_DBGKEY | STLINK_REG_DHCSR_C_HALT |
                                                                STLINK_REG_DHCSR_C_DEBUGEN);
    }

    unsigned char* const data = sl->q_buf;
    unsigned char* const cmd = sl->c_buf;
    ssize_t size;
    int rep_len = 2;
    int i = fill_command(sl, SG_DXFER_FROM_DEV, rep_len);

    cmd[i++] = STLINK_DEBUG_COMMAND;
    cmd[i++] = STLINK_DEBUG_STEPCORE;
    size = send_recv(slu, 1, cmd, slu->cmd_len, data, rep_len, CMD_CHECK_RETRY, "STEPCORE");

    return(size<0?-1:0);
}

/**
 * This seems to do a good job of restarting things from the beginning?
 * @param sl
 * @param type
 */
int _stlink_usb_run(stlink_t* sl, enum run_type type) {
    struct stlink_libusb * const slu = sl->backend_data;

    int res;

    if (sl->version.jtag_api != STLINK_JTAG_API_V1) {
        res = _stlink_usb_write_debug32(sl, STLINK_REG_DHCSR, 
                    STLINK_REG_DHCSR_DBGKEY | STLINK_REG_DHCSR_C_DEBUGEN |
                    ((type==RUN_FLASH_LOADER)?STLINK_REG_DHCSR_C_MASKINTS:0));
        return(res);
    }


    unsigned char* const data = sl->q_buf;
    unsigned char* const cmd = sl->c_buf;
    ssize_t size;
    int rep_len = 2;
    int i = fill_command(sl, SG_DXFER_FROM_DEV, rep_len);

    cmd[i++] = STLINK_DEBUG_COMMAND;
    cmd[i++] = STLINK_DEBUG_RUNCORE;
    size = send_recv(slu, 1, cmd, slu->cmd_len, data, rep_len, CMD_CHECK_RETRY, "RUNCORE");

    return(size<0?-1:0);
}

int _stlink_usb_set_swdclk(stlink_t* sl, int clk_freq) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const data = sl->q_buf;
    unsigned char* const cmd = sl->c_buf;
    ssize_t size;
    int rep_len = 2;
    int i;

    // clock speed only supported by stlink/v2 and for firmware >= 22
    if (sl->version.stlink_v == 2 && sl->version.jtag_v >= 22) {
        uint16_t clk_divisor;
        if (clk_freq) {
            const uint32_t map[] = {5, 15, 25, 50, 100, 125, 240, 480, 950, 1200, 1800, 4000};
            int speed_index = _stlink_match_speed_map(map, STLINK_ARRAY_SIZE(map), clk_freq);
            switch (map[speed_index]) {
            case 5:   clk_divisor = STLINK_SWDCLK_5KHZ_DIVISOR; break;
            case 15:  clk_divisor = STLINK_SWDCLK_15KHZ_DIVISOR; break;
            case 25:  clk_divisor = STLINK_SWDCLK_25KHZ_DIVISOR; break;
            case 50:  clk_divisor = STLINK_SWDCLK_50KHZ_DIVISOR; break;
            case 100: clk_divisor = STLINK_SWDCLK_100KHZ_DIVISOR; break;
            case 125: clk_divisor = STLINK_SWDCLK_125KHZ_DIVISOR; break;
            case 240: clk_divisor = STLINK_SWDCLK_240KHZ_DIVISOR; break;
            case 480: clk_divisor = STLINK_SWDCLK_480KHZ_DIVISOR; break;
            case 950: clk_divisor = STLINK_SWDCLK_950KHZ_DIVISOR; break;
            case 1200: clk_divisor = STLINK_SWDCLK_1P2MHZ_DIVISOR; break;
            default:
            case 1800: clk_divisor = STLINK_SWDCLK_1P8MHZ_DIVISOR; break;
            case 4000: clk_divisor = STLINK_SWDCLK_4MHZ_DIVISOR; break;
            }
        } else
            clk_divisor = STLINK_SWDCLK_1P8MHZ_DIVISOR;

        i = fill_command(sl, SG_DXFER_FROM_DEV, rep_len);

        cmd[i++] = STLINK_DEBUG_COMMAND;
        cmd[i++] = STLINK_DEBUG_APIV2_SWD_SET_FREQ;
        cmd[i++] = clk_divisor & 0xFF;
        cmd[i++] = (clk_divisor >> 8) & 0xFF;
        size = send_recv(slu, 1, cmd, slu->cmd_len, data, rep_len, CMD_CHECK_RETRY, "SWD_SET_FREQ");

        return(size<0?-1:0);
    } else if (sl->version.stlink_v == 3) {
        int speed_index;
        uint32_t map[STLINK_V3_MAX_FREQ_NB];
        i = fill_command(sl, SG_DXFER_FROM_DEV, 16);

        cmd[i++] = STLINK_DEBUG_COMMAND;
        cmd[i++] = STLINK_DEBUG_APIV3_GET_COM_FREQ;
        cmd[i++] = 0; // SWD mode
        size = send_recv(slu, 1, cmd, slu->cmd_len, data, 52, CMD_CHECK_STATUS, "GET_COM_FREQ");

        if (size < 0) {
            return(-1);
        }

        int speeds_size = data[8];
        if (speeds_size > STLINK_V3_MAX_FREQ_NB) {
            speeds_size = STLINK_V3_MAX_FREQ_NB;
        }

        for (i = 0; i < speeds_size; i++) map[i] = le_to_h_u32(&data[12 + 4 * i]);

        // Set to zero all the next entries
        for (i = speeds_size; i < STLINK_V3_MAX_FREQ_NB; i++) map[i] = 0;

        if (!clk_freq) clk_freq = 1000; // set default frequency
        speed_index = _stlink_match_speed_map(map, STLINK_ARRAY_SIZE(map), clk_freq);

        i = fill_command(sl, SG_DXFER_FROM_DEV, 16);

        cmd[i++] = STLINK_DEBUG_COMMAND;
        cmd[i++] = STLINK_DEBUG_APIV3_SET_COM_FREQ;
        cmd[i++] = 0; // SWD mode
        cmd[i++] = 0;
        cmd[i++] = (uint8_t)((map[speed_index] >> 0) & 0xFF);
        cmd[i++] = (uint8_t)((map[speed_index] >> 8) & 0xFF);
        cmd[i++] = (uint8_t)((map[speed_index] >> 16) & 0xFF);
        cmd[i++] = (uint8_t)((map[speed_index] >> 24) & 0xFF);

        size = send_recv(slu, 1, cmd, slu->cmd_len, data, 8, CMD_CHECK_STATUS, "SET_COM_FREQ");

        return(size<0?-1:0);
    } else if (clk_freq) {
        WLOG("ST-Link firmware does not support frequency setup\n");
    }

    return(-1);
}

int _stlink_usb_exit_debug_mode(stlink_t *sl) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const cmd = sl->c_buf;
    ssize_t size;
    int i = fill_command(sl, SG_DXFER_FROM_DEV, 0);

    cmd[i++] = STLINK_DEBUG_COMMAND;
    cmd[i++] = STLINK_DEBUG_EXIT;

    size = send_only(slu, 1, cmd, slu->cmd_len, "DEBUG_EXIT");

    return(size<0?-1:0);
}

int _stlink_usb_read_mem32(stlink_t *sl, uint32_t addr, uint16_t len) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const data = sl->q_buf;
    unsigned char* const cmd = sl->c_buf;
    ssize_t size;
    int i = fill_command(sl, SG_DXFER_FROM_DEV, len);

    cmd[i++] = STLINK_DEBUG_COMMAND;
    cmd[i++] = STLINK_DEBUG_READMEM_32BIT;
    write_uint32(&cmd[i], addr);
    write_uint16(&cmd[i + 4], len);
    size = send_recv(slu, 1, cmd, slu->cmd_len, data, len, CMD_CHECK_NO, "READMEM_32BIT");

    if (size < 0) {
        return(-1);
    }

    sl->q_len = (int)size;
    stlink_print_data(sl);

    return(0);
}

int _stlink_usb_read_all_regs(stlink_t *sl, struct stlink_reg *regp) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const cmd = sl->c_buf;
    unsigned char* const data = sl->q_buf;
    ssize_t size;
    uint32_t rep_len = sl->version.jtag_api == STLINK_JTAG_API_V1 ? 84 : 88;
    int i = fill_command(sl, SG_DXFER_FROM_DEV, rep_len);

    cmd[i++] = STLINK_DEBUG_COMMAND;

    if (sl->version.jtag_api == STLINK_JTAG_API_V1) {
        cmd[i++] = STLINK_DEBUG_APIV1_READALLREGS;
    } else {
        cmd[i++] = STLINK_DEBUG_APIV2_READALLREGS;
    }

    size = send_recv(slu, 1, cmd, slu->cmd_len, data, rep_len, CMD_CHECK_STATUS, "READALLREGS");

    if (size < 0) {
        return(-1);
    }

    /* V1: regs data from offset 0 */
    /* V2: status at offset 0, regs data from offset 4 */
    int reg_offset = sl->version.jtag_api == STLINK_JTAG_API_V1 ? 0 : 4;
    sl->q_len = (int)size;
    stlink_print_data(sl);

    for (i = 0; i < 16; i++) regp->r[i] = read_uint32(sl->q_buf, reg_offset + i * 4);

    regp->xpsr       = read_uint32(sl->q_buf, reg_offset + 64);
    regp->main_sp    = read_uint32(sl->q_buf, reg_offset + 68);
    regp->process_sp = read_uint32(sl->q_buf, reg_offset + 72);
    regp->rw         = read_uint32(sl->q_buf, reg_offset + 76);
    regp->rw2        = read_uint32(sl->q_buf, reg_offset + 80);

    if (sl->verbose < 2) { return(0); }

    DLOG("xpsr       = 0x%08x\n", regp->xpsr);
    DLOG("main_sp    = 0x%08x\n", regp->main_sp);
    DLOG("process_sp = 0x%08x\n", regp->process_sp);
    DLOG("rw         = 0x%08x\n", regp->rw);
    DLOG("rw2        = 0x%08x\n", regp->rw2);

    return(0);
}

int _stlink_usb_read_reg(stlink_t *sl, int r_idx, struct stlink_reg *regp) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const data = sl->q_buf;
    unsigned char* const cmd  = sl->c_buf;
    ssize_t size;
    uint32_t r;
    uint32_t rep_len = sl->version.jtag_api == STLINK_JTAG_API_V1 ? 4 : 8;
    int reg_offset = sl->version.jtag_api == STLINK_JTAG_API_V1 ? 0 : 4;
    int i = fill_command(sl, SG_DXFER_FROM_DEV, rep_len);

    cmd[i++] = STLINK_DEBUG_COMMAND;

    if (sl->version.jtag_api == STLINK_JTAG_API_V1) {
        cmd[i++] = STLINK_DEBUG_APIV1_READREG;
    } else {
        cmd[i++] = STLINK_DEBUG_APIV2_READREG;
    }

    cmd[i++] = (uint8_t)r_idx;
    size = send_recv(slu, 1, cmd, slu->cmd_len, data, rep_len, CMD_CHECK_RETRY, "READREG");

    if (size < 0) {
        return(-1);
    }

    sl->q_len = (int)size;
    stlink_print_data(sl);
    r = read_uint32(sl->q_buf, reg_offset);
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

/* See section C1.6 of the ARMv7-M Architecture Reference Manual */
int _stlink_usb_read_unsupported_reg(stlink_t *sl, int r_idx, struct stlink_reg *regp) {
    uint32_t r;
    int ret;

    sl->q_buf[0] = (unsigned char)r_idx;

    for (int i = 1; i < 4; i++) sl->q_buf[i] = 0;

    ret = _stlink_usb_write_mem32(sl, STLINK_REG_DCRSR, 4);

    if (ret == -1) { return(ret); }

    ret = _stlink_usb_read_mem32(sl, STLINK_REG_DCRDR, 4);

    if (ret == -1) { return(ret); }

    r = read_uint32(sl->q_buf, 0);
    DLOG("r_idx (%2d) = 0x%08x\n", r_idx, r);

    switch (r_idx) {
    case 0x14:
        regp->primask = (uint8_t)(r & 0xFF);
        regp->basepri = (uint8_t)((r >> 8) & 0xFF);
        regp->faultmask = (uint8_t)((r >> 16) & 0xFF);
        regp->control = (uint8_t)((r >> 24) & 0xFF);
        break;
    case 0x21:
        regp->fpscr = r;
        break;
    default:
        regp->s[r_idx - 0x40] = r;
        break;
    }

    return(0);
}

int _stlink_usb_read_all_unsupported_regs(stlink_t *sl, struct stlink_reg *regp) {
    int ret;

    ret = _stlink_usb_read_unsupported_reg(sl, 0x14, regp);

    if (ret == -1) { return(ret); }

    ret = _stlink_usb_read_unsupported_reg(sl, 0x21, regp);

    if (ret == -1) { return(ret); }

    for (int i = 0; i < 32; i++) {
        ret = _stlink_usb_read_unsupported_reg(sl, 0x40 + i, regp);

        if (ret == -1) { return(ret); }
    }

    return(0);
}

/* See section C1.6 of the ARMv7-M Architecture Reference Manual */
int _stlink_usb_write_unsupported_reg(stlink_t *sl, uint32_t val, int r_idx, struct stlink_reg *regp) {
    int ret;

    if (r_idx >= 0x1C && r_idx <= 0x1F) { // primask, basepri, faultmask, or control
        /* These are held in the same register */
        ret = _stlink_usb_read_unsupported_reg(sl, 0x14, regp);

        if (ret == -1) { return(ret); }

        val = (uint8_t)(val >> 24);

        switch (r_idx) {
        case 0x1C: /* control */
            val = (((uint32_t)val) << 24) |
                  (((uint32_t)regp->faultmask) << 16) |
                  (((uint32_t)regp->basepri) << 8) |
                  ((uint32_t)regp->primask);
            break;
        case 0x1D: /* faultmask */
            val = (((uint32_t)regp->control) << 24) |
                  (((uint32_t)val) << 16) |
                  (((uint32_t)regp->basepri) << 8) |
                  ((uint32_t)regp->primask);
            break;
        case 0x1E: /* basepri */
            val = (((uint32_t)regp->control) << 24) |
                  (((uint32_t)regp->faultmask) << 16) |
                  (((uint32_t)val) << 8) |
                  ((uint32_t)regp->primask);
            break;
        case 0x1F: /* primask */
            val = (((uint32_t)regp->control) << 24) |
                  (((uint32_t)regp->faultmask) << 16) |
                  (((uint32_t)regp->basepri) << 8) |
                  ((uint32_t)val);
            break;
        }

        r_idx = 0x14;
    }

    write_uint32(sl->q_buf, val);

    ret = _stlink_usb_write_mem32(sl, STLINK_REG_DCRDR, 4);

    if (ret == -1) { return(ret); }

    sl->q_buf[0] = (unsigned char)r_idx;
    sl->q_buf[1] = 0;
    sl->q_buf[2] = 0x01;
    sl->q_buf[3] = 0;

    return(_stlink_usb_write_mem32(sl, STLINK_REG_DCRSR, 4));
}

int _stlink_usb_write_reg(stlink_t *sl, uint32_t reg, int idx) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const data = sl->q_buf;
    unsigned char* const cmd  = sl->c_buf;
    ssize_t size;
    uint32_t rep_len = 2;
    int i = fill_command(sl, SG_DXFER_FROM_DEV, rep_len);

    cmd[i++] = STLINK_DEBUG_COMMAND;

    if (sl->version.jtag_api == STLINK_JTAG_API_V1) {
        cmd[i++] = STLINK_DEBUG_APIV1_WRITEREG;
    } else {
        cmd[i++] = STLINK_DEBUG_APIV2_WRITEREG;
    }

    cmd[i++] = idx;
    write_uint32(&cmd[i], reg);
    size = send_recv(slu, 1, cmd, slu->cmd_len, data, rep_len, CMD_CHECK_RETRY, "WRITEREG");

    return(size<0?-1:0);
}

int _stlink_usb_enable_trace(stlink_t* sl, uint32_t frequency) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const data = sl->q_buf;
    unsigned char* const cmd  = sl->c_buf;
    ssize_t size;
    uint32_t rep_len = 2;

    int i = fill_command(sl, SG_DXFER_TO_DEV, rep_len);
    cmd[i++] = STLINK_DEBUG_COMMAND;
    cmd[i++] = STLINK_DEBUG_APIV2_START_TRACE_RX;
    write_uint16(&cmd[i + 0], 2 * STLINK_TRACE_BUF_LEN);
    write_uint32(&cmd[i + 2], frequency);

    size = send_recv(slu, 1, cmd, slu->cmd_len, data, rep_len, CMD_CHECK_STATUS, "START_TRACE_RX");

    return(size<0?-1:0);
}

int _stlink_usb_disable_trace(stlink_t* sl) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const data = sl->q_buf;
    unsigned char* const cmd  = sl->c_buf;
    ssize_t size;
    uint32_t rep_len = 2;

    int i = fill_command(sl, SG_DXFER_TO_DEV, rep_len);
    cmd[i++] = STLINK_DEBUG_COMMAND;
    cmd[i++] = STLINK_DEBUG_APIV2_STOP_TRACE_RX;

    size = send_recv(slu, 1, cmd, slu->cmd_len, data, rep_len, CMD_CHECK_STATUS, "STOP_TRACE_RX");

    return(size<0?-1:0);
}

int _stlink_usb_read_trace(stlink_t* sl, uint8_t* buf, size_t size) {
    struct stlink_libusb * const slu = sl->backend_data;
    unsigned char* const data = sl->q_buf;
    unsigned char* const cmd  = sl->c_buf;
    uint32_t rep_len = 2;
    int i = fill_command(sl, SG_DXFER_FROM_DEV, rep_len);

    cmd[i++] = STLINK_DEBUG_COMMAND;
    cmd[i++] = STLINK_DEBUG_APIV2_GET_TRACE_NB;
    ssize_t send_size = send_recv(slu, 1, cmd, slu->cmd_len, data, rep_len, CMD_CHECK_NO, "GET_TRACE_NB");

    if (send_size < 0) {
        return(-1);
    } else if (send_size != 2) {
        ELOG("STLINK_DEBUG_APIV2_GET_TRACE_NB reply size %d\n", (int)send_size);
        return(-1);
    }

    uint16_t trace_count = read_uint16(sl->q_buf, 0);

    if (trace_count > size) {
        ELOG("read_trace insufficient buffer length\n");
        return -1;
    }

    if (trace_count != 0) {
        int res = 0;
        int t = libusb_bulk_transfer(slu->usb_handle, slu->ep_trace, buf, trace_count, &res, 3000);

        if (t || res != (int)trace_count) {
            ELOG("read_trace read error %d\n", t);
            return(-1);
        }
    }

    return trace_count;
}

static stlink_backend_t _stlink_usb_backend = {
    _stlink_usb_close,
    _stlink_usb_exit_debug_mode,
    _stlink_usb_enter_swd_mode,
    NULL, // don't enter_jtag_mode here...
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
    _stlink_usb_target_voltage,
    _stlink_usb_set_swdclk,
    _stlink_usb_enable_trace,
    _stlink_usb_disable_trace,
    _stlink_usb_read_trace
};

/* return the length of serial or (0) in case of errors */
size_t stlink_serial(struct libusb_device_handle *handle, struct libusb_device_descriptor *desc, char *serial) {
	unsigned char desc_serial[(STLINK_SERIAL_LENGTH) * 2];

	/* truncate the string in the serial buffer */
	serial[0] = '\0';

	/* get the LANGID from String Descriptor Zero */
	int ret = libusb_get_string_descriptor(handle, 0, 0, desc_serial, sizeof(desc_serial));
	if (ret < 4) return 0;

	uint32_t langid = desc_serial[2] | (desc_serial[3] << 8);

	/* get the serial */
	ret = libusb_get_string_descriptor(handle, desc->iSerialNumber, langid, desc_serial,
		sizeof(desc_serial));
	if (ret < 0) return 0; // could not read serial

	unsigned char len = desc_serial[0];

	if (len == ((STLINK_SERIAL_LENGTH + 1) * 2)) { /* len == 50 */
		/* good ST-Link adapter */
		ret = libusb_get_string_descriptor_ascii(
			handle, desc->iSerialNumber, (unsigned char *)serial, STLINK_SERIAL_BUFFER_SIZE);
		if (ret < 0) return 0;
	} else if (len == ((STLINK_SERIAL_LENGTH / 2 + 1) * 2)) { /* len == 26 */
		/* fix-up the buggy serial */
		for (unsigned int i = 0; i < STLINK_SERIAL_LENGTH; i += 2)
			sprintf(serial + i, "%02X", desc_serial[i + 2]);
		serial[STLINK_SERIAL_LENGTH] = '\0';
	} else {
		return 0;
	}

	return strlen(serial);
}

stlink_t *stlink_open_usb(enum ugly_loglevel verbose, enum connect_type connect, char serial[STLINK_SERIAL_BUFFER_SIZE], int freq) {
    stlink_t* sl = NULL;
    struct stlink_libusb* slu = NULL;
    int ret = -1;
    int config;

    sl = calloc(1, sizeof(stlink_t));
    if (sl == NULL) { goto on_malloc_error; }

    slu = calloc(1, sizeof(struct stlink_libusb));
    if (slu == NULL) { goto on_malloc_error; }

    ugly_init(verbose);
    sl->backend = &_stlink_usb_backend;
    sl->backend_data = slu;

    sl->core_stat = TARGET_UNKNOWN;

    if (libusb_init(&(slu->libusb_ctx))) {
        WLOG("failed to init libusb context, wrong version of libraries?\n");
        goto on_error;
    }

#if LIBUSB_API_VERSION < 0x01000106
    libusb_set_debug(slu->libusb_ctx, ugly_libusb_log_level(verbose));
#else
    libusb_set_option(slu->libusb_ctx, LIBUSB_OPTION_LOG_LEVEL, ugly_libusb_log_level(verbose));
#endif

    libusb_device **list = NULL;
    // TODO: We should use ssize_t and use it as a counter if > 0.
    // As per libusb API: ssize_t libusb_get_device_list (libusb_context *ctx, libusb_device ***list)
    int cnt = (int)libusb_get_device_list(slu->libusb_ctx, &list);
    struct libusb_device_descriptor desc;
    int devBus  = 0;
    int devAddr = 0;

    // TODO: Reading a environment variable in a usb open function is not very nice, this should
    // be refactored and moved into the CLI tools, and instead of giving USB_BUS:USB_ADDR a real
    // stlink serial string should be passed to this function. Probably people are using this
    // but this is very odd because as programmer can change to multiple busses and it is better
    // to detect them based on serial.
    char *device = getenv("STLINK_DEVICE");

    if (device) {
        char *c = strchr(device, ':');

        if (c == NULL) {
            WLOG("STLINK_DEVICE must be <USB_BUS>:<USB_ADDR> format\n");
            goto on_error;
        }

        devBus = atoi(device);
        *c++ = 0;
        devAddr = atoi(c);
        ILOG("bus %03d dev %03d\n", devBus, devAddr);
    }

    while (cnt-- > 0) {
        struct libusb_device_handle *handle;

        libusb_get_device_descriptor(list[cnt], &desc);

        if (desc.idVendor != STLINK_USB_VID_ST) { continue; }

        if (devBus && devAddr) {
            if ((libusb_get_bus_number(list[cnt]) != devBus) ||
                (libusb_get_device_address(list[cnt]) != devAddr)) {
                continue;
            }
        }

        ret = libusb_open(list[cnt], &handle);

        if (ret) { continue; } // could not open device

        size_t serial_len = stlink_serial(handle, &desc, sl->serial);

        libusb_close(handle);

        if (serial_len != STLINK_SERIAL_LENGTH) { continue; } // could not read the serial

        // if no serial provided, or if serial match device, fixup version and protocol
        if (((serial == NULL) || (*serial == 0)) || (memcmp(serial, &sl->serial, STLINK_SERIAL_LENGTH) == 0)) {
            if (STLINK_V1_USB_PID(desc.idProduct)) {
                slu->protocoll = 1;
                sl->version.stlink_v = 1;
            } else if (STLINK_V2_USB_PID(desc.idProduct) || STLINK_V2_1_USB_PID(desc.idProduct)) {
                sl->version.stlink_v = 2;
            } else if (STLINK_V3_USB_PID(desc.idProduct)) {
                sl->version.stlink_v = 3;
            }

            break;
        }
    }

    if (cnt < 0) {
        WLOG ("Couldn't find %s ST-Link devices\n", (devBus && devAddr) ? "matched" : "any");
        libusb_free_device_list(list, 1);
        goto on_error;
    } else {
        ret = libusb_open(list[cnt], &slu->usb_handle);

        if (ret != 0) {
            WLOG("Error %d (%s) opening ST-Link v%d device %03d:%03d\n", ret,
                 strerror(errno),
                 sl->version.stlink_v,
                 libusb_get_bus_number(list[cnt]),
                 libusb_get_device_address(list[cnt]));
            libusb_free_device_list(list, 1);
            goto on_error;
        }
    }

    libusb_free_device_list(list, 1);

    if (libusb_kernel_driver_active(slu->usb_handle, 0) == 1) {
        ret = libusb_detach_kernel_driver(slu->usb_handle, 0);

        if (ret < 0) {
            WLOG("libusb_detach_kernel_driver(() error %s\n", strerror(-ret));
            goto on_libusb_error;
        }
    }

    if (libusb_get_configuration(slu->usb_handle, &config)) {
        // this may fail for a previous configured device
        WLOG("libusb_get_configuration()\n");
        goto on_libusb_error;
    }

    if (config != 1) {
        printf("setting new configuration (%d -> 1)\n", config);

        if (libusb_set_configuration(slu->usb_handle, 1)) {
            // this may fail for a previous configured device
            WLOG("libusb_set_configuration() failed\n");
            goto on_libusb_error;
        }
    }

    if (libusb_claim_interface(slu->usb_handle, 0)) {
        WLOG("Stlink usb device found, but unable to claim (probably already in use?)\n");
        goto on_libusb_error;
    }

    // TODO: Could use the scanning technique from STM8 code here...
    slu->ep_rep = 1 /* ep rep */ | LIBUSB_ENDPOINT_IN;

    if (desc.idProduct == STLINK_USB_PID_STLINK_NUCLEO ||
        desc.idProduct == STLINK_USB_PID_STLINK_32L_AUDIO ||
        desc.idProduct == STLINK_USB_PID_STLINK_V2_1 ||
        desc.idProduct == STLINK_USB_PID_STLINK_V3_USBLOADER ||
        desc.idProduct == STLINK_USB_PID_STLINK_V3E_PID ||
        desc.idProduct == STLINK_USB_PID_STLINK_V3S_PID ||
        desc.idProduct == STLINK_USB_PID_STLINK_V3_2VCP_PID ||
        desc.idProduct == STLINK_USB_PID_STLINK_V3_NO_MSD_PID) {
        slu->ep_req = 1 /* ep req */ | LIBUSB_ENDPOINT_OUT;
        slu->ep_trace = 2 | LIBUSB_ENDPOINT_IN;
    } else {
        slu->ep_req = 2 /* ep req */ | LIBUSB_ENDPOINT_OUT;
        slu->ep_trace = 3 | LIBUSB_ENDPOINT_IN;
    }

    slu->sg_transfer_idx = 0;
    slu->cmd_len = (slu->protocoll == 1) ? STLINK_SG_SIZE : STLINK_CMD_SIZE;

    // initialize stlink version (sl->version)
    stlink_version(sl);

    int mode = stlink_current_mode(sl);
    if (mode == STLINK_DEV_DFU_MODE) {
        DLOG("-- exit_dfu_mode\n");
        _stlink_usb_exit_dfu_mode(sl);
    }

    if (connect == CONNECT_UNDER_RESET) {
        // for the connect under reset only
        // OpenOСD says (official documentation is not available) that
        // the NRST pin must be pull down before selecting the SWD/JTAG mode
        if (mode == STLINK_DEV_DEBUG_MODE) {
            DLOG("-- exit_debug_mode\n");
            _stlink_usb_exit_dfu_mode(sl);
        }

        _stlink_usb_jtag_reset(sl, STLINK_DEBUG_APIV2_DRIVE_NRST_LOW);
    }

    sl->freq = freq;
    // set the speed before entering the mode as the chip discovery phase
    // should be done at this speed too
    // set the stlink clock speed (default is 1800kHz)
    DLOG("JTAG/SWD freq set to %d\n", freq);
    _stlink_usb_set_swdclk(sl, freq);

    stlink_target_connect(sl, connect);
    return(sl);

on_libusb_error:
    stlink_close(sl);
    return(NULL);

on_error:
    if (slu->libusb_ctx) { libusb_exit(slu->libusb_ctx); }

on_malloc_error:
    if (sl != NULL) { free(sl); }
    if (slu != NULL) { free(slu); }

    return(NULL);
}

static size_t stlink_probe_usb_devs(libusb_device **devs, stlink_t **sldevs[], enum connect_type connect, int freq) {
    stlink_t **_sldevs;
    libusb_device *dev;
    int i = 0;
    size_t slcnt = 0;
    size_t slcur = 0;

    /* Count STLINKs */
    while ((dev = devs[i++]) != NULL) {
        struct libusb_device_descriptor desc;
        int ret = libusb_get_device_descriptor(dev, &desc);

        if (ret < 0) {
            WLOG("failed to get libusb device descriptor (libusb error: %d)\n", ret);
            break;
        }

        if (desc.idVendor != STLINK_USB_VID_ST) { continue; }

        if (!STLINK_SUPPORTED_USB_PID(desc.idProduct)) {
            WLOG("skipping ST device : %#04x:%#04x)\n", desc.idVendor, desc.idProduct);
            continue;
        }

        slcnt++;
    }

    _sldevs = calloc(slcnt, sizeof(stlink_t *)); // allocate list of pointers

    if (!_sldevs) {
        *sldevs = NULL;
        return(0);
    }

    /* Open STLINKS and attach them to list */
    i = 0;

    while ((dev = devs[i++]) != NULL) {
        struct libusb_device_descriptor desc;
        int ret = libusb_get_device_descriptor(dev, &desc);

        if (ret < 0) {
            WLOG("failed to get libusb device descriptor (libusb error: %d)\n", ret);
            break;
        }

        if (!STLINK_SUPPORTED_USB_PID(desc.idProduct)) { continue; }

        struct libusb_device_handle* handle;
        char serial[STLINK_SERIAL_BUFFER_SIZE] = {0, };

        ret = libusb_open(dev, &handle);

        if (ret < 0) {
            if (ret == LIBUSB_ERROR_ACCESS) {
                ELOG("Could not open USB device %#06x:%#06x, access error.\n", desc.idVendor, desc.idProduct);
            } else {
                ELOG("Failed to open USB device %#06x:%#06x, libusb error: %d)\n", desc.idVendor, desc.idProduct, ret);
            }

            break;
        }

        size_t serial_len = stlink_serial(handle, &desc, serial);

        libusb_close(handle);

        if (serial_len != STLINK_SERIAL_LENGTH) { continue; }

        stlink_t *sl = stlink_open_usb(0, connect, serial, freq);

        if (!sl) {
            ELOG("Failed to open USB device %#06x:%#06x\n", desc.idVendor, desc.idProduct);
            continue;
        }

        _sldevs[slcur++] = sl;
    }

    *sldevs = _sldevs;

    return(slcur);
}

size_t stlink_probe_usb(stlink_t **stdevs[], enum connect_type connect, int freq) {
    libusb_device **devs;
    stlink_t **sldevs;

    size_t slcnt = 0;
    int r;
    ssize_t cnt;

    r = libusb_init(NULL);

    if (r < 0) { return(0); }

    cnt = libusb_get_device_list(NULL, &devs);

    if (cnt < 0) { return(0); }

    slcnt = stlink_probe_usb_devs(devs, &sldevs, connect, freq);
    libusb_free_device_list(devs, 1);

    libusb_exit(NULL);

    *stdevs = sldevs;

    return(slcnt);
}

void stlink_probe_usb_free(stlink_t ***stdevs, size_t size) {
    if (stdevs == NULL || *stdevs == NULL || size == 0) { return; }

    for (size_t n = 0; n < size; n++) { stlink_close((*stdevs)[n]); }

    free(*stdevs);
    *stdevs = NULL;
}
