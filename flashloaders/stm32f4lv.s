    .syntax unified
    .text

    .global mycopy
mycopy:
    ldr r12, flash_base
    ldr r10, flash_off_sr
    add r10, r10, r12

myloop:
    # copy 1 byte each time and 4 times as one group
    ldrb r3, [r0]
    ldrb r4, [r0, #1]
    ldrb r5, [r0, #2]
    ldrb r6, [r0, #3]
    strb r3, [r1]
    strb r4, [r1, #1]
    strb r5, [r1, #2]
    strb r6, [r1, #3]

    add r0, r0, #4
    add r1, r1, #4

    # wait if FLASH_SR == 1
mywait:
    ldrh r3, [r10]
    tst r3, #0x1
    beq mywait

    # loop if r2 != 0
    sub r2, r2, #1
    cmp r2, #0
    bne myloop

myexit:
    bkpt

    .align 2
flash_base:
    .word 0x40023c00
flash_off_sr:
    .word 0x0e
