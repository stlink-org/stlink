/* Adopted from STM AN4065 stm32f0xx_flash.c:FLASH_ProgramWord */

write:
    ldr     r4, STM32_FLASH_BASE
    mov     r5, #1            /*  FLASH_CR_PG, FLASH_SR_BUSY */
    mov     r6, #4            /*  PGERR  */
write_half_word:
    ldr     r3, [r4, #16]     /*  FLASH->CR   */
    orr     r3, r5
    str     r3, [r4, #16]     /*  FLASH->CR |= FLASH_CR_PG */
    ldrh    r3, [r0]          /*  r3 = *sram */
    strh    r3, [r1]          /*  *flash = r3 */
busy:
    ldr     r3, [r4, #12]     /*  FLASH->SR  */
    tst     r3, r5            /*  FLASH_SR_BUSY  */
    beq     busy

    tst     r3, r6            /*  PGERR  */
    bne     exit

    add     r0, r0, #2        /*  sram += 2  */
    add     r1, r1, #2        /*  flash += 2  */
    sub     r2, r2, #0x01     /*  count--  */
    cmp     r2, #0
    bne     write_half_word
exit:
    ldr     r3, [r4, #16]     /*  FLASH->CR  */
    bic     r3, r5
    str     r3, [r4, #16]     /*  FLASH->CR &= ~FLASH_CR_PG  */
    bkpt    #0x00

STM32_FLASH_BASE: .word 0x40022000
