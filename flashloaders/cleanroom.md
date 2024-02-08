# Flash Loader Documentation

Code is situated in section `.text`

Shall add a compile directive at the head: `.syntax unified`

**Calling convention**:

All parameters would be passed over registers

`r0`: the base address of the copy source
`r1`: the base address of the copy destination
`r2`: the count of byte to be copied
`r3`: flash register offset (used to support two banks)

**What the program is expected to do**:

Copy data from source to destination, after which trigger a breakpint to exit. Before exit, `r2` must be less or equal to zero to indicate that the copy is done.

**Limitation**: No stack operations are permitted. Registers ranging from `r3` to `r12` are free to use. Note that `r13` is `sp`(stack pointer), `r14` is `lr`(commonly used to store jump address), `r15` is `pc`(program counter).

**Requirement**: After every single copy, wait until the flash finishes. The detailed single copy length and the way to check can be found below. Address of `flash_base` shall be two-bytes aligned.

## stm32f0.s

`flash_base`: 0x40022000

`FLASH_CR`: offset from `flash_base` is 16

`FLASH_SR`: offset from `flash_base` is 12

**Reference**: [https://chromium.googlesource.com/chromiumos/platform/ec/+/master/chip/stm32/registers-stm32f0.h](https://chromium.googlesource.com/chromiumos/platform/ec/+/master/chip/stm32/registers-stm32f0.h)
[https://www.st.com/resource/en/reference_manual/dm00031936-stm32f0x1stm32f0x2stm32f0x8-advanced-armbased-32bit-mcus-stmicroelectronics.pdf](https://www.st.com/resource/en/reference_manual/dm00031936-stm32f0x1stm32f0x2stm32f0x8-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)

**Special requirements**:

Before every copy, read a word from FLASH_CR, set the PG bit to 1 and write back. Copy one half word each time.

How to wait for the write process: Read a word from FLASH_SR, loop until the busy bit is reset. After that, FLASH_SR is checked. The process is interrupted if the error bit (0x04) is set.

Exit: After the copying process and before triggering the breakpoint, clear the PG bit in FLASH_CR.

## stm32f4.s

`flash_base`: 0x40023c00

`FLASH_SR`: offset from `flash_base` is 0xe (14)

**Reference**: [https://chromium.googlesource.com/chromiumos/platform/ec/+/master/chip/stm32/registers-stm32f4.h](https://chromium.googlesource.com/chromiumos/platform/ec/+/master/chip/stm32/registers-stm32f4.h)
[https://www.st.com/content/ccc/resource/technical/document/reference_manual/3d/6d/5a/66/b4/99/40/d4/DM00031020.pdf/files/DM00031020.pdf/jcr:content/translations/en.DM00031020.pdf](https://www.st.com/content/ccc/resource/technical/document/reference_manual/3d/6d/5a/66/b4/99/40/d4/DM00031020.pdf/files/DM00031020.pdf/jcr:content/translations/en.DM00031020.pdf)


**Special requirements**:

Copy one word each time.

How to wait for the write process: Read a word from FLASH_SR, loop until the busy bit is reset.

## stm32f4lv.s

`flash_base`: 0x40023c00

`FLASH_SR`: offset from `flash_base` is 0xe (14)

**Reference**: [https://chromium.googlesource.com/chromiumos/platform/ec/+/master/chip/stm32/registers-stm32f4.h](https://chromium.googlesource.com/chromiumos/platform/ec/+/master/chip/stm32/registers-stm32f4.h)
[https://www.st.com/content/ccc/resource/technical/document/reference_manual/3d/6d/5a/66/b4/99/40/d4/DM00031020.pdf/files/DM00031020.pdf/jcr:content/translations/en.DM00031020.pdf](https://www.st.com/content/ccc/resource/technical/document/reference_manual/3d/6d/5a/66/b4/99/40/d4/DM00031020.pdf/files/DM00031020.pdf/jcr:content/translations/en.DM00031020.pdf)

**Special Requirements**:

Copy one byte each time.

How to wait from the write process: read a half word from FLASH_SR, loop until the busy bit is reset.

## stm32f7.s

**Reference**: [https://chromium.googlesource.com/chromiumos/platform/ec/+/master/chip/stm32/registers-stm32f7.h](https://chromium.googlesource.com/chromiumos/platform/ec/+/master/chip/stm32/registers-stm32f7.h)
[https://www.st.com/resource/en/reference_manual/dm00124865-stm32f75xxx-and-stm32f74xxx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf](https://www.st.com/resource/en/reference_manual/dm00124865-stm32f75xxx-and-stm32f74xxx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)

Mostly same with `stm32f4.s`. Require establishing a memory barrier after every copy and before checking for finished writing by `dsb sy`

## stm32f7lv.s

**Reference**: [https://chromium.googlesource.com/chromiumos/platform/ec/+/master/chip/stm32/registers-stm32f7.h](https://chromium.googlesource.com/chromiumos/platform/ec/+/master/chip/stm32/registers-stm32f7.h)
[https://www.st.com/resource/en/reference_manual/dm00124865-stm32f75xxx-and-stm32f74xxx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf](https://www.st.com/resource/en/reference_manual/dm00124865-stm32f75xxx-and-stm32f74xxx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)

**Special Requirements**:

Mostly same with `stm32f7.s`. Copy one byte each time.

## stm32lx.s

**Special Requirements**:

Copy one word each time. No wait for write.

## stm32l4.s

`flash_base`: 0x40022000
`FLASH_BSY`: offset from `flash_base` is 0x12

**Reference**: [https://chromium.googlesource.com/chromiumos/platform/ec/+/master/chip/stm32/registers-stm32l4.h](https://chromium.googlesource.com/chromiumos/platform/ec/+/master/chip/stm32/registers-stm32l4.h)
[https://www.st.com/resource/en/reference_manual/dm00310109-stm32l4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf](https://www.st.com/resource/en/reference_manual/dm00310109-stm32l4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)

**Special Requirements**:

Copy one double word each time (More than one register is allowed).

How to wait for the write process: read a half word from `FLASH_BSY`, loop until the busy bit is reset.