    .syntax unified
    .text

    .global mycopy
mycopy:
myloop:
    # copy 4 bytes
    ldr r3, [r0]
    str r3, [r1]

    add r0, r0, #4
    add r1, r1, #4

    # loop if r2 != 0
    sub r2, r2, #1
    cmp r2, #0
    bne myloop

myexit:
    bkpt
