#include <stlink.h>
#include "calculate.h"
#include "common_flash.h"

uint32_t calculate_F4_sectornum(uint32_t flashaddr) {
  uint32_t offset = 0;
  flashaddr &= ~STM32_FLASH_BASE; // page now holding the actual flash address

  if (flashaddr >= 0x100000) {
    offset = 12;
    flashaddr -= 0x100000;
  }

  if (flashaddr < 0x4000) {
    return (offset + 0);
  } else if (flashaddr < 0x8000) {
    return (offset + 1);
  } else if (flashaddr < 0xc000) {
    return (offset + 2);
  } else if (flashaddr < 0x10000) {
    return (offset + 3);
  } else if (flashaddr < 0x20000) {
    return (offset + 4);
  } else {
    return (offset + (flashaddr / 0x20000) + 4);
  }
}

uint32_t calculate_F7_sectornum(uint32_t flashaddr) {
  flashaddr &= ~STM32_FLASH_BASE; // Page now holding the actual flash address

  if (flashaddr < 0x20000) {
    return (flashaddr / 0x8000);
  } else if (flashaddr < 0x40000) {
    return (4);
  } else {
    return ((flashaddr / 0x40000) + 4);
  }
}

uint32_t calculate_H7_sectornum(stlink_t *sl, uint32_t flashaddr,
                                unsigned bank) {
  flashaddr &=
      ~((bank == BANK_1)
            ? STM32_FLASH_BASE
            : STM32_H7_FLASH_BANK2_BASE); // sector holding the flash address
  return (flashaddr / sl->flash_pgsz);
}

// returns BKER:PNB for the given page address
uint32_t calculate_L4_page(stlink_t *sl, uint32_t flashaddr) {
  uint32_t bker = 0;
  uint32_t flashopt;
  stlink_read_debug32(sl, STM32L4_FLASH_OPTR, &flashopt);
  flashaddr -= STM32_FLASH_BASE;

  if (sl->chip_id == STM32_CHIPID_L4 ||
      sl->chip_id == STM32_CHIPID_L496x_L4A6x ||
      sl->chip_id == STM32_CHIPID_L4Rx) {
    // this chip use dual banked flash
    if (flashopt & (uint32_t)(1lu << STM32L4_FLASH_OPTR_DUALBANK)) {
      uint32_t banksize = (uint32_t)sl->flash_size / 2;

      if (flashaddr >= banksize) {
        flashaddr -= banksize;
        bker = 0x100;
      }
    }
  }

  // For 1MB chips without the dual-bank option set, the page address will
  // overflow into the BKER bit, which gives us the correct bank:page value.
  return (bker | flashaddr / (uint32_t)sl->flash_pgsz);
}
