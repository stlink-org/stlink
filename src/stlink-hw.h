#ifndef _STLINK_HW_H_
#define _STLINK_HW_H_

#include <stdint.h>

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

struct stlink {
	int sg_fd;
	int do_scsi_pt_err;
	// sg layer verboseness: 0 for no debug info, 10 for lots
	int verbose;

	unsigned char cdb_cmd_blk[CDB_SL];

	// Data transferred from or to device
	unsigned char q_buf[Q_BUF_LEN];
	int q_len;
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

struct stlink* stlink_quirk_open(const char *dev_name, const int verbose);
int stlink_current_mode(struct stlink *sl);
void stlink_enter_swd_mode(struct stlink *sl);
void stlink_enter_jtag_mode(struct stlink *sl);
void stlink_exit_debug_mode(struct stlink *sl);
void stlink_core_id(struct stlink *sl);
void stlink_status(struct stlink *sl);
void stlink_force_debug(struct stlink *sl);
void stlink_reset(struct stlink *sl);
void stlink_run(struct stlink *sl);
void stlink_step(struct stlink *sl);
void stlink_read_all_regs(struct stlink *sl);
void stlink_read_reg(struct stlink *sl, int r_idx);
void stlink_write_reg(struct stlink *sl, uint32_t reg, int idx);
void stlink_read_mem32(struct stlink *sl, uint32_t addr, uint16_t len);
void stlink_write_mem8(struct stlink *sl, uint32_t addr, uint16_t len);
void stlink_write_mem32(struct stlink *sl, uint32_t addr, uint16_t len);
void stlink_close(struct stlink *sl);

int stlink_erase_flash_page(struct stlink* sl, stm32_addr_t page);
int stlink_erase_flash_mass(struct stlink* sl);
int stlink_write_flash(struct stlink* sl, stm32_addr_t address, uint8_t* data, unsigned length);

#endif
