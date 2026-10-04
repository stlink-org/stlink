    .syntax unified
    .text

    /*
     * STM32H5 flash loader.
     *
     * Arguments:
     *   r0 - source memory ptr (SRAM staging buffer)
     *   r1 - target memory ptr (flash)
     *   r2 - count of bytes (padded to a multiple of 16 by the host)
     *   r3 - flash register base (unused: the host unlocks flash and sets the
     *        NSCR PG bit before running, and clears it afterwards)
     *
     * The H5 NVM is programmed one 128-bit quad-word (16 bytes) at a time.
     * FLASH_NSSR is at 0x40022020, with BSY in bit 0 (RM0481).
     */

    .global copy
copy:
    ldr r12, flash_base
    ldr r10, flash_off_sr
    add r10, r10, r12

loop:
    # copy one 16-byte quad-word
    ldr r4, [r0]
    ldr r5, [r0, #4]
    ldr r6, [r0, #8]
    ldr r7, [r0, #12]
    str r4, [r1]
    str r5, [r1, #4]
    str r6, [r1, #8]
    str r7, [r1, #12]

    # increment addresses
    add r0, r0, #16
    add r1, r1, #16

    # ensure the quad-word write has been issued before polling status
    dsb sy

wait:
    # get FLASH_NSSR
    ldr r4, [r10]

    # wait until the BUSY flag (bit 0) is reset
    tst r4, #0x1
    bne wait

    # loop while bytes remain
    subs r2, r2, #16
    bgt loop

exit:
    bkpt

    .align 2
flash_base:
    .word 0x40022000
flash_off_sr:
    .word 0x20
