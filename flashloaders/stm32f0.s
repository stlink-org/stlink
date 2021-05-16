    .syntax unified
    .text

    /*
     * Arguments:
     *   r0 - source memory ptr
     *   r1 - target memory ptr
     *   r2 - count of bytes
     *   r3 - flash register offset
     */

    .global copy
copy:
    /*
     * These two NOPs here are a safety precaution, added by Pekka Nikander
     * while debugging the STM32F05x support.  They may not be needed, but
     * there were strange problems with simpler programs, like a program
     * that had just a breakpoint or a program that first moved zero to register r2
     * and then had a breakpoint.  So, it appears safest to have these two nops.
     *
     * Feel free to remove them, if you dare, but then please do test the result
     * rigorously.  Also, if you remove these, it may be a good idea first to
     * #if 0 them out, with a comment when these were taken out, and to remove
     * these only a few months later...  But YMMV.
     */
    nop
    nop

    # load flash control register address
    # add r3 to flash_base for support dual bank (see flash_loader.c)
    ldr r7, flash_base
    add r7, r7, r3
    ldr r5, flash_off_sr
    add r5, r5, r7

loop:
    # copy 2 bytes
    ldrh r4, [r0]
    strh r4, [r1]

    # increment address
    adds r0, r0, #0x2
    adds r1, r1, #0x2

    # BUSY flag
    ldr r7, =0x01
wait:
    # get FLASH_SR
    ldr r4, [r5]

    # wait until BUSY flag is reset
    tst r4, r7
    bne wait

    # test PGERR or WRPRTERR flag is reset
    ldr r7, =0x14
    tst r4, r7
    bne exit

    # loop if count > 0
    subs r2, r2, #0x2
    bgt loop

exit:
    bkpt

    .align 2
flash_base:
    .word 0x40022000
flash_off_sr:
    .word 0x0c
