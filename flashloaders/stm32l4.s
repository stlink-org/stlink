.global start
.syntax unified

@ Adapted from stm32f4.s
@ STM32L4's flash controller expects double-word writes, has the flash
@ controller mapped in a different location with the registers we care about
@ moved down from the base address, and has BSY moved to bit 16 of SR.
@ r0 = source
@ r1 = target
@ r2 = wordcount
@ r3 = flash_base
@ r4 = temp
@ r5 = temp

start:
    ldr     r3, flash_base
next:
    cbz     r2, done
    ldr     r4, [r0]            /* copy doubleword from source to target */
    ldr     r5, [r0, #4]
    str     r4, [r1]
    str     r5, [r1, #4]

wait:
    ldrh    r4, [r3, #0x12]     /* high half of status register */
    tst     r4, #1              /* BSY = bit 16 */
    bne     wait

    add     r0, #8
    add     r1, #8
    sub     r2, #1
    b       next
done:
    bkpt

.align 2

flash_base:
    .word 0x40022000
