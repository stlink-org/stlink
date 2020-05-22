    .syntax unified
    .text

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
    ldr r7, =flash_base
    ldr r4, [r7]
    ldr r7, =flash_off_cr
    ldr r6, [r7]
    adds r6, r6, r4
    ldr r7, =flash_off_sr
    ldr r5, [r7]
    adds r5, r5, r4

loop:
    # FLASH_CR ^= 1
    ldr r7, =0x1
    ldr r3, [r6]
    orrs r3, r3, r7
    str r3, [r6]

    # copy 2 bytes
    ldrh r3, [r0]
    strh r3, [r1]

    ldr r7, =2
    adds r0, r0, r7
    adds r1, r1, r7

    # wait if FLASH_SR == 1
wait:
    ldr r7, =0x1
    ldr r3, [r5]
    tst r3, r7
    beq wait

    # exit if FLASH_SR != 4
    ldr r7, =0x4
    tst r3, r7
    bne exit

    # loop if r2 != 0
    ldr r7, =0x1
    subs r2, r2, r7
    cmp r2, #0
    bne loop

exit:
    # FLASH_CR &= ~1
    ldr r7, =0x1
    ldr r3, [r6]
    bics r3, r3, r7
    str r3, [r6]

    bkpt

    .align 2
flash_base:
    .word 0x40022000
flash_off_cr:
    .word 0x10
flash_off_sr:
    .word 0x0c
