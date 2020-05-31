    .syntax unified
    .text

    .global copy
copy:
loop:
    # copy 4 bytes
    ldr r3, [r0]
    str r3, [r1]

    ldr r7, =4
    add r0, r0, r7
    add r1, r1, r7

    # loop if r2 != 0
    ldr r7, =1
    subs r2, r2, r7
    cmp r2, #0
    bne loop

exit:
    bkpt
