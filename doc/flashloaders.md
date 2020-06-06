# Flashloaders

## What do flashloaders do

The on-chip FLASH of STM32 needs to be written once a byte/half word/word/double word, which would lead to a unbearably long flashing time if the process is solely done by `stlink` from the host side. Flashloaders are introduced to cooperate with `stlink` so that the flashing process is divided into two stages. In the first stage, `stlink`  loads flashloaders and flash data to SRAM where busy check is not applied. In the second stage, flashloaders are kick-started, writing data from SRAM to FLASH, where a busy check is applied. Thus the write-check\_if\_busy cycle of flashing is done solely by STM32 chip, which saves considerable time in communications between `stlink` and STM32.

As SRAM is usually less in size than FLASH, `stlink` only flashes one page (may be less if SRAM is insufficient) at a time. The whole flashing process may consist of server launches of flashloaders.

## The flahsing process

1. `st-flash` loads compiled binary of corresponding flashloader to SRAM by calling `stlink_flash_loader_init` in `src/flash_loader.c`
2. `st-flash` erases corresponding flash page by calling `stlink_erase_flash_page` in `common.c`.
3. `st-flash` calls `stlink_flash_loader_run` in `flash_loader.c`. In this function
   + buffer of one flash page is written to SRAM following the flashloader
   + the buffer start address (in SRAM) is written to register `r0`
   + the target start address (in FLASH, page aligned) is written to register `r1`
   + the buffer size is written to register `r2`
   + the start address (for now 0x20000000) of flash loader is written to `r15` (`pc`)
   + After that, launching the flashloader and waiting for a halted core (triggered by our flashloader) and confirming that flashing is completed with a zeroed `r2`
4. flashloader part: much like a `memcpy` with busy check
   + copy a single unit of data from SRAM to FLASH
   + (for most devices) wait until flash is not busy
   + trigger a breakpoint which halts the core when finished

## Constraints

Thus for developers who want to modify flashloaders, the following constraints should be satisfied.

* only thumb-1 (for stm32f0 etc) or (thumb-1 and thumb-2) (for stm32f1 etc) instructions can be used, no ARM instructions.
* no stack, since it may overwrite buffer data.
* for most devices, after writing a single unit data, wait until FLASH is not busy.
* for some devices, check if there are any errors during flashing process.
* respect unit size of a single copy.
* after flashing, trigger a breakpint to halt the core.
* a sucessful run ends with `r2` set to zero when halted.
* be sure that flashloaders are at least be capable of running at 0x20000000 (the base address of SRAM)


For devices that need to wait until the flash is not busy, check FLASH_SR_BUSY bit. For devices that need to check if there is any errors during flash, check FLASH\_SR\_(X)ERR where `X` can be any error state

FLASH_SR related offset and copy unit size may be found in ST official reference manuals and/or some header files in other open source projects. Clean room document provides some of them.


## Debug tricks

If you find some flashloaders to be broken or you need to write a new flashloader for new devices, the following tricks may help.

1. Modify `WAIT_ROUNDS` marco to a bigger value so that you will have time to kill st-flash when it is waiting for a halted core.
2. run `st-flash` and kill it after the flashloader is loaded to SRAM
3. launch `st-util` and `gdb`/`lldb`
4. set a breakpoint at the base address of SRAM
5. jump to the base address and start your debug

The tricks work because by this means, most work (flash unlock, flash erase, load flashloader to SRAM) would have been done automatically, saving time to construct a debug environment.