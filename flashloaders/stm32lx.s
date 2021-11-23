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
loop:
    # copy 4 bytes
    ldr r4, [r0]
    str r4, [r1]

    # increment address
    add r0, r0, #4
    add r1, r1, #4

    # loop if count > 0
    subs r2, r2, #4
    bgt loop

exit:
    bkpt
