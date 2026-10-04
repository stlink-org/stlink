    .syntax unified
    .text

    /*
     * Arguments:
     *   r0 - source memory ptr
     *   r1 - target memory ptr
     *   r2 - count of bytes

     * Register usage:
     *   r0 - SRAM source pointer (incrementing)
     *   r1 - FLASH word address (incrementing)
     *   r2 - remaining bytes count (decrementing)
     *   r3 - scratch
     *   r4 - scratch
     *   r5 - scratch
     *   r6 - scratch
     *   r7 - FLASH peripheral pointer
     */

    .global copy
copy:
    # subtract flash_origin and divide by 4, to get a FLASH word address
    ldr r3, flash_origin
    subs r1, r3
    lsrs r1, #2

    # load address of FLASH peripheral
    ldr r7, flash_base

loop:
    # fill ADDRESS (@ 0x18)
    str r1, [r7, #0x18]

    # fill DATA0-3 (@ 0x40 - 0x4C)
    adds r7, #0x40
    ldmia r0!, { r3, r4, r5, r6 }
    stmia r7!, { r3, r4, r5, r6 }
    subs r7, #0x50

    # clear CMDDONE, CMDERR and ILLCMD in IRQRAW (@0x10)
    movs r5, #0x0D
    str r5, [r7, #0x10]

    # write BURSTWRITE to COMMAND (@ 0x00)
    movs r4, #0xCC
    str r4, [r7]

wait:
    # wait until CMDDONE, CMDERR or ILLCMD is set in IRQRAW
    ldr r4, [r7, #0x10]
    ands r4, r5
    beq wait

    # remove the lowest bit (CMDDONE), exit if error bits remain
    lsrs r4, #1
    bne exit

    # add 4 words to ADDRESS, subtract 16 bytes from count
    adds r1, #4
    subs r2, #16

    # loop if count > 0
    bgt loop

exit:
    bkpt

    .align 2
flash_base:
    .word 0x40001000
flash_origin:
    .word 0x10040000
