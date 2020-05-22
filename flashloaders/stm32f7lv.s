    .syntax unified
    .text

    .global copy
copy:
    ldr r12, flash_base
    ldr r10, flash_off_sr
    add r10, r10, r12

    # tip 1: original r2 indicates the count in 4 bytes need to copy,
    #   but we can only copy one byte each time.
    #   as we have no flash larger than 1GB, we do a little trick here.
    # tip 2: r2 is always a power of 2
    mov r2, r2, lsl#2

loop:
    # copy 1 byte
    ldrb r3, [r0]
    strb r3, [r1]

    add r0, r0, #1
    add r1, r1, #1

    # memory barrier
    dsb sy

    # wait if FLASH_SR == 1
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
    .word 0x40023c00
flash_off_sr:
    .word 0x0e
