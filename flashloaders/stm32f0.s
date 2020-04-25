    .syntax unified
    .text

    .global mycopy
mycopy:
    ldr r7, =flash_base
    ldr r4, [r7]
    ldr r7, =flash_off_cr
    ldr r6, [r7]
    adds r6, r6, r4
    ldr r7, =flash_off_sr
    ldr r5, [r7]
    adds r5, r5, r4

myloop:
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
mywait:
    ldr r7, =0x1
    ldr r3, [r5]
    tst r3, r7
    beq mywait

    # exit if FLASH_SR == 4
    ldr r7, =0x4
    tst r3, r7
    beq myexit

    # loop if r2 != 0
    ldr r7, =0x1
    subs r2, r2, r7
    cmp r2, #0
    bne myloop

myexit:
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
