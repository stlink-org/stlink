/*
 * File: stlink.h
 *
 * This should contain all the common top level stlink interfaces,
 * regardless of how the backend does the work....
 */

#ifndef STLINK_H
#define STLINK_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "stm32.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STLINK_ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

/* Max data transfer size */
// 6kB = max mem32_read block, 8kB sram
// #define Q_BUF_LEN    96
#define Q_BUF_LEN (1024 * 100)

/* Statuses of core */
enum target_state {
    TARGET_UNKNOWN = 0,
    TARGET_RUNNING = 1,
    TARGET_HALTED = 2,
    TARGET_RESET = 3,
    TARGET_DEBUG_RUNNING = 4,
};

#define STLINK_CORE_RUNNING             0x80
#define STLINK_CORE_HALTED              0x81

/* STLINK modes */
#define STLINK_DEV_DFU_MODE             0x00
#define STLINK_DEV_MASS_MODE            0x01
#define STLINK_DEV_DEBUG_MODE           0x02
#define STLINK_DEV_UNKNOWN_MODE           -1

/* NRST pin states */
#define STLINK_DEBUG_APIV2_DRIVE_NRST_LOW  0x00
#define STLINK_DEBUG_APIV2_DRIVE_NRST_HIGH 0x01

/* Baud rate divisors for SWDCLK */
#define STLINK_SWDCLK_4MHZ_DIVISOR        0
#define STLINK_SWDCLK_1P8MHZ_DIVISOR      1
#define STLINK_SWDCLK_1P2MHZ_DIVISOR      2
#define STLINK_SWDCLK_950KHZ_DIVISOR      3
#define STLINK_SWDCLK_480KHZ_DIVISOR      7
#define STLINK_SWDCLK_240KHZ_DIVISOR     15
#define STLINK_SWDCLK_125KHZ_DIVISOR     31
#define STLINK_SWDCLK_100KHZ_DIVISOR     40
#define STLINK_SWDCLK_50KHZ_DIVISOR      79
#define STLINK_SWDCLK_25KHZ_DIVISOR     158
#define STLINK_SWDCLK_15KHZ_DIVISOR     265
#define STLINK_SWDCLK_5KHZ_DIVISOR      798

#define STLINK_SERIAL_LENGTH             24
#define STLINK_SERIAL_BUFFER_SIZE        (STLINK_SERIAL_LENGTH + 1)

#define STLINK_V3_MAX_FREQ_NB            10

#define STLINK_TRACE_BUF_LEN               2048
#define STLINK_V2_MAX_TRACE_FREQUENCY   2000000
#define STLINK_V3_MAX_TRACE_FREQUENCY  24000000
#define STLINK_DEFAULT_TRACE_FREQUENCY  2000000

/* Map the relevant features, quirks and workaround for specific firmware version of stlink */
#define STLINK_F_HAS_TRACE              (1 << 0)
#define STLINK_F_HAS_SWD_SET_FREQ       (1 << 1)
#define STLINK_F_HAS_JTAG_SET_FREQ      (1 << 2)
#define STLINK_F_HAS_MEM_16BIT          (1 << 3)
#define STLINK_F_HAS_GETLASTRWSTATUS2   (1 << 4)
#define STLINK_F_HAS_DAP_REG            (1 << 5)
#define STLINK_F_QUIRK_JTAG_DP_READ     (1 << 6)
#define STLINK_F_HAS_AP_INIT            (1 << 7)
#define STLINK_F_HAS_DPBANKSEL          (1 << 8)
#define STLINK_F_HAS_RW8_512BYTES       (1 << 9)

/* Error code */
#define STLINK_DEBUG_ERR_OK              0x80
#define STLINK_DEBUG_ERR_FAULT           0x81
#define STLINK_DEBUG_ERR_WRITE           0x0c
#define STLINK_DEBUG_ERR_WRITE_VERIFY    0x0d
#define STLINK_DEBUG_ERR_AP_WAIT         0x10
#define STLINK_DEBUG_ERR_AP_FAULT        0x11
#define STLINK_DEBUG_ERR_AP_ERROR        0x12
#define STLINK_DEBUG_ERR_DP_WAIT         0x14
#define STLINK_DEBUG_ERR_DP_FAULT        0x15
#define STLINK_DEBUG_ERR_DP_ERROR        0x16

#define CMD_CHECK_NO         0
#define CMD_CHECK_REP_LEN    1
#define CMD_CHECK_STATUS     2
#define CMD_CHECK_RETRY      3 /* check status and retry if wait error */

#define C_BUF_LEN 32

enum stlink_flash_type {
    STLINK_FLASH_TYPE_UNKNOWN = 0,
    STLINK_FLASH_TYPE_F0,    // used by f0, f1 (except f1xl),f3. */
    STLINK_FLASH_TYPE_F1_XL, // f0 flash with dual bank, apparently */
    STLINK_FLASH_TYPE_F4,    // used by f2, f4 */
    STLINK_FLASH_TYPE_F7,
    STLINK_FLASH_TYPE_L0,    // l0, l1 */
    STLINK_FLASH_TYPE_L4,    // l4, l4+ */
    STLINK_FLASH_TYPE_G0,
    STLINK_FLASH_TYPE_G4,
    STLINK_FLASH_TYPE_WB,
    STLINK_FLASH_TYPE_H7,
    STLINK_FLASH_TYPE_MAX,
};

struct stlink_reg {
    uint32_t r[16];
    uint32_t s[32];
    uint32_t xpsr;
    uint32_t main_sp;
    uint32_t process_sp;
    uint32_t rw;
    uint32_t rw2;
    uint8_t control;
    uint8_t faultmask;
    uint8_t basepri;
    uint8_t primask;
    uint32_t fpscr;
};

typedef uint32_t stm32_addr_t;

typedef struct flash_loader {
    stm32_addr_t loader_addr; // loader sram addr
    stm32_addr_t buf_addr; // buffer sram address
    uint32_t rcc_dma_bkp; // backup RCC DMA enable state
    uint32_t iwdg_kr; // IWDG key register address
} flash_loader_t;

typedef struct _cortex_m3_cpuid_ {
    uint16_t implementer_id;
    uint16_t variant;
    uint16_t part;
    uint8_t revision;
} cortex_m3_cpuid_t;

enum stlink_jtag_api_version {
    STLINK_JTAG_API_V1 = 1,
    STLINK_JTAG_API_V2,
    STLINK_JTAG_API_V3,
};

typedef struct stlink_version_ {
    uint32_t stlink_v;
    uint32_t jtag_v;
    uint32_t swim_v;
    uint32_t st_vid;
    uint32_t stlink_pid;
    // jtag api version supported
    enum stlink_jtag_api_version jtag_api;
    // one bit for each feature supported. See macros STLINK_F_*
    uint32_t flags;
} stlink_version_t;

enum transport_type {
    TRANSPORT_TYPE_ZERO = 0,
    TRANSPORT_TYPE_LIBSG,
    TRANSPORT_TYPE_LIBUSB,
    TRANSPORT_TYPE_INVALID
};

enum connect_type {
    CONNECT_HOT_PLUG = 0,
    CONNECT_NORMAL = 1,
    CONNECT_UNDER_RESET = 2,
};

enum reset_type {
    RESET_AUTO = 0,
    RESET_HARD = 1,
    RESET_SOFT = 2,
    RESET_SOFT_AND_HALT = 3,
};

enum run_type {
    RUN_NORMAL = 0,
    RUN_FLASH_LOADER = 1,
};


typedef struct _stlink stlink_t;

#include <backend.h>

struct _stlink {
    struct _stlink_backend *backend;
    void *backend_data;

    // room for the command header
    unsigned char c_buf[C_BUF_LEN];
    // data transferred from or to device
    unsigned char q_buf[Q_BUF_LEN];
    int q_len;

    // transport layer verboseness: 0 for no debug info, 10 for lots
    int verbose;
    int opt;
    uint32_t core_id;            // set by stlink_core_id(), result from STLINK_DEBUGREADCOREID
    uint32_t chip_id;            // set by stlink_load_device_params(), used to identify flash and sram
    enum target_state core_stat; // set by stlink_status()

    char serial[STLINK_SERIAL_BUFFER_SIZE];
    int freq;                    // set by stlink_open_usb(), values: STLINK_SWDCLK_xxx_DIVISOR

    enum stlink_flash_type flash_type;
    // stlink_chipid_params.flash_type, set by stlink_load_device_params(), values: STLINK_FLASH_TYPE_xxx

    stm32_addr_t flash_base;     // STM32_FLASH_BASE, set by stlink_load_device_params()
    size_t flash_size;           // calculated by stlink_load_device_params()
    size_t flash_pgsz;           // stlink_chipid_params.flash_pagesize, set by stlink_load_device_params()

    /* sram settings */
    stm32_addr_t sram_base;      // STM32_SRAM_BASE, set by stlink_load_device_params()
    size_t sram_size;            // stlink_chipid_params.sram_size, set by stlink_load_device_params()

    /* option settings */
    stm32_addr_t option_base;
    size_t option_size;

    // bootloader
    // sys_base and sys_size are not used by the tools, but are only there to download the bootloader code
    // (see tests/sg.c)
    stm32_addr_t sys_base;       // stlink_chipid_params.bootrom_base, set by stlink_load_device_params()
    size_t sys_size;             // stlink_chipid_params.bootrom_size, set by stlink_load_device_params()

    struct stlink_version_ version;

    uint32_t chip_flags;         // stlink_chipid_params.flags, set by stlink_load_device_params(), values: CHIP_F_xxx

    uint32_t max_trace_freq;     // set by stlink_open_usb()
};

int stlink_enter_swd_mode(stlink_t *sl);
int stlink_enter_jtag_mode(stlink_t *sl);
int stlink_exit_debug_mode(stlink_t *sl);
int stlink_exit_dfu_mode(stlink_t *sl);
void stlink_close(stlink_t *sl);
int stlink_core_id(stlink_t *sl);
int stlink_reset(stlink_t *sl, enum reset_type type);
int stlink_run(stlink_t *sl, enum run_type type);
int stlink_status(stlink_t *sl);
int stlink_version(stlink_t *sl);
int stlink_read_debug32(stlink_t *sl, uint32_t addr, uint32_t *data);
int stlink_read_mem32(stlink_t *sl, uint32_t addr, uint16_t len);
int stlink_write_debug32(stlink_t *sl, uint32_t addr, uint32_t data);
int stlink_write_mem32(stlink_t *sl, uint32_t addr, uint16_t len);
int stlink_write_mem8(stlink_t *sl, uint32_t addr, uint16_t len);
int stlink_read_all_regs(stlink_t *sl, struct stlink_reg *regp);
int stlink_read_all_unsupported_regs(stlink_t *sl, struct stlink_reg *regp);
int stlink_read_reg(stlink_t *sl, int r_idx, struct stlink_reg *regp);
int stlink_read_unsupported_reg(stlink_t *sl, int r_idx, struct stlink_reg *regp);
int stlink_write_unsupported_reg(stlink_t *sl, uint32_t value, int r_idx, struct stlink_reg *regp);
int stlink_write_reg(stlink_t *sl, uint32_t reg, int idx);
int stlink_step(stlink_t *sl);
int stlink_current_mode(stlink_t *sl);
int stlink_force_debug(stlink_t *sl);
int stlink_target_voltage(stlink_t *sl);
int stlink_set_swdclk(stlink_t *sl, int freq_khz);
int stlink_trace_enable(stlink_t* sl, uint32_t frequency);
int stlink_trace_disable(stlink_t* sl);
int stlink_trace_read(stlink_t* sl, uint8_t* buf, size_t size);
int stlink_erase_flash_mass(stlink_t* sl);
int stlink_erase_flash_section(stlink_t *sl, stm32_addr_t base_addr, size_t size, bool align_size);
int stlink_write_flash(stlink_t* sl, stm32_addr_t address, uint8_t* data, uint32_t length, uint8_t eraseonly);
int stlink_parse_ihex(const char* path, uint8_t erased_pattern, uint8_t * * mem, size_t * size, uint32_t * begin);
uint8_t stlink_get_erased_pattern(stlink_t *sl);
int stlink_mwrite_flash(stlink_t *sl, uint8_t* data, uint32_t length, stm32_addr_t addr);
int stlink_fwrite_flash(stlink_t *sl, const char* path, stm32_addr_t addr);
int stlink_mwrite_sram(stlink_t *sl, uint8_t* data, uint32_t length, stm32_addr_t addr);
int stlink_fwrite_sram(stlink_t *sl, const char* path, stm32_addr_t addr);
int stlink_verify_write_flash(stlink_t *sl, stm32_addr_t address, uint8_t *data, uint32_t length);

int stlink_chip_id(stlink_t *sl, uint32_t *chip_id);
int stlink_cpu_id(stlink_t *sl, cortex_m3_cpuid_t *cpuid);

int stlink_erase_flash_page(stlink_t* sl, stm32_addr_t flashaddr);
uint32_t stlink_calculate_pagesize(stlink_t *sl, uint32_t flashaddr);
int stlink_check_address_range_validity(stlink_t *sl, stm32_addr_t addr, size_t size);
int stlink_check_address_alignment(stlink_t *sl, stm32_addr_t addr);
uint16_t read_uint16(const unsigned char *c, const int pt);
void stlink_core_stat(stlink_t *sl);
void stlink_print_data(stlink_t *sl);
unsigned int is_bigendian(void);
uint32_t read_uint32(const unsigned char *c, const int pt);
void write_uint32(unsigned char* buf, uint32_t ui);
void write_uint16(unsigned char* buf, uint16_t ui);
bool stlink_is_core_halted(stlink_t *sl);
int write_buffer_to_sram(stlink_t *sl, flash_loader_t* fl, const uint8_t* buf, size_t size);
int write_loader_to_sram(stlink_t *sl, stm32_addr_t* addr, size_t* size);
int stlink_fread(stlink_t* sl, const char* path, bool is_ihex, stm32_addr_t addr, size_t size);
int stlink_load_device_params(stlink_t *sl);

int stlink_read_option_bytes32(stlink_t *sl, uint32_t* option_byte);
int stlink_read_option_bytes_boot_add32(stlink_t *sl, uint32_t* option_byte);
int stlink_read_option_control_register32(stlink_t *sl, uint32_t* option_byte);
int stlink_read_option_control_register1_32(stlink_t *sl, uint32_t* option_byte);

int stlink_write_option_bytes32(stlink_t *sl, uint32_t option_byte);
int stlink_write_option_bytes_boot_add32(stlink_t *sl, uint32_t option_bytes_boot_add);
int stlink_write_option_control_register32(stlink_t *sl, uint32_t option_control_register);
int stlink_write_option_control_register1_32(stlink_t *sl, uint32_t option_control_register1);

int stlink_write_option_bytes(stlink_t *sl, stm32_addr_t addr, uint8_t* base, uint32_t len);
int stlink_fwrite_option_bytes(stlink_t *sl, const char* path, stm32_addr_t addr);

int stlink_flashloader_start(stlink_t *sl, flash_loader_t *fl);
int stlink_flashloader_write(stlink_t *sl, flash_loader_t *fl, stm32_addr_t addr, uint8_t* base, uint32_t len);
int stlink_flashloader_stop(stlink_t *sl, flash_loader_t *fl);

int stlink_target_connect(stlink_t *sl, enum connect_type connect);

#include <sg.h>
#include <usb.h>
#include <reg.h>
#include <commands.h>
#include <chipid.h>
#include <flash_loader.h>
#include <version.h>
#include <logging.h>

#ifdef __cplusplus
}
#endif

#endif // STLINK_H
