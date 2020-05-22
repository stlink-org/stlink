    .syntax unified
    .text

    .global copy
copy:
    ldr r12, flash_base
    ldr r10, flash_off_bsy
    add r10, r10, r12

loop:
    # copy 8 bytes
    ldr r3, [r0]
    ldr r4, [r0, #4]
    str r3, [r1]
    str r4, [r1, #4]

    add r0, r0, #8
    add r1, r1, #8

    # wait if FLASH_BSY[0b] == 1
wait:
    ldrh r3, [r10]
    tst r3, #0x1
    beq wait

    # loop if r2 != 0
    sub r2, r2, #1
    cmp r2, #0
    bne loop

exit:
    bkpt

    .align 2
flash_base:
    .word 0x40022000
flash_off_bsy:
    .word 0x12
