/*
 * File:   stlink.h
 *
 * This should contain all the common top level stlink interfaces, regardless
 * of how the backend does the work....
 */
#ifndef STLINK_H
#define STLINK_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STLINK_ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

    // Max data transfer size.
    // 6kB = max mem32_read block, 8kB sram
    //#define Q_BUF_LEN	96
#define Q_BUF_LEN			(1024 * 100)

    // st-link vendor cmd's
#define USB_ST_VID			0x0483
#define USB_STLINK_PID			0x3744
#define USB_STLINK_32L_PID		0x3748
#define USB_STLINK_NUCLEO_PID	0x374b

    // STLINK_DEBUG_RESETSYS, etc:
#define STLINK_OK			0x80
#define STLINK_FALSE			0x81
#define STLINK_CORE_RUNNING		0x80
#define STLINK_CORE_HALTED		0x81
#define STLINK_CORE_STAT_UNKNOWN	-1

#define STLINK_GET_VERSION		0xf1
#define STLINK_GET_CURRENT_MODE	0xf5
#define STLINK_GET_TARGET_VOLTAGE	0xF7

#define STLINK_DEBUG_COMMAND		0xF2
#define STLINK_DFU_COMMAND		0xF3
#define STLINK_DFU_EXIT		0x07
    // enter dfu could be 0x08?

    // STLINK_GET_CURRENT_MODE
#define STLINK_DEV_DFU_MODE		0x00
#define STLINK_DEV_MASS_MODE		0x01
#define STLINK_DEV_DEBUG_MODE		0x02
#define STLINK_DEV_UNKNOWN_MODE	-1

    // jtag mode cmds
#define STLINK_DEBUG_ENTER		0x20
#define STLINK_DEBUG_EXIT		0x21
#define STLINK_DEBUG_READCOREID	0x22
#define STLINK_DEBUG_GETSTATUS		0x01
#define STLINK_DEBUG_FORCEDEBUG	0x02
#define STLINK_DEBUG_RESETSYS		0x03
#define STLINK_DEBUG_READALLREGS	0x04
#define STLINK_DEBUG_READREG		0x05
#define STLINK_DEBUG_WRITEREG		0x06
#define STLINK_DEBUG_READMEM_32BIT	0x07
#define STLINK_DEBUG_WRITEMEM_32BIT	0x08
#define STLINK_DEBUG_RUNCORE		0x09
#define STLINK_DEBUG_STEPCORE		0x0a
#define STLINK_DEBUG_SETFP		0x0b
#define STLINK_DEBUG_WRITEMEM_8BIT	0x0d
#define STLINK_DEBUG_CLEARFP		0x0e
#define STLINK_DEBUG_WRITEDEBUGREG	0x0f
#define STLINK_DEBUG_ENTER_SWD		0xa3
#define STLINK_DEBUG_ENTER_JTAG	0x00

    // TODO - possible poor names...
#define STLINK_SWD_ENTER 0x30
#define STLINK_SWD_READCOREID 0x32  // TBD
#define STLINK_JTAG_WRITEDEBUG_32BIT 0x35
#define STLINK_JTAG_READDEBUG_32BIT 0x36
#define STLINK_JTAG_DRIVE_NRST 0x3c
#define STLINK_JTAG_DRIVE_NRST 0x3c

    // cortex m3 technical reference manual
#define CM3_REG_CPUID 0xE000ED00
#define CM3_REG_FP_CTRL 0xE0002000
#define CM3_REG_FP_COMP0 0xE0002008

    /* cortex core ids */
    // TODO clean this up...
#define STM32VL_CORE_ID 0x1ba01477
#define STM32L_CORE_ID 0x2ba01477
#define STM32F3_CORE_ID 0x2ba01477
#define STM32F4_CORE_ID 0x2ba01477
#define STM32F0_CORE_ID 0xbb11477
#define CORE_M3_R1 0x1BA00477
#define CORE_M3_R2 0x4BA00477
#define CORE_M4_R0 0x2BA01477

    // Constant STM32 memory map figures
#define STM32_FLASH_BASE 0x08000000
#define STM32_SRAM_BASE 0x20000000

    /* Cortex™-M3 Technical Reference Manual */
    /* Debug Halting Control and Status Register */
#define DHCSR 0xe000edf0
#define DCRSR 0xe000edf4
#define DCRDR 0xe000edf8
#define DBGKEY 0xa05f0000

    /* Enough space to hold both a V2 command or a V1 command packaged as generic scsi*/
#define C_BUF_LEN 32

    enum flash_type {
        FLASH_TYPE_UNKNOWN = 0,
        FLASH_TYPE_F0,
        FLASH_TYPE_L0,
        FLASH_TYPE_F4,
        FLASH_TYPE_L4,
    };

#include "stlink/chipid.h"

    typedef struct {
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
    } reg;

    typedef uint32_t stm32_addr_t;

    typedef struct _cortex_m3_cpuid_ {
        uint16_t implementer_id;
        uint16_t variant;
        uint16_t part;
        uint8_t revision;
    } cortex_m3_cpuid_t;

    typedef struct stlink_version_ {
        uint32_t stlink_v;
        uint32_t jtag_v;
        uint32_t swim_v;
        uint32_t st_vid;
        uint32_t stlink_pid;
    } stlink_version_t;

    typedef struct flash_loader {
        stm32_addr_t loader_addr; /* loader sram adddr */
        stm32_addr_t buf_addr; /* buffer sram address */
    } flash_loader_t;

    enum transport_type {
        TRANSPORT_TYPE_ZERO = 0,
        TRANSPORT_TYPE_LIBSG,
        TRANSPORT_TYPE_LIBUSB,
        TRANSPORT_TYPE_INVALID
    };

    typedef struct _stlink stlink_t;

    typedef struct _stlink_backend {
        void (*close) (stlink_t * sl);
        int (*exit_debug_mode) (stlink_t * sl);
        int (*enter_swd_mode) (stlink_t * sl);
        int (*enter_jtag_mode) (stlink_t * stl);
        int (*exit_dfu_mode) (stlink_t * stl);
        int (*core_id) (stlink_t * stl);
        int (*reset) (stlink_t * stl);
        int (*jtag_reset) (stlink_t * stl, int value);
        int (*run) (stlink_t * stl);
        int (*status) (stlink_t * stl);
        int (*version) (stlink_t *sl);
        int (*read_debug32) (stlink_t *sl, uint32_t addr, uint32_t *data);
        int (*read_mem32) (stlink_t *sl, uint32_t addr, uint16_t len);
        int (*write_debug32) (stlink_t *sl, uint32_t addr, uint32_t data);
        int (*write_mem32) (stlink_t *sl, uint32_t addr, uint16_t len);
        int (*write_mem8) (stlink_t *sl, uint32_t addr, uint16_t len);
        int (*read_all_regs) (stlink_t *sl, reg * regp);
        int (*read_reg) (stlink_t *sl, int r_idx, reg * regp);
        int (*read_all_unsupported_regs) (stlink_t *sl, reg *regp);
        int (*read_unsupported_reg) (stlink_t *sl, int r_idx, reg *regp);
        int (*write_unsupported_reg) (stlink_t *sl, uint32_t value, int idx, reg *regp);
        int (*write_reg) (stlink_t *sl, uint32_t reg, int idx);
        int (*step) (stlink_t * stl);
        int (*current_mode) (stlink_t * stl);
        int (*force_debug) (stlink_t *sl);
        int32_t (*target_voltage) (stlink_t *sl);
    } stlink_backend_t;

    struct _stlink {
        struct _stlink_backend *backend;
        void *backend_data;

        // Room for the command header
        unsigned char c_buf[C_BUF_LEN];
        // Data transferred from or to device
        unsigned char q_buf[Q_BUF_LEN];
        int q_len;

        // transport layer verboseness: 0 for no debug info, 10 for lots
        int verbose;
        uint32_t core_id;
        uint32_t chip_id;
        int core_stat;

        char serial[16];
        int serial_size;

#define STM32_FLASH_PGSZ 1024
#define STM32L_FLASH_PGSZ 256

#define STM32F4_FLASH_PGSZ 16384
#define STM32F4_FLASH_SIZE (128 * 1024 * 8)

        enum flash_type flash_type;
        stm32_addr_t flash_base;
        size_t flash_size;
        size_t flash_pgsz;

        /* sram settings */
#define STM32_SRAM_SIZE (8 * 1024)
#define STM32L_SRAM_SIZE (16 * 1024)
        stm32_addr_t sram_base;
        size_t sram_size;

        // bootloader
        stm32_addr_t sys_base;
        size_t sys_size;

        struct stlink_version_ version;
    };

    //stlink_t* stlink_quirk_open(const char *dev_name, const int verbose);

    // delegated functions...
    int stlink_enter_swd_mode(stlink_t *sl);
    int stlink_enter_jtag_mode(stlink_t *sl);
    int stlink_exit_debug_mode(stlink_t *sl);
    int stlink_exit_dfu_mode(stlink_t *sl);
    void stlink_close(stlink_t *sl);
    int stlink_core_id(stlink_t *sl);
    int stlink_reset(stlink_t *sl);
    int stlink_jtag_reset(stlink_t *sl, int value);
    int stlink_run(stlink_t *sl);
    int stlink_status(stlink_t *sl);
    int stlink_version(stlink_t *sl);
    int stlink_read_debug32(stlink_t *sl, uint32_t addr, uint32_t *data);
    int stlink_read_mem32(stlink_t *sl, uint32_t addr, uint16_t len);
    int stlink_write_debug32(stlink_t *sl, uint32_t addr, uint32_t data);
    int stlink_write_mem32(stlink_t *sl, uint32_t addr, uint16_t len);
    int stlink_write_mem8(stlink_t *sl, uint32_t addr, uint16_t len);
    int stlink_read_all_regs(stlink_t *sl, reg *regp);
    int stlink_read_all_unsupported_regs(stlink_t *sl, reg *regp);
    int stlink_read_reg(stlink_t *sl, int r_idx, reg *regp);
    int stlink_read_unsupported_reg(stlink_t *sl, int r_idx, reg *regp);
    int stlink_write_unsupported_reg(stlink_t *sl, uint32_t value, int r_idx, reg *regp);
    int stlink_write_reg(stlink_t *sl, uint32_t reg, int idx);
    int stlink_step(stlink_t *sl);
    int stlink_current_mode(stlink_t *sl);
    int stlink_force_debug(stlink_t *sl);
    int stlink_target_voltage(stlink_t *sl);


    // unprocessed
    int stlink_erase_flash_mass(stlink_t* sl);
    int stlink_write_flash(stlink_t* sl, stm32_addr_t address, uint8_t* data, uint32_t length, uint8_t eraseonly);
    int stlink_fwrite_flash(stlink_t *sl, const char* path, stm32_addr_t addr);
    int stlink_fwrite_sram(stlink_t *sl, const char* path, stm32_addr_t addr);
    int stlink_verify_write_flash(stlink_t *sl, stm32_addr_t address, uint8_t *data, uint32_t length);

    // PUBLIC
    int stlink_chip_id(stlink_t *sl, uint32_t *chip_id);
    int stlink_cpu_id(stlink_t *sl, cortex_m3_cpuid_t *cpuid);

    // privates, publics, the rest....
    // TODO sort what is private, and what is not
    int stlink_erase_flash_page(stlink_t* sl, stm32_addr_t flashaddr);
    uint32_t stlink_calculate_pagesize(stlink_t *sl, uint32_t flashaddr);
    uint16_t read_uint16(const unsigned char *c, const int pt);
    void stlink_core_stat(stlink_t *sl);
    void stlink_print_data(stlink_t *sl);
    unsigned int is_bigendian(void);
    uint32_t read_uint32(const unsigned char *c, const int pt);
    void write_uint32(unsigned char* buf, uint32_t ui);
    void write_uint16(unsigned char* buf, uint16_t ui);
    unsigned int is_core_halted(stlink_t *sl);
    int write_buffer_to_sram(stlink_t *sl, flash_loader_t* fl, const uint8_t* buf, size_t size);
    int write_loader_to_sram(stlink_t *sl, stm32_addr_t* addr, size_t* size);
    int stlink_fread(stlink_t* sl, const char* path, stm32_addr_t addr, size_t size);
    int run_flash_loader(stlink_t *sl, flash_loader_t* fl, stm32_addr_t target, const uint8_t* buf, size_t size);
    int stlink_load_device_params(stlink_t *sl);

#include "stlink/sg.h"
#include "stlink/usb.h"

#ifdef __cplusplus
}
#endif

#endif /* STLINK_H */
