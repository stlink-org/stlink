Original Chinese version can be found below.

# Clean Room Documentation English Version

Code is situated in section `.text`

Shall add a compile directive at the head: `.syntax unified`

**Calling convention**:

All parameters would be passed over registers

`r0`: the base address of the copy source
`r1`: the base address of the copy destination
`r2`: the total word (4 bytes) count to be copied (with expeptions)

**What the program is expected to do**:

Copy data from source to destination, after which trigger a breakpint to exit. Before exit, `r2` must be cleared to zero to indicate that the copy is done.

**Limitation**: No stack operations are permitted. Registers ranging from `r3` to `r12` are free to use. Note that `r13` is `sp`(stack pointer), `r14` is `lr`(commonly used to store jump address), `r15` is `pc`(program counter).

**Requirement**: After every single copy, wait until the flash finishes. The detailed single copy length and the way to check can be found below. Address of `flash_base` shall be two-bytes aligned.

## stm32f0.s

**Exception**: `r2` stores the total half word (2 bytes) count to be copied

`flash_base`: 0x40022000

`FLASH_CR`: offset from `flash_base` is 16

`FLASH_SR`: offset from `flash_base` is 12

**Reference**: [https://chromium.googlesource.com/chromiumos/platform/ec/+/master/chip/stm32/registers-stm32f0.h](https://chromium.googlesource.com/chromiumos/platform/ec/+/master/chip/stm32/registers-stm32f0.h)
[https://www.st.com/resource/en/reference_manual/dm00031936-stm32f0x1stm32f0x2stm32f0x8-advanced-armbased-32bit-mcus-stmicroelectronics.pdf](https://www.st.com/resource/en/reference_manual/dm00031936-stm32f0x1stm32f0x2stm32f0x8-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)

**Special requirements**:

Before every copy, read a word from FLASH_CR, set the lowest bit to 1 and write back. Copy one half word each time.

How to wait for the write process: read a word from FLASH_SR, loop until the content is not 1. After that, check FLASH_SR, proceed if the content is 4, otherwise exit.

Exit: after the copying process and before triggering the breakpoint, clear the lowest bit in FLASH_CR.

## stm32f4.s

`flash_base`: 0x40023c00

`FLASH_SR`: offset from `flash_base` is 0xe (14)

**Reference**: [https://chromium.googlesource.com/chromiumos/platform/ec/+/master/chip/stm32/registers-stm32f4.h](https://chromium.googlesource.com/chromiumos/platform/ec/+/master/chip/stm32/registers-stm32f4.h)
[https://www.st.com/content/ccc/resource/technical/document/reference_manual/3d/6d/5a/66/b4/99/40/d4/DM00031020.pdf/files/DM00031020.pdf/jcr:content/translations/en.DM00031020.pdf](https://www.st.com/content/ccc/resource/technical/document/reference_manual/3d/6d/5a/66/b4/99/40/d4/DM00031020.pdf/files/DM00031020.pdf/jcr:content/translations/en.DM00031020.pdf)


**Special requirements**:

Copy one word each time.
How to wait for the write process: read a half word from FLASH_SR, loop until the content is not 1.

## stm32f4lv.s

`flash_base`: 0x40023c00

`FLASH_SR`: offset from `flash_base` is 0xe (14)

**Reference**: [https://chromium.googlesource.com/chromiumos/platform/ec/+/master/chip/stm32/registers-stm32f4.h](https://chromium.googlesource.com/chromiumos/platform/ec/+/master/chip/stm32/registers-stm32f4.h)
[https://www.st.com/content/ccc/resource/technical/document/reference_manual/3d/6d/5a/66/b4/99/40/d4/DM00031020.pdf/files/DM00031020.pdf/jcr:content/translations/en.DM00031020.pdf](https://www.st.com/content/ccc/resource/technical/document/reference_manual/3d/6d/5a/66/b4/99/40/d4/DM00031020.pdf/files/DM00031020.pdf/jcr:content/translations/en.DM00031020.pdf)

**Special Requirements**:

Copy one byte each time.

How to wait from the write process: read a half word from FLASH_SR, loop until the content is not 1.

## stm32f7.s

**Reference**: [https://chromium.googlesource.com/chromiumos/platform/ec/+/master/chip/stm32/registers-stm32f7.h](https://chromium.googlesource.com/chromiumos/platform/ec/+/master/chip/stm32/registers-stm32f7.h)
[https://www.st.com/resource/en/reference_manual/dm00124865-stm32f75xxx-and-stm32f74xxx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf](https://www.st.com/resource/en/reference_manual/dm00124865-stm32f75xxx-and-stm32f74xxx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)

Mostly same with `stm32f4.s`. Require establishing a memory barrier after every copy and before checking for finished writing by `dsb sy`

## stm32f7lv.s

**Reference**: [https://chromium.googlesource.com/chromiumos/platform/ec/+/master/chip/stm32/registers-stm32f7.h](https://chromium.googlesource.com/chromiumos/platform/ec/+/master/chip/stm32/registers-stm32f7.h)
[https://www.st.com/resource/en/reference_manual/dm00124865-stm32f75xxx-and-stm32f74xxx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf](https://www.st.com/resource/en/reference_manual/dm00124865-stm32f75xxx-and-stm32f74xxx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)

**Special Requirements**:

Mostly same with `stm32f7.s`. Copy one byte each time.

## stm32l0x.s

**Special Requirements**:

Copy one word each time. No wait for write.

## stm32l4.s

**Exception**: r2 stores the double word count to be copied.

`flash_base`: 0x40022000
`FLASH_BSY`: offset from `flash_base` is 0x12

**Reference**: [https://chromium.googlesource.com/chromiumos/platform/ec/+/master/chip/stm32/registers-stm32l4.h](https://chromium.googlesource.com/chromiumos/platform/ec/+/master/chip/stm32/registers-stm32l4.h)
[https://www.st.com/resource/en/reference_manual/dm00310109-stm32l4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf](https://www.st.com/resource/en/reference_manual/dm00310109-stm32l4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)

**Special Requirements**:

Copy one double word each time (More than one registers are allowed).

How to wait for the write process: read a half word from `FLASH_BSY`, loop until the lowest bit turns non-1.

## stm32lx.s

Same with stm32l0x.s.


# 净室工程文档-原始中文版

代码位于的section：`.text`
编译制导添加`.syntax unified`

传入参数约定：

参数全部通过寄存器传递

`r0`: 拷贝源点起始地址
`r1`: 拷贝终点起始地址
`r2`: 拷贝word（4字节）数(存在例外)

程序功能：将数据从源点拷贝到终点，在拷贝完毕后触发断点以结束执行，结束时`r2`值需清零表明传输完毕。

限制：不可使用栈，可自由使用的临时寄存器为`R3`到`R12`。`R13`为`sp`（stack pointer），`R14`为lr（一般用于储存跳转地址），`R15`为`pc`（program counter）。

要求：每完成一次拷贝，需等待flash完成写入，单次拷贝宽度、检查写入完成的方式见每个文件的具体要求。

特殊地址`flash_base`存放地址需2字节对齐。

## stm32f0.s

例外：`r2`:拷贝half word（2字节）数

特殊地址定义：`flash_base`:定义为0x40022000

`FLASH_CR`: 相对`flash_base`的offset为16

`FLASH_SR`: 相对`flash_base`的offset为12

参考：[https://chromium.googlesource.com/chromiumos/platform/ec/+/master/chip/stm32/registers-stm32f0.h](https://chromium.googlesource.com/chromiumos/platform/ec/+/master/chip/stm32/registers-stm32f0.h)
[https://www.st.com/resource/en/reference_manual/dm00031936-stm32f0x1stm32f0x2stm32f0x8-advanced-armbased-32bit-mcus-stmicroelectronics.pdf](https://www.st.com/resource/en/reference_manual/dm00031936-stm32f0x1stm32f0x2stm32f0x8-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)

特殊要求：
每次拷贝开始前需要读出FLASH_CR处的4字节内容，将其最低bit设置为1，写回FLASH_CR。

每次写入数据宽度为2字节（半字）。

每完成一次写入，需等待flash完成写入，检查方式为读取FLASH_SR处4字节内容，若取值为1，则说明写入尚未完成，需继续轮询等待；否则需要检查FLASH_SR处值是否为4，若非4，则应直接准备退出。

退出：全部拷贝执行完毕后触发断点前，将FLASH_CR处4字节内容最低bit清为0，写回FLASH_CR。



## stm32f4.s

特殊地址定义： `flash_base`：定义为0x40023c00

`FLASH_SR`:相对flash_base的offset为0xe（14）

参考：[https://chromium.googlesource.com/chromiumos/platform/ec/+/master/chip/stm32/registers-stm32f4.h](https://chromium.googlesource.com/chromiumos/platform/ec/+/master/chip/stm32/registers-stm32f4.h)
[https://www.st.com/content/ccc/resource/technical/document/reference_manual/3d/6d/5a/66/b4/99/40/d4/DM00031020.pdf/files/DM00031020.pdf/jcr:content/translations/en.DM00031020.pdf](https://www.st.com/content/ccc/resource/technical/document/reference_manual/3d/6d/5a/66/b4/99/40/d4/DM00031020.pdf/files/DM00031020.pdf/jcr:content/translations/en.DM00031020.pdf)

特殊要求：

每次写入的数据宽度为4字节（字）。

每完成一次写入，需等待flash完成写入，检查方式为读取FLASH_SR处2字节内容，若取值为1，则说明写入尚未完成，需继续轮询等待。

## stm32f4lv.s

特殊地址定义：`flash_base`：定义为0x40023c00

`FLASH_SR`:相对`flash_base`的offset为0xe (14)

参考：[https://chromium.googlesource.com/chromiumos/platform/ec/+/master/chip/stm32/registers-stm32f4.h](https://chromium.googlesource.com/chromiumos/platform/ec/+/master/chip/stm32/registers-stm32f4.h)
[https://www.st.com/content/ccc/resource/technical/document/reference_manual/3d/6d/5a/66/b4/99/40/d4/DM00031020.pdf/files/DM00031020.pdf/jcr:content/translations/en.DM00031020.pdf](https://www.st.com/content/ccc/resource/technical/document/reference_manual/3d/6d/5a/66/b4/99/40/d4/DM00031020.pdf/files/DM00031020.pdf/jcr:content/translations/en.DM00031020.pdf)

特殊要求：

每次写入的数据宽度为1字节（1/4字）。

每完成一次写入，需等待flash完成写入，检查方式为读取FLASH_SR处2字节内容，若取值为1，则说明写入尚未完成，需继续轮询等待。

## stm32f7.s

参考：[https://chromium.googlesource.com/chromiumos/platform/ec/+/master/chip/stm32/registers-stm32f7.h](https://chromium.googlesource.com/chromiumos/platform/ec/+/master/chip/stm32/registers-stm32f7.h)
[https://www.st.com/resource/en/reference_manual/dm00124865-stm32f75xxx-and-stm32f74xxx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf](https://www.st.com/resource/en/reference_manual/dm00124865-stm32f75xxx-and-stm32f74xxx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)

要求同stm32f4.s，额外要求在每次拷贝执行完毕、flash写入成功检测前，执行`dsb sy`指令以建立内存屏障。


## stm32f7lv.s

参考：[https://chromium.googlesource.com/chromiumos/platform/ec/+/master/chip/stm32/registers-stm32f7.h](https://chromium.googlesource.com/chromiumos/platform/ec/+/master/chip/stm32/registers-stm32f7.h)
[https://www.st.com/resource/en/reference_manual/dm00124865-stm32f75xxx-and-stm32f74xxx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf](https://www.st.com/resource/en/reference_manual/dm00124865-stm32f75xxx-and-stm32f74xxx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
要求基本同stm32f7.s，差异要求为每次写入的数据宽度为1字节（1/4字）。

## stm32l0x.s

特殊要求：

每次写入的数据宽度为4字节（字）

无需实现检查flash写入完成功能

## stm32l4.s

例外：`r2`： 拷贝双字（8字节）数

特殊地址定义：`flash_base`: 0x40022000

`FLASH_BSY`：相对flash_base的offset为0x12

参考：[https://chromium.googlesource.com/chromiumos/platform/ec/+/master/chip/stm32/registers-stm32l4.h](https://chromium.googlesource.com/chromiumos/platform/ec/+/master/chip/stm32/registers-stm32l4.h)
[https://www.st.com/resource/en/reference_manual/dm00310109-stm32l4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf](https://www.st.com/resource/en/reference_manual/dm00310109-stm32l4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)

拷贝方式：一次性拷贝连续的8个字节（使用两个连续寄存器作中转）并写入

每完成一次写入，需等待flash完成写入，检查方式为读取FLASH_BSY处半字（2字节），若其最低位非1，可继续拷贝。

## stm32lx.s

要求与stm32l0x.s相同