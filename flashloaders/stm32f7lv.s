.global start
.syntax unified

@ r0 = source
@ r1 = target
@ r2 = wordcount
@ r3 = flash_base
@ r4 = temp

start:
    lsls    r2, r2, #2
    ldr     r3, flash_base
next:
    cbz     r2, done
    ldrb    r4, [r0]
    strb    r4, [r1]
    dsb     sy

wait:
    ldrh    r4, [r3, #0x0e]
    tst.w   r4, #1
    bne     wait

    add     r0, #1
    add     r1, #1
    sub     r2, #1
    b       next
done:
    bkpt

.align 2

flash_base:
	.word 0x40023c00
