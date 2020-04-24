    .syntax unified
    .text

    .global mycopy
mycopy:
    ldr r12, flash_base
    ldr r11, flash_off_cr
    add r11, r11, r12
    ldr r10, flash_off_sr
    add r10, r10, r12

myloop:
    # FLASH_CR ^= 1
    ldr r3, [r11]
    orr r3, r3, #0x1
    str r3, [r11]

    # copy 2 bytes
    ldrh r3, [r0]
    strh r3, [r1]

    add r0, r0, #2
    add r1, r1, #2

    # wait if FLASH_SR == 1
mywait:
    ldr r3, [r10]
    tst r3, #0x1
    beq mywait

    # exit if FLASH_SR == 4
    tst r3, #0x4
    beq myexit

    # loop if r2 != 0
    sub r2, r2, #1
    cmp r2, #0
    bne myloop

myexit:
    # FLASH_CR &= ~1
    ldr r3, [r11]
    bic r3, r3, #0x1
    str r3, [r11]

    bkpt

flash_base:
    .word 0x40022000
flash_off_cr:
    .word 0x10
flash_off_sr:
    .word 0x0c
