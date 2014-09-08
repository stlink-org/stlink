/*
 * File:   stlink-common.h
 * Bulk import from stlink-hw.h
 *
 * This should contain all the common top level stlink interfaces, regardless
 * of how the backend does the work....
 */

#ifndef STLINK_COMMON_H
#define STLINK_COMMON_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdint.h>

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

    /*
     * Chip IDs are explained in the appropriate programming manual for the
     * DBGMCU_IDCODE register (0xE0042000)
     */
    // stm32 chipids, only lower 12 bits..
#define STM32_CHIPID_F1_MEDIUM      0x410
#define STM32_CHIPID_F2             0x411
#define STM32_CHIPID_F1_LOW         0x412
#define STM32_CHIPID_F4             0x413
#define STM32_CHIPID_F1_HIGH        0x414

#define STM32_CHIPID_L1_MEDIUM      0x416
#define STM32_CHIPID_L0             0x417
#define STM32_CHIPID_F1_CONN        0x418
#define STM32_CHIPID_F4_HD          0x419
#define STM32_CHIPID_F1_VL_MEDIUM   0x420

#define STM32_CHIPID_F3             0x422
#define STM32_CHIPID_F4_LP          0x423

#define STM32_CHIPID_F411RE         0x431

#define STM32_CHIPID_L1_MEDIUM_PLUS 0x427
#define STM32_CHIPID_F1_VL_HIGH     0x428

#define STM32_CHIPID_F1_XL          0x430

#define STM32_CHIPID_F37x           0x432
#define STM32_CHIPID_F4_DE          0x433

#define STM32_CHIPID_L1_HIGH        0x436
#define STM32_CHIPID_L152_RE        0x437
#define STM32_CHIPID_F334           0x438

#define STM32_CHIPID_F3_SMALL       0x439
#define STM32_CHIPID_F0             0x440

#define STM32_CHIPID_F0_SMALL       0x444

#define STM32_CHIPID_F0_CAN         0x448

    /*
     * 0x436 is actually assigned to some L1 chips that are called "Medium-Plus"
     * and some that are called "High".  0x427 is assigned to the other "Medium-
     * plus" chips.  To make it a bit simpler we just call 427 MEDIUM_PLUS and
     * 0x436 HIGH.
     */

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

    typedef struct chip_params_ {
        uint32_t chip_id;
        char* description;
        uint32_t flash_size_reg;
        uint32_t flash_pagesize;
        uint32_t sram_size;
        uint32_t bootrom_base, bootrom_size;
    } chip_params_t;


    // These maps are from a combination of the Programming Manuals, and
    // also the Reference manuals.  (flash size reg is normally in ref man)
    static const chip_params_t devices[] = {
        { // table 2, PM0063
            .chip_id = STM32_CHIPID_F1_MEDIUM,
            .description = "F1 Medium-density device",
            .flash_size_reg = 0x1ffff7e0,
            .flash_pagesize = 0x400,
            .sram_size = 0x5000,
            .bootrom_base = 0x1ffff000,
            .bootrom_size = 0x800
        },
        {  // table 1, PM0059
            .chip_id = STM32_CHIPID_F2,
            .description = "F2 device",
            .flash_size_reg = 0x1fff7a22, /* As in RM0033 Rev 5*/
            .flash_pagesize = 0x20000,
            .sram_size = 0x20000,
            .bootrom_base = 0x1fff0000,
            .bootrom_size = 0x7800
        },
        { // PM0063
            .chip_id = STM32_CHIPID_F1_LOW,
            .description = "F1 Low-density device",
            .flash_size_reg = 0x1ffff7e0,
            .flash_pagesize = 0x400,
            .sram_size = 0x2800,
            .bootrom_base = 0x1ffff000,
            .bootrom_size = 0x800
        },
        {
            .chip_id = STM32_CHIPID_F4,
            .description = "F4 device",
            .flash_size_reg = 0x1FFF7A22,  /* As in rm0090 since Rev 2*/
            .flash_pagesize = 0x4000,
            .sram_size = 0x30000,
            .bootrom_base = 0x1fff0000,
            .bootrom_size = 0x7800
        },
        {
            .chip_id = STM32_CHIPID_F4_HD,
            .description = "F42x and F43x device",
            .flash_size_reg = 0x1FFF7A22,  /* As in rm0090 since Rev 2*/
            .flash_pagesize = 0x4000,
            .sram_size = 0x30000,
            .bootrom_base = 0x1fff0000,
            .bootrom_size = 0x7800
        },
        {
            .chip_id = STM32_CHIPID_F4_LP,
            .description = "F4 device (low power)",
            .flash_size_reg = 0x1FFF7A22,
            .flash_pagesize = 0x4000,
            .sram_size = 0x10000,
            .bootrom_base = 0x1fff0000,
            .bootrom_size = 0x7800
        },
        {
            .chip_id = STM32_CHIPID_F411RE,
            .description = "F4 device (low power) - stm32f411re",
            .flash_size_reg = 0x1FFF7A22,
            .flash_pagesize = 0x4000,
            .sram_size = 0x20000,
            .bootrom_base = 0x1fff0000,
            .bootrom_size = 0x7800
        },
        {
            .chip_id = STM32_CHIPID_F4_DE,
            .description = "F4 device (Dynamic Efficency)",
            .flash_size_reg = 0x1FFF7A22,
            .flash_pagesize = 0x4000,
            .sram_size = 0x18000,
            .bootrom_base = 0x1fff0000,
            .bootrom_size = 0x7800
        },
        {
            .chip_id = STM32_CHIPID_F1_HIGH,
            .description = "F1 High-density device",
            .flash_size_reg = 0x1ffff7e0,
            .flash_pagesize = 0x800,
            .sram_size = 0x10000,
            .bootrom_base = 0x1ffff000,
            .bootrom_size = 0x800
        },
        {
            // This ignores the EEPROM! (and uses the page erase size,
            // not the sector write protection...)
            .chip_id = STM32_CHIPID_L1_MEDIUM,
            .description = "L1 Med-density device",
            .flash_size_reg = 0x1ff8004c,
            .flash_pagesize = 0x100,
            .sram_size = 0x4000,
            .bootrom_base = 0x1ff00000,
            .bootrom_size = 0x1000
        },
        {
            .chip_id = STM32_CHIPID_L1_MEDIUM_PLUS,
            .description = "L1 Medium-Plus-density device",
            .flash_size_reg = 0x1ff800cc,
            .flash_pagesize = 0x100,
            .sram_size = 0x8000,/*Not completely clear if there are some with 48K*/
            .bootrom_base = 0x1ff00000,
            .bootrom_size = 0x1000
        },
        {
            .chip_id = STM32_CHIPID_L1_HIGH,
            .description = "L1 High-density device",
            .flash_size_reg = 0x1ff800cc,
            .flash_pagesize = 0x100,
            .sram_size = 0xC000, /*Not completely clear if there are some with 32K*/
            .bootrom_base = 0x1ff00000,
            .bootrom_size = 0x1000
        },
        {
            .chip_id = STM32_CHIPID_L152_RE,
            .description = "L152RE",
            .flash_size_reg = 0x1ff800cc,
            .flash_pagesize = 0x100,
            .sram_size = 0x14000, /*Not completely clear if there are some with 32K*/
            .bootrom_base = 0x1ff00000,
            .bootrom_size = 0x1000
        },
        {
            .chip_id = STM32_CHIPID_F1_CONN,
            .description = "F1 Connectivity line device",
            .flash_size_reg = 0x1ffff7e0,
            .flash_pagesize = 0x800,
            .sram_size = 0x10000,
            .bootrom_base = 0x1fffb000,
            .bootrom_size = 0x4800
        },
        {
            .chip_id = STM32_CHIPID_F1_VL_MEDIUM,
            .description = "F1 Medium-density Value Line device",
            .flash_size_reg = 0x1ffff7e0,
            .flash_pagesize = 0x400,
            .sram_size = 0x2000,
            .bootrom_base = 0x1ffff000,
            .bootrom_size = 0x800
        },
        {
            // This is STK32F303VCT6 device from STM32 F3 Discovery board.
            // Support based on DM00043574.pdf (RM0316) document.
            .chip_id = STM32_CHIPID_F3,
            .description = "F3 device",
            .flash_size_reg = 0x1ffff7cc,
            .flash_pagesize = 0x800,
            .sram_size = 0xa000,
            .bootrom_base = 0x1ffff000,
            .bootrom_size = 0x800
        },
        {
            // This is STK32F373VCT6 device from STM32 F373 eval board
            // Support based on 303 above (37x and 30x have same memory map)
            .chip_id = STM32_CHIPID_F37x,
            .description = "F3 device",
            .flash_size_reg = 0x1ffff7cc,
            .flash_pagesize = 0x800,
            .sram_size = 0xa000,
            .bootrom_base = 0x1ffff000,
            .bootrom_size = 0x800
        },
        {
            .chip_id = STM32_CHIPID_F1_VL_HIGH,
            .description = "F1 High-density value line device",
            .flash_size_reg = 0x1ffff7e0,
            .flash_pagesize = 0x800,
            .sram_size = 0x8000,
            .bootrom_base = 0x1ffff000,
            .bootrom_size = 0x800
        },
        {
            .chip_id = STM32_CHIPID_F1_XL,
            .description = "F1 XL-density device",
            .flash_size_reg = 0x1ffff7e0,
            .flash_pagesize = 0x800,
            .sram_size = 0x18000,
            .bootrom_base = 0x1fffe000,
            .bootrom_size = 0x1800
        },
        {
            //Use this as an example for mapping future chips:
            //RM0091 document was used to find these paramaters
            .chip_id = STM32_CHIPID_F0_CAN,
            .description = "F07x device",
            .flash_size_reg = 0x1ffff7cc,      // "Flash size data register" (pg735)
            .flash_pagesize = 0x800,           // Page sizes listed in Table 4
            .sram_size = 0x4000,               // "SRAM" byte size in hex from Table 2
            .bootrom_base = 0x1fffC800,                // "System memory" starting address from Table 2
            .bootrom_size = 0x3000             // "System memory" byte size in hex from Table 2
        },
        {
            //Use this as an example for mapping future chips:
            //RM0091 document was used to find these paramaters
            .chip_id = STM32_CHIPID_F0,
            .description = "F0 device",
            .flash_size_reg = 0x1ffff7cc,	// "Flash size data register" (pg735)
            .flash_pagesize = 0x400,		// Page sizes listed in Table 4
            .sram_size = 0x2000,		// "SRAM" byte size in hex from Table 2
            .bootrom_base = 0x1fffec00,		// "System memory" starting address from Table 2
            .bootrom_size = 0xC00 		// "System memory" byte size in hex from Table 2
        },
        {
            //Use this as an example for mapping future chips:
            //RM0091 document was used to find these paramaters
            .chip_id = STM32_CHIPID_F0_SMALL,
            .description = "F0 small device",
            .flash_size_reg = 0x1ffff7cc,	// "Flash size data register" (pg735)
            .flash_pagesize = 0x400,		// Page sizes listed in Table 4
            .sram_size = 0x1000,		// "SRAM" byte size in hex from Table 2
            .bootrom_base = 0x1fffec00,		// "System memory" starting address from Table 2
            .bootrom_size = 0xC00 		// "System memory" byte size in hex from Table 2
        },
        {
            // STM32F30x
            .chip_id = STM32_CHIPID_F3_SMALL,
            .description = "F3 small device",
            .flash_size_reg = 0x1ffff7cc,
            .flash_pagesize = 0x800,
            .sram_size = 0xa000,
            .bootrom_base = 0x1fffd800,
            .bootrom_size = 0x2000
        },
        {
            // STM32L0x
            // RM0367,RM0377 documents was used to find these parameters
            .chip_id = STM32_CHIPID_L0,
            .description = "L0x3 device",
            .flash_size_reg = 0x1ff8007c,
            .flash_pagesize = 0x80,
            .sram_size = 0x2000,
            .bootrom_base = 0x1ff0000,
            .bootrom_size = 0x1000
        },
        {
            // STM32F334
            // RM0364 document was used to find these parameters
            .chip_id = STM32_CHIPID_F334,
            .description = "F334 device",
            .flash_size_reg = 0x1ffff7cc,
            .flash_pagesize = 0x800,
            .sram_size = 0x3000,
            .bootrom_base = 0x1fffd800,
            .bootrom_size = 0x2000
        },

 };


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
        void (*exit_debug_mode) (stlink_t * sl);
        void (*enter_swd_mode) (stlink_t * sl);
        void (*enter_jtag_mode) (stlink_t * stl);
        void (*exit_dfu_mode) (stlink_t * stl);
        void (*core_id) (stlink_t * stl);
        void (*reset) (stlink_t * stl);
        void (*jtag_reset) (stlink_t * stl, int value);
        void (*run) (stlink_t * stl);
        void (*status) (stlink_t * stl);
        void (*version) (stlink_t *sl);
        uint32_t (*read_debug32) (stlink_t *sl, uint32_t addr);
        void (*read_mem32) (stlink_t *sl, uint32_t addr, uint16_t len);
        void (*write_debug32) (stlink_t *sl, uint32_t addr, uint32_t data);
        void (*write_mem32) (stlink_t *sl, uint32_t addr, uint16_t len);
        void (*write_mem8) (stlink_t *sl, uint32_t addr, uint16_t len);
        void (*read_all_regs) (stlink_t *sl, reg * regp);
        void (*read_reg) (stlink_t *sl, int r_idx, reg * regp);
        void (*read_all_unsupported_regs) (stlink_t *sl, reg *regp);
        void (*read_unsupported_reg) (stlink_t *sl, int r_idx, reg *regp);
        void (*write_unsupported_reg) (stlink_t *sl, uint32_t value, int idx, reg *regp);
        void (*write_reg) (stlink_t *sl, uint32_t reg, int idx);
        void (*step) (stlink_t * stl);
        int (*current_mode) (stlink_t * stl);
        void (*force_debug) (stlink_t *sl);
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

#define STM32_FLASH_PGSZ 1024
#define STM32L_FLASH_PGSZ 256

#define STM32F4_FLASH_PGSZ 16384
#define STM32F4_FLASH_SIZE (128 * 1024 * 8)

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
    void stlink_enter_swd_mode(stlink_t *sl);
    void stlink_enter_jtag_mode(stlink_t *sl);
    void stlink_exit_debug_mode(stlink_t *sl);
    void stlink_exit_dfu_mode(stlink_t *sl);
    void stlink_close(stlink_t *sl);
    uint32_t stlink_core_id(stlink_t *sl);
    void stlink_reset(stlink_t *sl);
    void stlink_jtag_reset(stlink_t *sl, int value);
    void stlink_run(stlink_t *sl);
    void stlink_status(stlink_t *sl);
    void stlink_version(stlink_t *sl);
    uint32_t stlink_read_debug32(stlink_t *sl, uint32_t addr);
    void stlink_read_mem32(stlink_t *sl, uint32_t addr, uint16_t len);
    void stlink_write_debug32(stlink_t *sl, uint32_t addr, uint32_t data);
    void stlink_write_mem32(stlink_t *sl, uint32_t addr, uint16_t len);
    void stlink_write_mem8(stlink_t *sl, uint32_t addr, uint16_t len);
    void stlink_read_all_regs(stlink_t *sl, reg *regp);
    void stlink_read_all_unsupported_regs(stlink_t *sl, reg *regp);
    void stlink_read_reg(stlink_t *sl, int r_idx, reg *regp);
    void stlink_read_unsupported_reg(stlink_t *sl, int r_idx, reg *regp);
    void stlink_write_unsupported_reg(stlink_t *sl, uint32_t value, int r_idx, reg *regp);
    void stlink_write_reg(stlink_t *sl, uint32_t reg, int idx);
    void stlink_step(stlink_t *sl);
    int stlink_current_mode(stlink_t *sl);
    void stlink_force_debug(stlink_t *sl);
    int stlink_target_voltage(stlink_t *sl);


    // unprocessed
    int stlink_erase_flash_mass(stlink_t* sl);
    int stlink_write_flash(stlink_t* sl, stm32_addr_t address, uint8_t* data, uint32_t length);
    int stlink_fwrite_flash(stlink_t *sl, const char* path, stm32_addr_t addr);
    int stlink_fwrite_sram(stlink_t *sl, const char* path, stm32_addr_t addr);
    int stlink_verify_write_flash(stlink_t *sl, stm32_addr_t address, uint8_t *data, uint32_t length);

    // PUBLIC
    uint32_t stlink_chip_id(stlink_t *sl);
    void stlink_cpu_id(stlink_t *sl, cortex_m3_cpuid_t *cpuid);

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



#include "stlink-sg.h"
#include "stlink-usb.h"



#ifdef	__cplusplus
}
#endif

#endif	/* STLINK_COMMON_H */
