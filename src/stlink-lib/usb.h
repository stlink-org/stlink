/*
 * File: usb.h
 *
 * USB commands & interaction with ST-LINK devices
 */

#ifndef USB_H
#define USB_H

#include <stdint.h>

#include "libusb_settings.h"
#include "logging.h"

#define STLINK_USB_VID_ST                   0x0483
#define STLINK_USB_PID_STLINK               0x3744
#define STLINK_USB_PID_STLINK_32L           0x3748
#define STLINK_USB_PID_STLINK_32L_AUDIO     0x374a
#define STLINK_USB_PID_STLINK_NUCLEO        0x374b
#define STLINK_USB_PID_STLINK_V2_1          0x3752
#define STLINK_USB_PID_STLINK_V3_USBLOADER  0x374d
#define STLINK_USB_PID_STLINK_V3E_PID       0x374e
#define STLINK_USB_PID_STLINK_V3S_PID       0x374f
#define STLINK_USB_PID_STLINK_V3_2VCP_PID   0x3753
#define STLINK_USB_PID_STLINK_V3_NO_MSD_PID 0x3754

#define STLINK_V1_USB_PID(pid) ((pid) == STLINK_USB_PID_STLINK)

#define STLINK_V2_USB_PID(pid) ((pid) == STLINK_USB_PID_STLINK_32L || \
                                (pid) == STLINK_USB_PID_STLINK_32L_AUDIO || \
                                (pid) == STLINK_USB_PID_STLINK_NUCLEO)

#define STLINK_V2_1_USB_PID(pid) ((pid) == STLINK_USB_PID_STLINK_V2_1)

#define STLINK_V3_USB_PID(pid) ((pid) == STLINK_USB_PID_STLINK_V3_USBLOADER || \
                                (pid) == STLINK_USB_PID_STLINK_V3E_PID || \
                                (pid) == STLINK_USB_PID_STLINK_V3S_PID || \
                                (pid) == STLINK_USB_PID_STLINK_V3_2VCP_PID || \
                                (pid) == STLINK_USB_PID_STLINK_V3_NO_MSD_PID)

#define STLINK_SUPPORTED_USB_PID(pid) (STLINK_V1_USB_PID(pid) || \
                                       STLINK_V2_USB_PID(pid) || \
                                       STLINK_V2_1_USB_PID(pid) || \
                                       STLINK_V3_USB_PID(pid))

#define STLINK_SG_SIZE 31
#define STLINK_CMD_SIZE 16

enum SCSI_Generic_Direction {SG_DXFER_TO_DEV = 0, SG_DXFER_FROM_DEV = 0x80};

struct stlink_libusb {
    libusb_context* libusb_ctx;
    libusb_device_handle* usb_handle;
    uint32_t ep_req;
    uint32_t ep_rep;
    uint32_t ep_trace;
    int32_t protocoll;
    uint32_t sg_transfer_idx;
    uint32_t cmd_len;
};

// static inline uint32_t le_to_h_u32(const uint8_t* buf);
// static int32_t _stlink_match_speed_map(const uint32_t *map, uint32_t map_size, uint32_t khz);
void _stlink_usb_close(stlink_t* sl);
ssize_t send_recv(struct stlink_libusb* handle, int32_t terminate, unsigned char* txbuf, uint32_t txsize,
                    unsigned char* rxbuf, uint32_t rxsize, int32_t check_error, const char *cmd);
// static inline int32_t send_only(struct stlink_libusb* handle, int32_t terminate, unsigned char* txbuf,
//                                  uint32_t txsize, const char *cmd);
// static int32_t fill_command(stlink_t * sl, enum SCSI_Generic_Direction dir, uint32_t len);
int32_t _stlink_usb_version(stlink_t *sl);
int32_t _stlink_usb_target_voltage(stlink_t *sl);
int32_t _stlink_usb_read_debug32(stlink_t *sl, uint32_t addr, uint32_t *data);
int32_t _stlink_usb_write_debug32(stlink_t *sl, uint32_t addr, uint32_t data);
int32_t _stlink_usb_get_rw_status(stlink_t *sl);
int32_t _stlink_usb_write_mem32(stlink_t *sl, uint32_t addr, uint16_t len);
int32_t _stlink_usb_write_mem8(stlink_t *sl, uint32_t addr, uint16_t len);
int32_t _stlink_usb_current_mode(stlink_t * sl);
int32_t _stlink_usb_core_id(stlink_t * sl);
int32_t _stlink_usb_status_v2(stlink_t *sl);
int32_t _stlink_usb_status(stlink_t * sl);
int32_t _stlink_usb_force_debug(stlink_t *sl);
int32_t _stlink_usb_enter_swd_mode(stlink_t * sl);
int32_t _stlink_usb_exit_dfu_mode(stlink_t* sl);
int32_t _stlink_usb_reset(stlink_t * sl);
int32_t _stlink_usb_jtag_reset(stlink_t * sl, int32_t value);
int32_t _stlink_usb_step(stlink_t* sl);
int32_t _stlink_usb_run(stlink_t* sl, enum run_type type);
int32_t _stlink_usb_set_swdclk(stlink_t* sl, int32_t clk_freq);
int32_t _stlink_usb_exit_debug_mode(stlink_t *sl);
int32_t _stlink_usb_read_mem32(stlink_t *sl, uint32_t addr, uint16_t len);
int32_t _stlink_usb_read_all_regs(stlink_t *sl, struct stlink_reg *regp);
int32_t _stlink_usb_read_reg(stlink_t *sl, int32_t r_idx, struct stlink_reg *regp);
int32_t _stlink_usb_read_unsupported_reg(stlink_t *sl, int32_t r_idx, struct stlink_reg *regp);
int32_t _stlink_usb_read_all_unsupported_regs(stlink_t *sl, struct stlink_reg *regp);
int32_t _stlink_usb_write_unsupported_reg(stlink_t *sl, uint32_t val, int32_t r_idx, struct stlink_reg *regp);
int32_t _stlink_usb_write_reg(stlink_t *sl, uint32_t reg, int32_t idx);
int32_t _stlink_usb_enable_trace(stlink_t* sl, uint32_t frequency);
int32_t _stlink_usb_disable_trace(stlink_t* sl);
int32_t _stlink_usb_read_trace(stlink_t* sl, uint8_t* buf, uint32_t size);

// static stlink_backend_t _stlink_usb_backend = { };

size_t stlink_serial(struct libusb_device_handle *handle, struct libusb_device_descriptor *desc, char *serial);
stlink_t *stlink_open_usb(enum ugly_loglevel verbose, enum connect_type connect, char serial[STLINK_SERIAL_BUFFER_SIZE], int32_t freq);
// static uint32_t stlink_probe_usb_devs(libusb_device **devs, stlink_t **sldevs[], enum connect_type connect, int32_t freq);
size_t stlink_probe_usb(stlink_t **stdevs[], enum connect_type connect, int32_t freq);
void stlink_probe_usb_free(stlink_t **stdevs[], uint32_t size);

#endif // USB_H
