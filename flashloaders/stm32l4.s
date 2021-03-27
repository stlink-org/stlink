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
    ldr r12, flash_base
    ldr r10, flash_off_sr
    add r10, r10, r12

loop:
    # copy 8 bytes
    ldr r5, [r0]
    ldr r4, [r0, #4]
    str r5, [r1]
    str r4, [r1, #4]

    # increment address
    add r0, r0, #8
    add r1, r1, #8

wait:
    # get FLASH_SR
    ldr r4, [r10]

    # wait until BUSY flag is reset
    tst r4, #0x10000
    bne wait

    # loop if count > 0
    subs r2, r2, #8
    bgt loop

exit:
    bkpt

    .align 2
flash_base:
    .word 0x40022000
flash_off_sr:
    .word 0x10
