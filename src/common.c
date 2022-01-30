#include <helper.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <md5.h>
#include <stlink.h>
#include <stm32.h>

#include "common_flash.h"
#include "calculate.h"
#include "map_file.h"
#include "common.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifdef _MSC_VER
#define __attribute__(x)
#endif

// Private structs and functions defines
struct stlink_fread_worker_arg {
  int fd;
};

struct stlink_fread_ihex_worker_arg {
  FILE *file;
  uint32_t addr;
  uint32_t lba;
  uint8_t buf[16];
  uint8_t buf_pos;
};

typedef bool (*save_block_fn)(void *arg, uint8_t *block, ssize_t len);

static void stop_wdg_in_debug(stlink_t *);
int stlink_jtag_reset(stlink_t *, int);
int stlink_soft_reset(stlink_t *, int);
void _parse_version(stlink_t *, stlink_version_t *);
static uint8_t stlink_parse_hex(const char *);
static int stlink_read(stlink_t *, stm32_addr_t, size_t, save_block_fn, void *);
static bool stlink_fread_ihex_init(struct stlink_fread_ihex_worker_arg *, int, stm32_addr_t);
static bool stlink_fread_ihex_worker(void *, uint8_t *, ssize_t);
static bool stlink_fread_ihex_finalize(struct stlink_fread_ihex_worker_arg *);
static bool stlink_fread_worker(void *, uint8_t *, ssize_t);
// End of private structs and functions defines

// Functions below are defined in stlink.h (see line num before function)
// 252
void stlink_close(stlink_t *sl) {
  DLOG("*** stlink_close ***\n");

  if (!sl) {
    return;
  }

  sl->backend->close(sl);
  free(sl);
}
// 250
int stlink_exit_debug_mode(stlink_t *sl) {
  DLOG("*** stlink_exit_debug_mode ***\n");

  if (sl->flash_type != STM32_FLASH_TYPE_UNKNOWN &&
      sl->core_stat != TARGET_RESET) {
    // stop debugging if the target has been identified
    stlink_write_debug32(sl, STLINK_REG_DHCSR, STLINK_REG_DHCSR_DBGKEY);
  }

  return (sl->backend->exit_debug_mode(sl));
}
//248
int stlink_enter_swd_mode(stlink_t *sl) {
  DLOG("*** stlink_enter_swd_mode ***\n");
  return (sl->backend->enter_swd_mode(sl));
}
// 271
// Force the core into the debug mode -> halted state.
int stlink_force_debug(stlink_t *sl) {
  DLOG("*** stlink_force_debug_mode ***\n");
  int res = sl->backend->force_debug(sl);
  if (res) {
     return (res);
  }
  // Stop the watchdogs in the halted state for suppress target reboot
  stop_wdg_in_debug(sl);
  return (0);
}
// 251
int stlink_exit_dfu_mode(stlink_t *sl) {
  DLOG("*** stlink_exit_dfu_mode ***\n");
  return (sl->backend->exit_dfu_mode(sl));
}
// 253
int stlink_core_id(stlink_t *sl) {
  int ret;

  DLOG("*** stlink_core_id ***\n");
  ret = sl->backend->core_id(sl);

  if (ret == -1) {
    ELOG("Failed to read core_id\n");
    return (ret);
  }

  if (sl->verbose > 2) {
    stlink_print_data(sl);
  }

  DLOG("core_id = 0x%08x\n", sl->core_id);
  return (ret);
}
// 287
// stlink_chip_id() is called by stlink_load_device_params()
// do not call this procedure directly.
int stlink_chip_id(stlink_t *sl, uint32_t *chip_id) {
  int ret;
  cortex_m3_cpuid_t cpu_id;

  // Read the CPU ID to determine where to read the core id
  if (stlink_cpu_id(sl, &cpu_id) ||
      cpu_id.implementer_id != STLINK_REG_CMx_CPUID_IMPL_ARM) {
    ELOG("Can not connect to target. Please use \'connect under reset\' and try again\n");
    return -1;
  }

  /*
   * the chip_id register in the reference manual have
   * DBGMCU_IDCODE / DBG_IDCODE name
   *
   */

  if ((sl->core_id == STM32_CORE_ID_M7F_H7_SWD || sl->core_id == STM32_CORE_ID_M7F_H7_JTAG) &&
      cpu_id.part == STLINK_REG_CMx_CPUID_PARTNO_CM7) {
    // STM32H7 chipid in 0x5c001000 (RM0433 pg3189)
    ret = stlink_read_debug32(sl, 0x5c001000, chip_id);
  } else if (cpu_id.part == STLINK_REG_CMx_CPUID_PARTNO_CM0 ||
             cpu_id.part == STLINK_REG_CMx_CPUID_PARTNO_CM0P) {
    // STM32F0 (RM0091, pg914; RM0360, pg713)
    // STM32L0 (RM0377, pg813; RM0367, pg915; RM0376, pg917)
    // STM32G0 (RM0444, pg1367)
    ret = stlink_read_debug32(sl, 0x40015800, chip_id);
  } else if (cpu_id.part == STLINK_REG_CMx_CPUID_PARTNO_CM33) {
    // STM32L5 (RM0438, pg2157)
    ret = stlink_read_debug32(sl, 0xE0044000, chip_id);
  } else /* СM3, СM4, CM7 */ {
    // default chipid address

    // STM32F1 (RM0008, pg1087; RM0041, pg681)
    // STM32F2 (RM0033, pg1326)
    // STM32F3 (RM0316, pg1095; RM0313, pg874)
    // STM32F7 (RM0385, pg1676; RM0410, pg1912)
    // STM32L1 (RM0038, pg861)
    // STM32L4 (RM0351, pg1840; RM0394, pg1560)
    // STM32G4 (RM0440, pg2086)
    // STM32WB (RM0434, pg1406)
    ret = stlink_read_debug32(sl, 0xE0042000, chip_id);
  }

  if (ret || !(*chip_id)) {
    *chip_id = 0;
    ret = ret?ret:-1;
    ELOG("Could not find chip id!\n");
  } else {
    *chip_id = (*chip_id) & 0xfff;

    // Fix chip_id for F4 rev A errata, read CPU ID, as CoreID is the same for
    // F2/F4
    if (*chip_id == 0x411 && cpu_id.part == STLINK_REG_CMx_CPUID_PARTNO_CM4) {
      *chip_id = 0x413;
    }
  }

  return (ret);
}
// 288
/**
 * Cortex M tech ref manual, CPUID register description
 * @param sl stlink context
 * @param cpuid pointer to the result object
 */
int stlink_cpu_id(stlink_t *sl, cortex_m3_cpuid_t *cpuid) {
  uint32_t raw;

  if (stlink_read_debug32(sl, STLINK_REG_CM3_CPUID, &raw)) {
    cpuid->implementer_id = 0;
    cpuid->variant = 0;
    cpuid->part = 0;
    cpuid->revision = 0;
    return (-1);
  }

  cpuid->implementer_id = (raw >> 24) & 0x7f;
  cpuid->variant = (raw >> 20) & 0xf;
  cpuid->part = (raw >> 4) & 0xfff;
  cpuid->revision = raw & 0xf;
  return (0);
}
// 303
/**
 * Reads and decodes the flash parameters, as dynamically as possible
 * @param sl
 * @return 0 for success, or -1 for unsupported core type.
 */
int stlink_load_device_params(stlink_t *sl) {
  // This seems to normally work so is unnecessary info for a normal user.
  // Demoted to debug. -- REW
  DLOG("Loading device parameters....\n");
  const struct stlink_chipid_params *params = NULL;
  stlink_core_id(sl);
  uint32_t flash_size;

  if (stlink_chip_id(sl, &sl->chip_id)) {
    return (-1);
  }

  params = stlink_chipid_get_params(sl->chip_id);

  if (params == NULL) {
    WLOG("unknown chip id! %#x\n", sl->chip_id);
    return (-1);
  }

  if (params->flash_type == STM32_FLASH_TYPE_UNKNOWN) {
    WLOG("Invalid flash type, please check device declaration\n");
    sl->flash_size = 0;
    return (0);
  }

  // These are fixed...
  sl->flash_base = STM32_FLASH_BASE;
  sl->sram_base = STM32_SRAM_BASE;
  stlink_read_debug32(sl, (params->flash_size_reg) & ~3, &flash_size);

  if (params->flash_size_reg & 2) {
    flash_size = flash_size >> 16;
  }

  flash_size = flash_size & 0xffff;

  if ((sl->chip_id == STM32_CHIPID_L1_MD ||
        sl->chip_id == STM32_CHIPID_F1_VL_MD_LD ||
        sl->chip_id == STM32_CHIPID_L1_MD_PLUS) &&
      (flash_size == 0)) {
    sl->flash_size = 128 * 1024;
  } else if (sl->chip_id == STM32_CHIPID_L1_CAT2) {
    sl->flash_size = (flash_size & 0xff) * 1024;
  } else if ((sl->chip_id & 0xFFF) == STM32_CHIPID_L1_MD_PLUS_HD) {
    // 0 is 384k and 1 is 256k
    if (flash_size == 0) {
      sl->flash_size = 384 * 1024;
    } else {
      sl->flash_size = 256 * 1024;
    }
  } else {
    sl->flash_size = flash_size * 1024;
  }

  sl->flash_type = params->flash_type;
  sl->flash_pgsz = params->flash_pagesize;
  sl->sram_size = params->sram_size;
  sl->sys_base = params->bootrom_base;
  sl->sys_size = params->bootrom_size;
  sl->option_base = params->option_base;
  sl->option_size = params->option_size;
  sl->chip_flags = params->flags;

  // medium and low devices have the same chipid. ram size depends on flash
  // size. STM32F100xx datasheet Doc ID 16455 Table 2
  if (sl->chip_id == STM32_CHIPID_F1_VL_MD_LD &&
      sl->flash_size < 64 * 1024) {
    sl->sram_size = 0x1000;
  }

  if (sl->chip_id == STM32_CHIPID_G4_CAT3) {
    uint32_t flash_optr;
    stlink_read_debug32(sl, STM32Gx_FLASH_OPTR, &flash_optr);

    if (!(flash_optr & (1 << STM32G4_FLASH_OPTR_DBANK))) {
      sl->flash_pgsz <<= 1;
    }
  }

  // H7 devices with small flash has one bank
  if (sl->chip_flags & CHIP_F_HAS_DUAL_BANK &&
      sl->flash_type == STM32_FLASH_TYPE_H7) {
    if ((sl->flash_size / sl->flash_pgsz) <= 1)
      sl->chip_flags &= ~CHIP_F_HAS_DUAL_BANK;
  }

  ILOG("%s: %u KiB SRAM, %u KiB flash in at least %u %s pages.\n",
      params->dev_type, (unsigned)(sl->sram_size / 1024), (unsigned)(sl->flash_size / 1024),
      (sl->flash_pgsz < 1024) ? (unsigned)(sl->flash_pgsz) : (unsigned)(sl->flash_pgsz / 1024),
      (sl->flash_pgsz < 1024) ? "byte" : "KiB");

  return (0);
}
// 254
int stlink_reset(stlink_t *sl, enum reset_type type) {
  uint32_t dhcsr;
  unsigned timeout;

  DLOG("*** stlink_reset ***\n");

  sl->core_stat = TARGET_RESET;

  if (type == RESET_AUTO) {
    // clear S_RESET_ST in DHCSR register for reset state detection
    stlink_read_debug32(sl, STLINK_REG_DHCSR, &dhcsr);
  }

  if (type == RESET_HARD || type == RESET_AUTO) {
    // hardware target reset
    if (sl->version.stlink_v > 1) {
      stlink_jtag_reset(sl, STLINK_DEBUG_APIV2_DRIVE_NRST_LOW);
      // minimum reset pulse duration of 20 us (RM0008, 8.1.2 Power reset)
      usleep(100);
      stlink_jtag_reset(sl, STLINK_DEBUG_APIV2_DRIVE_NRST_HIGH);
    }
    sl->backend->reset(sl);
    usleep(10000);
  }

  if (type == RESET_AUTO) {
    /* Check if the S_RESET_ST bit is set in DHCSR
     * This means that a reset has occurred
     * DDI0337E, p. 10-4, Debug Halting Control and Status Register */

    dhcsr = 0;
    int res = stlink_read_debug32(sl, STLINK_REG_DHCSR, &dhcsr);
    if ((dhcsr & STLINK_REG_DHCSR_S_RESET_ST) == 0 && !res) {
      // reset not done yet
      // try reset through AIRCR so that NRST does not need to be connected

      WLOG("NRST is not connected\n");
      DLOG("Using reset through SYSRESETREQ\n");
      return stlink_soft_reset(sl, 0);
    }

    // waiting for reset the S_RESET_ST bit within 500ms
    timeout = time_ms() + 500;
    while (time_ms() < timeout) {
      dhcsr = STLINK_REG_DHCSR_S_RESET_ST;
      stlink_read_debug32(sl, STLINK_REG_DHCSR, &dhcsr);
      if ((dhcsr & STLINK_REG_DHCSR_S_RESET_ST) == 0) {
        return (0);
      }
    }

    return (-1);
  }

  if (type == RESET_SOFT || type == RESET_SOFT_AND_HALT) {
    return stlink_soft_reset(sl, (type == RESET_SOFT_AND_HALT));
  }

  return (0);
}
// 255
int stlink_run(stlink_t *sl, enum run_type type) {
  struct stlink_reg rr;
  DLOG("*** stlink_run ***\n");

  /* Make sure we are in Thumb mode
   * Cortex-M chips don't support ARM mode instructions
   * xPSR may be incorrect if the vector table has invalid data */
  stlink_read_reg(sl, 16, &rr);
  if ((rr.xpsr & (1 << 24)) == 0) {
    ILOG("Go to Thumb mode\n");
    stlink_write_reg(sl, rr.xpsr | (1 << 24), 16);
  }

  return (sl->backend->run(sl, type));
}
// 273
int stlink_set_swdclk(stlink_t *sl, int freq_khz) {
  DLOG("*** set_swdclk ***\n");
  return (sl->backend->set_swdclk(sl, freq_khz));
}
// 293
// this function is called by stlink_status()
// do not call stlink_core_stat() directly, always use stlink_status()
void stlink_core_stat(stlink_t *sl) {
  switch (sl->core_stat) {
  case TARGET_RUNNING:
    DLOG("  core status: running\n");
    return;
  case TARGET_HALTED:
    DLOG("  core status: halted\n");
    return;
  case TARGET_RESET:
    DLOG("  core status: reset\n");
    return;
  case TARGET_DEBUG_RUNNING:
    DLOG("  core status: debug running\n");
    return;
  default:
    DLOG("  core status: unknown\n");
  }
}
// 256
int stlink_status(stlink_t *sl) {
  int ret;

  DLOG("*** stlink_status ***\n");
  ret = sl->backend->status(sl);
  stlink_core_stat(sl);
  return (ret);
}
// 257
int stlink_version(stlink_t *sl) {
  DLOG("*** looking up stlink version\n");

  if (sl->backend->version(sl)) {
    return (-1);
  }

  _parse_version(sl, &sl->version);

  DLOG("st vid         = 0x%04x (expect 0x%04x)\n", sl->version.st_vid,
       STLINK_USB_VID_ST);
  DLOG("stlink pid     = 0x%04x\n", sl->version.stlink_pid);
  DLOG("stlink version = 0x%x\n", sl->version.stlink_v);
  DLOG("jtag version   = 0x%x\n", sl->version.jtag_v);
  DLOG("swim version   = 0x%x\n", sl->version.swim_v);

  if (sl->version.jtag_v == 0) {
    WLOG("    warning: stlink doesn't support JTAG/SWD interface\n");
  }

  return (0);
}
// 272
int stlink_target_voltage(stlink_t *sl) {
  int voltage = -1;
  DLOG("*** reading target voltage\n");

  if (sl->backend->target_voltage != NULL) {
    voltage = sl->backend->target_voltage(sl);

    if (voltage != -1) {
      DLOG("target voltage = %imV\n", voltage);
    } else {
      DLOG("error reading target voltage\n");
    }
  } else {
    DLOG("reading voltage not supported by backend\n");
  }

  return (voltage);
}
// 299
bool stlink_is_core_halted(stlink_t *sl) {
  stlink_status(sl);
  return (sl->core_stat == TARGET_HALTED);
}
// 269
int stlink_step(stlink_t *sl) {
  DLOG("*** stlink_step ***\n");
  return (sl->backend->step(sl));
}
// 270
int stlink_current_mode(stlink_t *sl) {
  int mode = sl->backend->current_mode(sl);

  switch (mode) {
  case STLINK_DEV_DFU_MODE:
    DLOG("stlink current mode: dfu\n");
    return (mode);
  case STLINK_DEV_DEBUG_MODE:
    DLOG("stlink current mode: debug (jtag or swd)\n");
    return (mode);
  case STLINK_DEV_MASS_MODE:
    DLOG("stlink current mode: mass\n");
    return (mode);
  }

  DLOG("stlink mode: unknown!\n");
  return (STLINK_DEV_UNKNOWN_MODE);
}
// 274
int stlink_trace_enable(stlink_t *sl, uint32_t frequency) {
  DLOG("*** stlink_trace_enable ***\n");
  return (sl->backend->trace_enable(sl, frequency));
}
// 275
int stlink_trace_disable(stlink_t *sl) {
  DLOG("*** stlink_trace_disable ***\n");
  return (sl->backend->trace_disable(sl));
}
// 276
int stlink_trace_read(stlink_t *sl, uint8_t *buf, size_t size) {
  return (sl->backend->trace_read(sl, buf, size));
}
// 294
void stlink_print_data(stlink_t *sl) {
  if (sl->q_len <= 0 || sl->verbose < UDEBUG) {
    return;
  }

  if (sl->verbose > 2) {
    DLOG("data_len = %d 0x%x\n", sl->q_len, sl->q_len);
  }

  for (int i = 0; i < sl->q_len; i++) {
    if (i % 16 == 0) {
      /*
      if (sl->q_data_dir == Q_DATA_OUT) {
          fprintf(stdout, "\n<- 0x%08x ", sl->q_addr + i);
      } else {
          fprintf(stdout, "\n-> 0x%08x ", sl->q_addr + i);
      }
      */
    }
    // DLOG(" %02x", (unsigned int) sl->q_buf[i]);
    fprintf(stderr, " %02x", (unsigned int)sl->q_buf[i]);
  }
  // DLOG("\n\n");
  fprintf(stderr, "\n");
}
// 283
int stlink_mwrite_sram(stlink_t *sl, uint8_t *data, uint32_t length,
                       stm32_addr_t addr) {
  // write the file in sram at addr

  int error = -1;
  size_t off;
  size_t len;

  // check addr range is inside the sram
  if (addr < sl->sram_base) {
    fprintf(stderr, "addr too low\n");
    goto on_error;
  } else if ((addr + length) < addr) {
    fprintf(stderr, "addr overruns\n");
    goto on_error;
  } else if ((addr + length) > (sl->sram_base + sl->sram_size)) {
    fprintf(stderr, "addr too high\n");
    goto on_error;
  } else if (addr & 3) {
    fprintf(stderr, "unaligned addr\n");
    goto on_error;
  }

  len = length;

  if (len & 3) {
    len -= len & 3;
  }

  // do the copy by 1kB blocks
  for (off = 0; off < len; off += 1024) {
    size_t size = 1024;

    if ((off + size) > len) {
      size = len - off;
    }

    memcpy(sl->q_buf, data + off, size);

    if (size & 3) {
      size += 2;
    } // round size if needed

    stlink_write_mem32(sl, addr + (uint32_t)off, (uint16_t)size);
  }

  if (length > len) {
    memcpy(sl->q_buf, data + len, length - len);
    stlink_write_mem8(sl, addr + (uint32_t)len, (uint16_t)(length - len));
  }

  error = 0; // success
  stlink_fwrite_finalize(sl, addr);

on_error:
  return (error);
}
//284
int stlink_fwrite_sram(stlink_t *sl, const char *path, stm32_addr_t addr) {
  // write the file in sram at addr

  int error = -1;
  size_t off;
  size_t len;
  mapped_file_t mf = MAPPED_FILE_INITIALIZER;

  if (map_file(&mf, path) == -1) {
    fprintf(stderr, "map_file() == -1\n");
    return (-1);
  }

  printf("file %s ", path);
  md5_calculate(&mf);
  stlink_checksum(&mf);

  // check if addr range is inside the SRAM
  if (addr < sl->sram_base) {
    fprintf(stderr, "addr too low\n");
    goto on_error;
  } else if ((addr + mf.len) < addr) {
    fprintf(stderr, "addr overruns\n");
    goto on_error;
  } else if ((addr + mf.len) > (sl->sram_base + sl->sram_size)) {
    fprintf(stderr, "addr too high\n");
    goto on_error;
  } else if (addr & 3) {
    fprintf(stderr, "unaligned addr\n");
    goto on_error;
  }

  len = mf.len;

  if (len & 3) {
    len -= len & 3;
  }

  // do the copy by 1kB blocks
  for (off = 0; off < len; off += 1024) {
    size_t size = 1024;

    if ((off + size) > len) {
      size = len - off;
    }

    memcpy(sl->q_buf, mf.base + off, size);

    if (size & 3) {
      size += 2;
    } // round size if needed

    stlink_write_mem32(sl, addr + (uint32_t)off, (uint16_t)size);
  }

  if (mf.len > len) {
    memcpy(sl->q_buf, mf.base + len, mf.len - len);
    stlink_write_mem8(sl, addr + (uint32_t)len, (uint16_t)(mf.len - len));
  }

  // check the file has been written
  if (check_file(sl, &mf, addr) == -1) {
    fprintf(stderr, "check_file() == -1\n");
    goto on_error;
  }

  error = 0; // success
  stlink_fwrite_finalize(sl, addr);

on_error:
  unmap_file(&mf);
  return (error);
}
// 302
int stlink_fread(stlink_t *sl, const char *path, bool is_ihex,
                 stm32_addr_t addr, size_t size) {
  // read size bytes from addr to file
  ILOG("read from address %#010x size %u\n", addr, (unsigned)size);

  int error;
  int fd = open(path, O_RDWR | O_TRUNC | O_CREAT | O_BINARY, 00700);

  if (fd == -1) {
    fprintf(stderr, "open(%s) == -1\n", path);
    return (-1);
  }

  if (is_ihex) {
    struct stlink_fread_ihex_worker_arg arg;

    if (stlink_fread_ihex_init(&arg, fd, addr)) {
      error = stlink_read(sl, addr, size, &stlink_fread_ihex_worker, &arg);

      if (!stlink_fread_ihex_finalize(&arg)) {
        error = -1;
      }
    } else {
      error = -1;
    }
  } else {
    struct stlink_fread_worker_arg arg = {fd};
    error = stlink_read(sl, addr, size, &stlink_fread_worker, &arg);
  }

  close(fd);
  return (error);
}
// 300
int write_buffer_to_sram(stlink_t *sl, flash_loader_t *fl, const uint8_t *buf,
                         size_t size) {
  // write the buffer right after the loader
  int ret = 0;
  size_t chunk = size & ~0x3;
  size_t rem = size & 0x3;

  if (chunk) {
    memcpy(sl->q_buf, buf, chunk);
    ret = stlink_write_mem32(sl, fl->buf_addr, (uint16_t)chunk);
  }

  if (rem && !ret) {
    memcpy(sl->q_buf, buf + chunk, rem);
    ret = stlink_write_mem8(sl, (fl->buf_addr) + (uint32_t)chunk, (uint16_t)rem);
  }

  return (ret);
}
// 291
uint32_t stlink_calculate_pagesize(stlink_t *sl, uint32_t flashaddr) {
  if ((sl->chip_id == STM32_CHIPID_F2) ||
      (sl->chip_id == STM32_CHIPID_F4) ||
      (sl->chip_id == STM32_CHIPID_F4_DE) ||
      (sl->chip_id == STM32_CHIPID_F4_LP) ||
      (sl->chip_id == STM32_CHIPID_F4_HD) ||
      (sl->chip_id == STM32_CHIPID_F411xx) ||
      (sl->chip_id == STM32_CHIPID_F446) ||
      (sl->chip_id == STM32_CHIPID_F4_DSI) ||
      (sl->chip_id == STM32_CHIPID_F72xxx) ||
      (sl->chip_id == STM32_CHIPID_F412)) {
    uint32_t sector = calculate_F4_sectornum(flashaddr);

    if (sector >= 12) {
      sector -= 12;
    }

    if (sector < 4) {
      sl->flash_pgsz = 0x4000;
    } else if (sector < 5) {
      sl->flash_pgsz = 0x10000;
    } else {
      sl->flash_pgsz = 0x20000;
    }
  } else if (sl->chip_id == STM32_CHIPID_F7 ||
             sl->chip_id == STM32_CHIPID_F76xxx) {
    uint32_t sector = calculate_F7_sectornum(flashaddr);

    if (sector < 4) {
      sl->flash_pgsz = 0x8000;
    } else if (sector < 5) {
      sl->flash_pgsz = 0x20000;
    } else {
      sl->flash_pgsz = 0x40000;
    }
  }

  return ((uint32_t)sl->flash_pgsz);
}
// 279
int stlink_parse_ihex(const char *path, uint8_t erased_pattern, uint8_t **mem,
                      size_t *size, uint32_t *begin) {
  int res = 0;
  *begin = UINT32_MAX;
  uint8_t *data = NULL;
  uint32_t end = 0;
  bool eof_found = false;

  for (int scan = 0; (res == 0) && (scan < 2); ++scan) {
    // parse file two times - first to find memory range, second - to fill it
    if (scan == 1) {
      if (!eof_found) {
        ELOG("No EoF recond\n");
        res = -1;
        break;
      }

      if (*begin >= end) {
        ELOG("No data found in file\n");
        res = -1;
        break;
      }

      *size = (end - *begin) + 1;
      data = calloc(*size, 1); // use calloc to get NULL if out of memory

      if (!data) {
        ELOG("Cannot allocate %u bytes\n", (unsigned)(*size));
        res = -1;
        break;
      }

      memset(data, erased_pattern, *size);
    }

    FILE *file = fopen(path, "r");

    if (!file) {
      ELOG("Cannot open file\n");
      res = -1;
      break;
    }

    uint32_t lba = 0;
    char line[1 + 5 * 2 + 255 * 2 + 2];

    while (fgets(line, sizeof(line), file)) {
      if (line[0] == '\n' || line[0] == '\r') {
        continue;
      } // skip empty lines

      if (line[0] != ':') { // no marker - wrong file format
        ELOG("Wrong file format - no marker\n");
        res = -1;
        break;
      }

      size_t l = strlen(line);

      while (l > 0 && (line[l - 1] == '\n' || line[l - 1] == '\r')) {
        --l;
      } // trim EoL

      if ((l < 11) ||
          (l ==
           (sizeof(line) - 1))) { // line too short or long - wrong file format
        ELOG("Wrong file format - wrong line length\n");
        res = -1;
        break;
      }

      uint8_t chksum = 0; // check sum

      for (size_t i = 1; i < l; i += 2) {
        chksum += stlink_parse_hex(line + i);
      }

      if (chksum != 0) {
        ELOG("Wrong file format - checksum mismatch\n");
        res = -1;
        break;
      }

      uint8_t reclen = stlink_parse_hex(line + 1);

      if (((uint32_t)reclen + 5) * 2 + 1 != l) {
        ELOG("Wrong file format - record length mismatch\n");
        res = -1;
        break;
      }

      uint16_t offset = ((uint16_t)stlink_parse_hex(line + 3) << 8) |
                        ((uint16_t)stlink_parse_hex(line + 5));
      uint8_t rectype = stlink_parse_hex(line + 7);

      switch (rectype) {
      case 0: /* Data */
        if (scan == 0) {
          uint32_t b = lba + offset;
          uint32_t e = b + reclen - 1;

          if (b < *begin) {
            *begin = b;
          }

          if (e > end) {
            end = e;
          }
        } else {
          for (uint8_t i = 0; i < reclen; ++i) {
            uint8_t b = stlink_parse_hex(line + 9 + i * 2);
            uint32_t addr = lba + offset + i;

            if (addr >= *begin && addr <= end) {
              data[addr - *begin] = b;
            }
          }
        }
        break;
      case 1: /* EoF */
        eof_found = true;
        break;
      case 2: /* Extended Segment Address, unexpected */
        res = -1;
        break;
      case 3: /* Start Segment Address, unexpected */
        res = -1;
        break;
      case 4: /* Extended Linear Address */
        if (reclen == 2) {
          lba = ((uint32_t)stlink_parse_hex(line + 9) << 24) |
                ((uint32_t)stlink_parse_hex(line + 11) << 16);
        } else {
          ELOG("Wrong file format - wrong LBA length\n");
          res = -1;
        }
        break;
      case 5: /* Start Linear Address - expected, but ignore */
        break;
      default:
        ELOG("Wrong file format - unexpected record type %d\n", rectype);
        res = -1;
      }

      if (res != 0) {
        break;
      }
    }

    fclose(file);
  }

  if (res == 0) {
    *mem = data;
  } else {
    free(data);
  }

  return (res);
}
// 280
uint8_t stlink_get_erased_pattern(stlink_t *sl) {
  if (sl->flash_type == STM32_FLASH_TYPE_L0_L1) {
    return (0x00);
  } else {
    return (0xff);
  }
}

// 322
int stlink_target_connect(stlink_t *sl, enum connect_type connect) {
  if (connect == CONNECT_UNDER_RESET) {
    stlink_enter_swd_mode(sl);

    stlink_jtag_reset(sl, STLINK_DEBUG_APIV2_DRIVE_NRST_LOW);

    // try to halt the core before reset
    // this is useful if the NRST pin is not connected
    sl->backend->force_debug(sl);

    // minimum reset pulse duration of 20 us (RM0008, 8.1.2 Power reset)
    usleep(20);

    stlink_jtag_reset(sl, STLINK_DEBUG_APIV2_DRIVE_NRST_HIGH);

    // try to halt the core after reset
    unsigned timeout = time_ms() + 10;
    while (time_ms() < timeout) {
      sl->backend->force_debug(sl);
      usleep(100);
    }

    // check NRST connection
    uint32_t dhcsr = 0;
    stlink_read_debug32(sl, STLINK_REG_DHCSR, &dhcsr);
    if ((dhcsr & STLINK_REG_DHCSR_S_RESET_ST) == 0) {
      WLOG("NRST is not connected\n");
    }

    // addition soft reset for halt before the first instruction
    stlink_soft_reset(sl, 1 /* halt on reset */);
  }

  if (stlink_current_mode(sl) != STLINK_DEV_DEBUG_MODE &&
        stlink_enter_swd_mode(sl)) {
    printf("Failed to enter SWD mode\n");
    return -1;
  }

  if (connect == CONNECT_NORMAL) {
    stlink_reset(sl, RESET_AUTO);
  }

  return stlink_load_device_params(sl);
}

// End of delegates....  functions below are private to this module
// same as above with entrypoint.

static void stop_wdg_in_debug(stlink_t *sl) {
  uint32_t dbgmcu_cr;
  uint32_t set;
  uint32_t value;

  switch (sl->flash_type) {
  case STM32_FLASH_TYPE_F0_F1_F3:
  case STM32_FLASH_TYPE_F1_XL:
  case STM32_FLASH_TYPE_G4:
    dbgmcu_cr = STM32F0_DBGMCU_CR;
    set = (1 << STM32F0_DBGMCU_CR_IWDG_STOP) |
          (1 << STM32F0_DBGMCU_CR_WWDG_STOP);
    break;
  case STM32_FLASH_TYPE_F2_F4:
  case STM32_FLASH_TYPE_F7:
  case STM32_FLASH_TYPE_L4_L4P:
    dbgmcu_cr = STM32F4_DBGMCU_APB1FZR1;
    set = (1 << STM32F4_DBGMCU_APB1FZR1_IWDG_STOP) |
          (1 << STM32F4_DBGMCU_APB1FZR1_WWDG_STOP);
    break;
  case STM32_FLASH_TYPE_L0_L1:
  case STM32_FLASH_TYPE_G0:
    dbgmcu_cr = STM32L0_DBGMCU_APB1_FZ;
    set = (1 << STM32L0_DBGMCU_APB1_FZ_IWDG_STOP) |
          (1 << STM32L0_DBGMCU_APB1_FZ_WWDG_STOP);
    break;
  case STM32_FLASH_TYPE_H7:
    dbgmcu_cr = STM32H7_DBGMCU_APB1HFZ;
    set = (1 << STM32H7_DBGMCU_APB1HFZ_IWDG_STOP);
    break;
  case STM32_FLASH_TYPE_WB_WL:
    dbgmcu_cr = STM32WB_DBGMCU_APB1FZR1;
    set = (1 << STM32WB_DBGMCU_APB1FZR1_IWDG_STOP) |
          (1 << STM32WB_DBGMCU_APB1FZR1_WWDG_STOP);
    break;
  default:
    return;
  }

  if (!stlink_read_debug32(sl, dbgmcu_cr, &value)) {
    stlink_write_debug32(sl, dbgmcu_cr, value | set);
  }
}

int stlink_jtag_reset(stlink_t *sl, int value) {
  DLOG("*** stlink_jtag_reset %d ***\n", value);
  return (sl->backend->jtag_reset(sl, value));
}

int stlink_soft_reset(stlink_t *sl, int halt_on_reset) {
  int ret;
  unsigned timeout;
  uint32_t dhcsr, dfsr;

  DLOG("*** stlink_soft_reset %s***\n", halt_on_reset ? "(halt) " : "");

  // halt core and enable debugging (if not already done)
  // C_DEBUGEN is required to Halt on reset (DDI0337E, p. 10-6)
  stlink_write_debug32(sl, STLINK_REG_DHCSR,
                       STLINK_REG_DHCSR_DBGKEY | STLINK_REG_DHCSR_C_HALT |
                           STLINK_REG_DHCSR_C_DEBUGEN);

  // enable Halt on reset by set VC_CORERESET and TRCENA (DDI0337E, p. 10-10)
  if (halt_on_reset) {
    stlink_write_debug32(
        sl, STLINK_REG_CM3_DEMCR,
        STLINK_REG_CM3_DEMCR_TRCENA | STLINK_REG_CM3_DEMCR_VC_HARDERR |
            STLINK_REG_CM3_DEMCR_VC_BUSERR | STLINK_REG_CM3_DEMCR_VC_CORERESET);

    // clear VCATCH in the DFSR register
    stlink_write_debug32(sl, STLINK_REG_DFSR, STLINK_REG_DFSR_VCATCH);
  } else {
    stlink_write_debug32(sl, STLINK_REG_CM3_DEMCR,
                         STLINK_REG_CM3_DEMCR_TRCENA |
                             STLINK_REG_CM3_DEMCR_VC_HARDERR |
                             STLINK_REG_CM3_DEMCR_VC_BUSERR);
  }

  // clear S_RESET_ST in the DHCSR register
  stlink_read_debug32(sl, STLINK_REG_DHCSR, &dhcsr);

  // soft reset (core reset) by SYSRESETREQ (DDI0337E, p. 8-23)
  ret = stlink_write_debug32(sl, STLINK_REG_AIRCR,
                             STLINK_REG_AIRCR_VECTKEY |
                                 STLINK_REG_AIRCR_SYSRESETREQ);
  if (ret) {
    ELOG("Soft reset failed: error write to AIRCR\n");
    return (ret);
  }

  // waiting for a reset within 500ms
  // DDI0337E, p. 10-4, Debug Halting Control and Status Register
  timeout = time_ms() + 500;
  while (time_ms() < timeout) {
    // DDI0337E, p. 10-4, Debug Halting Control and Status Register
    dhcsr = STLINK_REG_DHCSR_S_RESET_ST;
    stlink_read_debug32(sl, STLINK_REG_DHCSR, &dhcsr);
    if ((dhcsr & STLINK_REG_DHCSR_S_RESET_ST) == 0) {
      if (halt_on_reset) {
        // waiting halt by the SYSRESETREQ exception
        // DDI0403E, p. C1-699, Debug Fault Status Register
        dfsr = 0;
        stlink_read_debug32(sl, STLINK_REG_DFSR, &dfsr);
        if ((dfsr & STLINK_REG_DFSR_VCATCH) == 0) {
          continue;
        }
      }
      timeout = 0;
      break;
    }
  }

  // reset DFSR register. DFSR is power-on reset only (DDI0337H, p. 7-5)
  stlink_write_debug32(sl, STLINK_REG_DFSR, STLINK_REG_DFSR_CLEAR);

  if (timeout) {
    ELOG("Soft reset failed: timeout\n");
    return (-1);
  }

  return (0);
}
/**
 * Decode the version bits, originally from -sg, verified with usb
 * @param sl stlink context, assumed to contain valid data in the buffer
 * @param slv output parsed version object
 */
void _parse_version(stlink_t *sl, stlink_version_t *slv) {
  sl->version.flags = 0;

  if (sl->version.stlink_v < 3) {
    uint32_t b0 = sl->q_buf[0]; // lsb
    uint32_t b1 = sl->q_buf[1];
    uint32_t b2 = sl->q_buf[2];
    uint32_t b3 = sl->q_buf[3];
    uint32_t b4 = sl->q_buf[4];
    uint32_t b5 = sl->q_buf[5]; // msb

    // b0 b1                       || b2 b3  | b4 b5
    // 4b        | 6b     | 6b     || 2B     | 2B
    // stlink_v  | jtag_v | swim_v || st_vid | stlink_pid

    slv->stlink_v = (b0 & 0xf0) >> 4;
    slv->jtag_v = ((b0 & 0x0f) << 2) | ((b1 & 0xc0) >> 6);
    slv->swim_v = b1 & 0x3f;
    slv->st_vid = (b3 << 8) | b2;
    slv->stlink_pid = (b5 << 8) | b4;

    // ST-LINK/V1 from J11 switch to api-v2 (and support SWD)
    if (slv->stlink_v == 1) {
      slv->jtag_api =
          slv->jtag_v > 11 ? STLINK_JTAG_API_V2 : STLINK_JTAG_API_V1;
    } else {
      slv->jtag_api = STLINK_JTAG_API_V2;

      // preferred API to get last R/W status from J15
      if (sl->version.jtag_v >= 15) {
        sl->version.flags |= STLINK_F_HAS_GETLASTRWSTATUS2;
      }

      if (sl->version.jtag_v >= 13) {
        sl->version.flags |= STLINK_F_HAS_TRACE;
        sl->max_trace_freq = STLINK_V2_MAX_TRACE_FREQUENCY;
      }
    }
  } else {
    // V3 uses different version format, for reference see OpenOCD source
    // (that was written from docs available from ST under NDA):
    // https://github.com/ntfreak/openocd/blob/a6dacdff58ef36fcdac00c53ec27f19de1fbce0d/src/jtag/drivers/stlink_usb.c#L965
    slv->stlink_v = sl->q_buf[0];
    slv->swim_v = sl->q_buf[1];
    slv->jtag_v = sl->q_buf[2];
    slv->st_vid = (uint32_t)((sl->q_buf[9] << 8) | sl->q_buf[8]);
    slv->stlink_pid = (uint32_t)((sl->q_buf[11] << 8) | sl->q_buf[10]);
    slv->jtag_api = STLINK_JTAG_API_V3;
    /* preferred API to get last R/W status */
    sl->version.flags |= STLINK_F_HAS_GETLASTRWSTATUS2;
    sl->version.flags |= STLINK_F_HAS_TRACE;
    sl->max_trace_freq = STLINK_V3_MAX_TRACE_FREQUENCY;
  }

  return;
}

void stlink_run_at(stlink_t *sl, stm32_addr_t addr) {
  stlink_write_reg(sl, addr, 15); /* pc register */
  stlink_run(sl, RUN_NORMAL);

  while (stlink_is_core_halted(sl)) {
    usleep(3000000);
  }
}

/* Limit the block size to compare to 0x1800 as anything larger will stall the
 * STLINK2 Maybe STLINK V1 needs smaller value!
 */
int check_file(stlink_t *sl, mapped_file_t *mf, stm32_addr_t addr) {
  size_t off;
  size_t n_cmp = sl->flash_pgsz;

  if (n_cmp > 0x1800) {
    n_cmp = 0x1800;
  }

  for (off = 0; off < mf->len; off += n_cmp) {
    size_t aligned_size;

    size_t cmp_size = n_cmp; // adjust last page size

    if ((off + n_cmp) > mf->len) {
      cmp_size = mf->len - off;
    }

    aligned_size = cmp_size;

    if (aligned_size & (4 - 1)) {
      aligned_size = (cmp_size + 4) & ~(4 - 1);
    }

    stlink_read_mem32(sl, addr + (uint32_t)off, (uint16_t)aligned_size);

    if (memcmp(sl->q_buf, mf->base + off, cmp_size)) {
      return (-1);
    }
  }

  return (0);
}

void md5_calculate(mapped_file_t *mf) {
  // calculate md5 checksum of given binary file
  Md5Context md5Context;
  MD5_HASH md5Hash;
  Md5Initialise(&md5Context);
  Md5Update(&md5Context, mf->base, (uint32_t)mf->len);
  Md5Finalise(&md5Context, &md5Hash);
  printf("md5 checksum: ");

  for (int i = 0; i < (int)sizeof(md5Hash); i++) {
    printf("%x", md5Hash.bytes[i]);
  }

  printf(", ");
}

void stlink_checksum(mapped_file_t *mp) {
  /* checksum that backward compatible with official ST tools */
  uint32_t sum = 0;
  uint8_t *mp_byte = (uint8_t *)mp->base;

  for (size_t i = 0; i < mp->len; ++i) {
    sum += mp_byte[i];
  }

  printf("stlink checksum: 0x%08x\n", sum);
}

void stlink_fwrite_finalize(stlink_t *sl, stm32_addr_t addr) {
  unsigned int val;
  // set PC to the reset routine
  stlink_read_debug32(sl, addr + 4, &val);
  stlink_write_reg(sl, val, 15);
  stlink_run(sl, RUN_NORMAL);
}

static int stlink_read(stlink_t *sl, stm32_addr_t addr, size_t size,
                       save_block_fn fn, void *fn_arg) {

  int error = -1;

  if (size < 1) {
    size = sl->flash_size;
  }

  if (size > sl->flash_size) {
    size = sl->flash_size;
  }

  size_t cmp_size = (sl->flash_pgsz > 0x1800) ? 0x1800 : sl->flash_pgsz;

  for (size_t off = 0; off < size; off += cmp_size) {
    size_t aligned_size;

    // adjust last page size
    if ((off + cmp_size) > size) {
      cmp_size = size - off;
    }

    aligned_size = cmp_size;

    if (aligned_size & (4 - 1)) {
      aligned_size = (cmp_size + 4) & ~(4 - 1);
    }

    stlink_read_mem32(sl, addr + (uint32_t)off, (uint16_t)aligned_size);

    if (!fn(fn_arg, sl->q_buf, aligned_size)) {
      goto on_error;
    }
  }

  error = 0; // success

on_error:
  return (error);
}

static bool stlink_fread_worker(void *arg, uint8_t *block, ssize_t len) {
  struct stlink_fread_worker_arg *the_arg =
      (struct stlink_fread_worker_arg *)arg;

  if (write(the_arg->fd, block, len) != len) {
    fprintf(stderr, "write() != aligned_size\n");
    return (false);
  } else {
    return (true);
  }
}

// TODO: length not checked
static uint8_t stlink_parse_hex(const char *hex) {
  uint8_t d[2];

  for (int i = 0; i < 2; ++i) {
    char c = *(hex + i);

    if (c >= '0' && c <= '9') {
      d[i] = c - '0';
    } else if (c >= 'A' && c <= 'F') {
      d[i] = c - 'A' + 10;
    } else if (c >= 'a' && c <= 'f') {
      d[i] = c - 'a' + 10;
    } else {
      return (0); // error
    }
  }

  return ((d[0] << 4) | (d[1]));
}

static bool
stlink_fread_ihex_newsegment(struct stlink_fread_ihex_worker_arg *the_arg) {
  uint32_t addr = the_arg->addr;
  uint8_t sum = 2 + 4 + (uint8_t)((addr & 0xFF000000) >> 24) +
                (uint8_t)((addr & 0x00FF0000) >> 16);

  if (17 != fprintf(the_arg->file, ":02000004%04X%02X\r\n",
                    (addr & 0xFFFF0000) >> 16, (uint8_t)(0x100 - sum))) {
    return (false);
  }

  the_arg->lba = (addr & 0xFFFF0000);
  return (true);
}

static bool
stlink_fread_ihex_writeline(struct stlink_fread_ihex_worker_arg *the_arg) {
  uint8_t count = the_arg->buf_pos;

  if (count == 0) {
    return (true);
  }

  uint32_t addr = the_arg->addr;

  if (the_arg->lba != (addr & 0xFFFF0000)) { // segment changed
    if (!stlink_fread_ihex_newsegment(the_arg)) {
      return (false);
    }
  }

  uint8_t sum = count + (uint8_t)((addr & 0x0000FF00) >> 8) +
                (uint8_t)(addr & 0x000000FF);

  if (9 != fprintf(the_arg->file, ":%02X%04X00", count, (addr & 0x0000FFFF))) {
    return (false);
  }

  for (uint8_t i = 0; i < count; ++i) {
    uint8_t b = the_arg->buf[i];
    sum += b;

    if (2 != fprintf(the_arg->file, "%02X", b)) {
      return (false);
    }
  }

  if (4 != fprintf(the_arg->file, "%02X\r\n", (uint8_t)(0x100 - sum))) {
    return (false);
  }

  the_arg->addr += count;
  the_arg->buf_pos = 0;

  return (true);
}

static bool stlink_fread_ihex_init(struct stlink_fread_ihex_worker_arg *the_arg,
                                   int fd, stm32_addr_t addr) {
  the_arg->file = fdopen(fd, "w");
  the_arg->addr = addr;
  the_arg->lba = 0;
  the_arg->buf_pos = 0;

  return (the_arg->file != NULL);
}

static bool stlink_fread_ihex_worker(void *arg, uint8_t *block, ssize_t len) {
  struct stlink_fread_ihex_worker_arg *the_arg =
      (struct stlink_fread_ihex_worker_arg *)arg;

  for (ssize_t i = 0; i < len; ++i) {
    if (the_arg->buf_pos == sizeof(the_arg->buf)) { // line is full
      if (!stlink_fread_ihex_writeline(the_arg)) {
        return (false);
      }
    }

    the_arg->buf[the_arg->buf_pos++] = block[i];
  }

  return (true);
}

static bool
stlink_fread_ihex_finalize(struct stlink_fread_ihex_worker_arg *the_arg) {
  if (!stlink_fread_ihex_writeline(the_arg)) {
    return (false);
  }

  // FIXME: do we need the Start Linear Address?

  if (13 != fprintf(the_arg->file, ":00000001FF\r\n")) { // EoF
    return (false);
  }

  return (0 == fclose(the_arg->file));
}
