/* 
 * File:   stlink-common.h
 * Bulk import from stlink-hw.h
 * 
 * This should contain all the common top level stlink interfaces, regardless
 * of how the backend does the work....
 */

#ifndef STLINK_COMMON_H
#define	STLINK_COMMON_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdint.h>

    // Max data transfer size.
    // 6kB = max mem32_read block, 8kB sram
    //#define Q_BUF_LEN	96
#define Q_BUF_LEN	1024 * 100

    // st-link vendor cmd's
#define USB_ST_VID			0x0483
#define USB_STLINK_PID			0x3744

    // STLINK_DEBUG_RESETSYS, etc:
#define STLINK_OK			0x80
#define STLINK_FALSE			0x81
#define STLINK_CORE_RUNNING		0x80
#define STLINK_CORE_HALTED		0x81
#define STLINK_CORE_STAT_UNKNOWN	-1

#define STLINK_GET_VERSION		0xf1
#define STLINK_GET_CURRENT_MODE	0xf5

#define STLINK_DEBUG_COMMAND		0xF2
#define STLINK_DFU_COMMAND		0xF3
#define STLINK_DFU_EXIT		0x07

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

    typedef struct {
        uint32_t r[16];
        uint32_t xpsr;
        uint32_t main_sp;
        uint32_t process_sp;
        uint32_t rw;
        uint32_t rw2;
    } reg;

    typedef uint32_t stm32_addr_t;

    enum transport_type {
        TRANSPORT_TYPE_ZERO = 0,
        TRANSPORT_TYPE_LIBSG,
        TRANSPORT_TYPE_LIBUSB,
        TRANSPORT_TYPE_INVALID
    };

    typedef struct _stlink stlink_t;
    
    typedef struct _stlink_backend {
        void (*close) (stlink_t* sl);
        void (*exit_debug_mode) (stlink_t *sl);
        void (*enter_swd_mode) (stlink_t *sl);
        void (*enter_jtag_mode) (stlink_t *stl);
        void (*exit_dfu_mode) (stlink_t *stl);
        void (*core_id) (stlink_t *stl);
        void (*reset) (stlink_t *stl);
        void (*run) (stlink_t *stl);
        void (*status) (stlink_t *stl);
        void (*version) (stlink_t *stl);
        void (*write_mem32) (stlink_t *sl, uint32_t addr, uint16_t len);
        void (*write_mem8) (stlink_t *sl, uint32_t addr, uint16_t len);
    } stlink_backend_t;

    struct _stlink {
        struct _stlink_backend *backend;
        void *backend_data;

        // Data transferred from or to device
        unsigned char q_buf[Q_BUF_LEN];
        int q_len;

        // transport layer verboseness: 0 for no debug info, 10 for lots
        int verbose;
        uint32_t core_id;
        int core_stat;

        
        
        /* medium density stm32 flash settings */
#define STM32_FLASH_BASE 0x08000000
#define STM32_FLASH_SIZE (128 * 1024)
#define STM32_FLASH_PGSZ 1024
        stm32_addr_t flash_base;
        size_t flash_size;
        size_t flash_pgsz;

        /* in flash system memory */
#define STM32_SYSTEM_BASE 0x1ffff000
#define STM32_SYSTEM_SIZE (2 * 1024)
        stm32_addr_t sys_base;
        size_t sys_size;

        /* sram settings */
#define STM32_SRAM_BASE 0x20000000
#define STM32_SRAM_SIZE (8 * 1024)
        stm32_addr_t sram_base;
        size_t sram_size;

    };

    // some quick and dirty logging...
    void D(stlink_t *sl, char *txt);
    void DD(stlink_t *sl, char *format, ...);

    //stlink_t* stlink_quirk_open(const char *dev_name, const int verbose);
    
    // delegated functions...
    void stlink_enter_swd_mode(stlink_t *sl);
    void stlink_enter_jtag_mode(stlink_t *sl);
    void stlink_exit_debug_mode(stlink_t *sl);
    void stlink_exit_dfu_mode(stlink_t *sl);
    void stlink_close(stlink_t *sl);
    void stlink_core_id(stlink_t *sl);
    void stlink_reset(stlink_t *sl);
    void stlink_run(stlink_t *sl);
    void stlink_status(stlink_t *sl);
    void stlink_version(stlink_t *sl);
    void stlink_write_mem32(stlink_t *sl, uint32_t addr, uint16_t len);
    void stlink_write_mem8(stlink_t *sl, uint32_t addr, uint16_t len);
    

    // unprocessed
    int stlink_current_mode(stlink_t *sl);
    void stlink_force_debug(stlink_t *sl);
    void stlink_step(stlink_t *sl);
    void stlink_read_all_regs(stlink_t *sl);
    void stlink_read_reg(stlink_t *sl, int r_idx);
    void stlink_write_reg(stlink_t *sl, uint32_t reg, int idx);
    void stlink_read_mem32(stlink_t *sl, uint32_t addr, uint16_t len);

    int stlink_erase_flash_page(stlink_t* sl, stm32_addr_t page);
    int stlink_erase_flash_mass(stlink_t* sl);
    int stlink_write_flash(stlink_t* sl, stm32_addr_t address, uint8_t* data, unsigned length);

    // privates....
    uint16_t read_uint16(const unsigned char *c, const int pt);
    void stlink_core_stat(stlink_t *sl);
    void stlink_print_data(stlink_t *sl);
    unsigned int is_bigendian(void);


#include "stlink-sg.h"
#include "stlink-usb.h"    



#ifdef	__cplusplus
}
#endif

#endif	/* STLINK_COMMON_H */

