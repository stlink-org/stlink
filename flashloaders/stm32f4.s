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
    # copy 4 bytes
    ldr r4, [r0]
    str r4, [r1]

    # increment address
    add r0, r0, #4
    add r1, r1, #4

wait:
    # get FLASH_SR
    ldrh r4, [r10]

    # wait until BUSY flag is reset
    tst r4, #0x1
    bne wait

    # loop if count > 0
    subs r2, r2, #4
    bgt loop

exit:
    bkpt

    .align 2
flash_base:
    .word 0x40023c00
flash_off_sr:
    .word 0x0e
