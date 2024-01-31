#ifndef MEMORY_MAP_H
#define MEMORY_MAP_H

static const char* const memory_map_template_F4 =
    "<?xml version=\"1.0\"?>"
    "<!DOCTYPE memory-map PUBLIC \"+//IDN gnu.org//DTD GDB Memory Map V1.0//EN\""
    "     \"http://sourceware.org/gdb/gdb-memory-map.dtd\">"
    "<memory-map>"
    "  <memory type=\"rom\" start=\"0x00000000\" length=\"0x100000\"/>"     // code = sram, bootrom or flash; flash is bigger
    "  <memory type=\"ram\" start=\"0x10000000\" length=\"0x10000\"/>"      // ccm ram
    "  <memory type=\"ram\" start=\"0x20000000\" length=\"0x20000\"/>"      // sram
    "  <memory type=\"flash\" start=\"0x08000000\" length=\"0x10000\">"     // Sectors 0...3
    "    <property name=\"blocksize\">0x4000</property>"                    // 16 kB
    "  </memory>"
    "  <memory type=\"flash\" start=\"0x08010000\" length=\"0x10000\">"     // Sector 4
    "    <property name=\"blocksize\">0x10000</property>"                   // 64 kB
    "  </memory>"
    "  <memory type=\"flash\" start=\"0x08020000\" length=\"0xE0000\">"     // Sectors 5...11
    "    <property name=\"blocksize\">0x20000</property>"                   // 128 kB
    "  </memory>"
    "  <memory type=\"ram\" start=\"0x40000000\" length=\"0x1fffffff\"/>"   // peripheral regs
    "  <memory type=\"ram\" start=\"0x60000000\" length=\"0x7fffffff\"/>"   // AHB3 Peripherals
    "  <memory type=\"ram\" start=\"0xe0000000\" length=\"0x1fffffff\"/>"   // cortex regs
    "  <memory type=\"rom\" start=\"0x1fff0000\" length=\"0x7800\"/>"       // bootrom
    "  <memory type=\"rom\" start=\"0x1fffc000\" length=\"0x10\"/>"         // option byte area
    "</memory-map>";

static const char* const memory_map_template_F4_HD =
    "<?xml version=\"1.0\"?>"
    "<!DOCTYPE memory-map PUBLIC \"+//IDN gnu.org//DTD GDB Memory Map V1.0//EN\""
    "     \"http://sourceware.org/gdb/gdb-memory-map.dtd\">"
    "<memory-map>"
    "  <memory type=\"rom\" start=\"0x00000000\" length=\"0x100000\"/>"     // code = sram, bootrom or flash; flash is bigger
    "  <memory type=\"ram\" start=\"0x10000000\" length=\"0x10000\"/>"      // ccm ram
    "  <memory type=\"ram\" start=\"0x20000000\" length=\"0x40000\"/>"      // sram
    "  <memory type=\"ram\" start=\"0x60000000\" length=\"0x10000000\"/>"   // fmc bank 1 (nor/psram/sram)
    "  <memory type=\"ram\" start=\"0x70000000\" length=\"0x20000000\"/>"   // fmc bank 2 & 3 (nand flash)
    "  <memory type=\"ram\" start=\"0x90000000\" length=\"0x10000000\"/>"   // fmc bank 4 (pc card)
    "  <memory type=\"ram\" start=\"0xC0000000\" length=\"0x20000000\"/>"   // fmc sdram bank 1 & 2
    "  <memory type=\"flash\" start=\"0x08000000\" length=\"0x10000\">"     // Sectors 0...3
    "    <property name=\"blocksize\">0x4000</property>"                    // 16 kB
    "  </memory>"
    "  <memory type=\"flash\" start=\"0x08010000\" length=\"0x10000\">"     // Sector 4
    "    <property name=\"blocksize\">0x10000</property>"                   // 64 kB
    "  </memory>"
    "  <memory type=\"flash\" start=\"0x08020000\" length=\"0xE0000\">"     // Sectors 5...11
    "    <property name=\"blocksize\">0x20000</property>"                   // 128 kB
    "  </memory>"
    "  <memory type=\"ram\" start=\"0x40000000\" length=\"0x1fffffff\"/>"   // peripheral regs
    "  <memory type=\"ram\" start=\"0xe0000000\" length=\"0x1fffffff\"/>"   // cortex regs
    "  <memory type=\"rom\" start=\"0x1fff0000\" length=\"0x7800\"/>"       // bootrom
    "  <memory type=\"rom\" start=\"0x1fffc000\" length=\"0x10\"/>"         // option byte area
    "</memory-map>";

static const char* const memory_map_template_F2 =
    "<?xml version=\"1.0\"?>"
    "<!DOCTYPE memory-map PUBLIC \"+//IDN gnu.org//DTD GDB Memory Map V1.0//EN\""
    "     \"http://sourceware.org/gdb/gdb-memory-map.dtd\">"
    "<memory-map>"
    "  <memory type=\"rom\" start=\"0x00000000\" length=\"0x%x\"/>"         // code = sram, bootrom or flash; flash is bigger
    "  <memory type=\"ram\" start=\"0x20000000\" length=\"0x%x\"/>"         // SRAM
    "  <memory type=\"flash\" start=\"0x08000000\" length=\"0x10000\">"     // Sectors 0...3
    "    <property name=\"blocksize\">0x4000</property>"                    // 16 kB
    "  </memory>"
    "  <memory type=\"flash\" start=\"0x08010000\" length=\"0x10000\">"     // Sector 4
    "    <property name=\"blocksize\">0x10000</property>"                   // 64 kB
    "  </memory>"
    "  <memory type=\"flash\" start=\"0x08020000\" length=\"0x%x\">"        // Sectors 5...
    "    <property name=\"blocksize\">0x20000</property>"                   // 128 kB
    "  </memory>"
    "  <memory type=\"ram\" start=\"0x40000000\" length=\"0x1fffffff\"/>"   // peripheral regs
    "  <memory type=\"ram\" start=\"0xe0000000\" length=\"0x1fffffff\"/>"   // cortex regs
    "  <memory type=\"rom\" start=\"0x%08x\" length=\"0x%x\"/>"             // bootrom
    "  <memory type=\"rom\" start=\"0x1fffc000\" length=\"0x10\"/>"         // option byte area
    "</memory-map>";

static const char* const memory_map_template_L4 =
    "<?xml version=\"1.0\"?>"
    "<!DOCTYPE memory-map PUBLIC \"+//IDN gnu.org//DTD GDB Memory Map V1.0//EN\""
    "     \"http://sourceware.org/gdb/gdb-memory-map.dtd\">"
    "<memory-map>"
    "  <memory type=\"rom\" start=\"0x00000000\" length=\"0x%x\"/>"         // code = sram, bootrom or flash; flash is bigger
    "  <memory type=\"ram\" start=\"0x10000000\" length=\"0x8000\"/>"       // SRAM2 (32 kB)
    "  <memory type=\"ram\" start=\"0x20000000\" length=\"0x18000\"/>"      // SRAM1 (96 kB)
    "  <memory type=\"flash\" start=\"0x08000000\" length=\"0x%x\">"
    "    <property name=\"blocksize\">0x800</property>"
    "  </memory>"
    "  <memory type=\"ram\" start=\"0x40000000\" length=\"0x1fffffff\"/>"   // peripheral regs
    "  <memory type=\"ram\" start=\"0x60000000\" length=\"0x7fffffff\"/>"   // AHB3 Peripherals
    "  <memory type=\"ram\" start=\"0xe0000000\" length=\"0x1fffffff\"/>"   // cortex regs
    "  <memory type=\"rom\" start=\"0x1fff0000\" length=\"0x7000\"/>"       // bootrom
    "  <memory type=\"rom\" start=\"0x1fff7800\" length=\"0x10\"/>"         // option byte area
    "  <memory type=\"rom\" start=\"0x1ffff800\" length=\"0x10\"/>"         // option byte area
    "</memory-map>";

static const char* const memory_map_template_L496 =
    "<?xml version=\"1.0\"?>"
    "<!DOCTYPE memory-map PUBLIC \"+//IDN gnu.org//DTD GDB Memory Map V1.0//EN\""
    "     \"http://sourceware.org/gdb/gdb-memory-map.dtd\">"
    "<memory-map>"
    "  <memory type=\"rom\" start=\"0x00000000\" length=\"0x%x\"/>"         // code = sram, bootrom or flash; flash is bigger
    "  <memory type=\"ram\" start=\"0x10000000\" length=\"0x10000\"/>"      // SRAM2 (64 kB)
    "  <memory type=\"ram\" start=\"0x20000000\" length=\"0x50000\"/>"      // SRAM1 + aliased SRAM2 (256 + 64 = 320 kB)
    "  <memory type=\"flash\" start=\"0x08000000\" length=\"0x%x\">"
    "    <property name=\"blocksize\">0x800</property>"
    "  </memory>"
    "  <memory type=\"ram\" start=\"0x40000000\" length=\"0x1fffffff\"/>"   // peripheral regs
    "  <memory type=\"ram\" start=\"0x60000000\" length=\"0x7fffffff\"/>"   // AHB3 Peripherals
    "  <memory type=\"ram\" start=\"0xe0000000\" length=\"0x1fffffff\"/>"   // cortex regs
    "  <memory type=\"rom\" start=\"0x1fff0000\" length=\"0x7000\"/>"       // bootrom
    "  <memory type=\"rom\" start=\"0x1fff7800\" length=\"0x10\"/>"         // option byte area
    "  <memory type=\"rom\" start=\"0x1ffff800\" length=\"0x10\"/>"         // option byte area
    "</memory-map>";

static const char* const memory_map_template =
    "<?xml version=\"1.0\"?>"
    "<!DOCTYPE memory-map PUBLIC \"+//IDN gnu.org//DTD GDB Memory Map V1.0//EN\""
    "     \"http://sourceware.org/gdb/gdb-memory-map.dtd\">"
    "<memory-map>"
    "  <memory type=\"rom\" start=\"0x00000000\" length=\"0x%x\"/>"         // code = sram, bootrom or flash; flash is bigger
    "  <memory type=\"ram\" start=\"0x20000000\" length=\"0x%x\"/>"         // SRAM (8 kB)
    "  <memory type=\"flash\" start=\"0x08000000\" length=\"0x%x\">"
    "    <property name=\"blocksize\">0x%x</property>"
    "  </memory>"
    "  <memory type=\"ram\" start=\"0x40000000\" length=\"0x1fffffff\"/>"   // peripheral regs
    "  <memory type=\"ram\" start=\"0xe0000000\" length=\"0x1fffffff\"/>"   // cortex regs
    "  <memory type=\"rom\" start=\"0x%08x\" length=\"0x%x\"/>"             // bootrom
    "  <memory type=\"rom\" start=\"0x1ffff800\" length=\"0x10\"/>"         // option byte area
    "</memory-map>";

static const char* const memory_map_template_F7 =
    "<?xml version=\"1.0\"?>"
    "<!DOCTYPE memory-map PUBLIC \"+//IDN gnu.org//DTD GDB Memory Map V1.0//EN\""
    "     \"http://sourceware.org/gdb/gdb-memory-map.dtd\">"
    "<memory-map>"
    "  <memory type=\"ram\" start=\"0x00000000\" length=\"0x4000\"/>"       // ITCM ram 16 kB
    "  <memory type=\"rom\" start=\"0x00200000\" length=\"0x100000\"/>"     // ITCM flash
    "  <memory type=\"ram\" start=\"0x20000000\" length=\"0x%x\"/>"         // SRAM
    "  <memory type=\"flash\" start=\"0x08000000\" length=\"0x20000\">"     // Sectors 0...3
    "    <property name=\"blocksize\">0x8000</property>"                    // 32 kB
    "  </memory>"
    "  <memory type=\"flash\" start=\"0x08020000\" length=\"0x20000\">"     // Sector 4
    "    <property name=\"blocksize\">0x20000</property>"                   // 128 kB
    "  </memory>"
    "  <memory type=\"flash\" start=\"0x08040000\" length=\"0xC0000\">"     // Sectors 5...7
    "    <property name=\"blocksize\">0x40000</property>"                   // 128 kB
    "  </memory>"
    "  <memory type=\"ram\" start=\"0x40000000\" length=\"0x1fffffff\"/>"   // peripheral regs
    "  <memory type=\"ram\" start=\"0x60000000\" length=\"0x7fffffff\"/>"   // AHB3 Peripherals
    "  <memory type=\"ram\" start=\"0xe0000000\" length=\"0x1fffffff\"/>"   // cortex regs
    "  <memory type=\"rom\" start=\"0x00100000\" length=\"0xEDC0\"/>"       // bootrom
    "  <memory type=\"rom\" start=\"0x1fff0000\" length=\"0x20\"/>"         // option byte area
    "</memory-map>";

static const char* const memory_map_template_H7 =
    "<?xml version=\"1.0\"?>"
    "<!DOCTYPE memory-map PUBLIC \"+//IDN gnu.org//DTD GDB Memory Map V1.0//EN\""
    "     \"http://sourceware.org/gdb/gdb-memory-map.dtd\">"
    "<memory-map>"
    "  <memory type=\"rom\" start=\"0x00000000\" length=\"0x10000\"/>"      // ITCMRAM 64 kB
    "  <memory type=\"ram\" start=\"0x20000000\" length=\"0x20000\"/>"      // DTCMRAM 128 kB
    "  <memory type=\"ram\" start=\"0x24000000\" length=\"0x80000\"/>"      // RAM D1 512 kB
    "  <memory type=\"ram\" start=\"0x30000000\" length=\"0x48000\"/>"      // RAM D2 288 kB
    "  <memory type=\"ram\" start=\"0x38000000\" length=\"0x10000\"/>"      // RAM D3 64 kB
    "  <memory type=\"flash\" start=\"0x08000000\" length=\"0x%x\">"
    "    <property name=\"blocksize\">0x%x</property>"
    "  </memory>"
    "  <memory type=\"ram\" start=\"0x40000000\" length=\"0x1fffffff\"/>"   // peripheral regs
    "  <memory type=\"ram\" start=\"0xe0000000\" length=\"0x1fffffff\"/>"   // cortex regs
    "  <memory type=\"rom\" start=\"0x1ff00000\" length=\"0x20000\"/>"      // bootrom
    "</memory-map>";

static const char* const memory_map_template_H72x3x =
    "<?xml version=\"1.0\"?>"
    "<!DOCTYPE memory-map PUBLIC \"+//IDN gnu.org//DTD GDB Memory Map V1.0//EN\""
    "     \"http://sourceware.org/gdb/gdb-memory-map.dtd\">"
    "<memory-map>"
    "  <memory type=\"rom\" start=\"0x00000000\" length=\"0x40000\"/>"      // ITCMRAM 64 kB + Optional remap
    "  <memory type=\"ram\" start=\"0x20000000\" length=\"0x20000\"/>"      // DTCMRAM 128 kB
    "  <memory type=\"ram\" start=\"0x24000000\" length=\"0x80000\"/>"      // RAM D1 320 kB
    "  <memory type=\"ram\" start=\"0x30000000\" length=\"0x08000\"/>"      // RAM D2 23 kB
    "  <memory type=\"ram\" start=\"0x38000000\" length=\"0x04000\"/>"      // RAM D3 16 kB
    "  <memory type=\"ram\" start=\"0x38800000\" length=\"0x01000\"/>"      // Backup RAM 4 kB
    "  <memory type=\"flash\" start=\"0x08000000\" length=\"0x%x\">"
    "    <property name=\"blocksize\">0x%x</property>"
    "  </memory>"
    "  <memory type=\"ram\" start=\"0x40000000\" length=\"0x1fffffff\"/>"   // peripheral regs
    "  <memory type=\"ram\" start=\"0x60000000\" length=\"0x3fffffff\"/>"   // External Memory
    "  <memory type=\"ram\" start=\"0xC0000000\" length=\"0x1fffffff\"/>"   // External device
    "  <memory type=\"ram\" start=\"0xe0000000\" length=\"0x1fffffff\"/>"   // cortex regs
    "  <memory type=\"rom\" start=\"0x1ff00000\" length=\"0x20000\"/>"      // bootrom
    "</memory-map>";

static const char* const memory_map_template_F4_DE =
    "<?xml version=\"1.0\"?>"
    "<!DOCTYPE memory-map PUBLIC \"+//IDN gnu.org//DTD GDB Memory Map V1.0//EN\""
    "     \"http://sourceware.org/gdb/gdb-memory-map.dtd\">"
    "<memory-map>"
    "  <memory type=\"rom\" start=\"0x00000000\" length=\"0x80000\"/>"      // code = sram, bootrom or flash; flash is bigger
    "  <memory type=\"ram\" start=\"0x20000000\" length=\"0x18000\"/>"      // SRAM
    "  <memory type=\"flash\" start=\"0x08000000\" length=\"0x10000\">"     // Sectors 0..3
    "    <property name=\"blocksize\">0x4000</property>"                    // 16 kB
    "  </memory>"
    "  <memory type=\"flash\" start=\"0x08010000\" length=\"0x10000\">"     // Sector 4
    "    <property name=\"blocksize\">0x10000</property>"                   // 64 kB
    "  </memory>"
    "  <memory type=\"flash\" start=\"0x08020000\" length=\"0x60000\">"     // Sectors 5..7
    "    <property name=\"blocksize\">0x20000</property>"                   // 128 kB
    "  </memory>"
    "  <memory type=\"ram\" start=\"0x40000000\" length=\"0x1fffffff\"/>"   // peripheral regs
    "  <memory type=\"ram\" start=\"0xe0000000\" length=\"0x1fffffff\"/>"   // cortex regs
    "  <memory type=\"rom\" start=\"0x1fff0000\" length=\"0x7800\"/>"       // bootrom
    "  <memory type=\"rom\" start=\"0x1fff7800\" length=\"0x210\"/>"        // otp
    "  <memory type=\"rom\" start=\"0x1fffc000\" length=\"0x10\"/>"         // option byte area
    "</memory-map>";

#endif // MEMORY_MAP_H