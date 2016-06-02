#include <stdio.h>
#include <stlink.h>

int main(int ac, char** av)
{
	(void)ac;
	(void)av;

    stlink_t* sl;
    struct stlink_reg regs;

    sl = stlink_open_usb(10, 1, NULL);
    if (sl != NULL) {
        printf("-- version\n");
        stlink_version(sl);

        printf("mode before doing anything: %d\n", stlink_current_mode(sl));

        if (stlink_current_mode(sl) == STLINK_DEV_DFU_MODE) {
            printf("-- exit_dfu_mode\n");
            stlink_exit_dfu_mode(sl);
        }

        printf("-- enter_swd_mode\n");
        stlink_enter_swd_mode(sl);

        printf("-- mode after entering swd mode: %d\n", stlink_current_mode(sl));

        printf("-- chip id: %#x\n", sl->chip_id);
        printf("-- core_id: %#x\n", sl->core_id);

        cortex_m3_cpuid_t cpuid;
        stlink_cpu_id(sl, &cpuid);
        printf("cpuid:impl_id = %0#x, variant = %#x\n", cpuid.implementer_id, cpuid.variant);
        printf("cpuid:part = %#x, rev = %#x\n", cpuid.part, cpuid.revision);

        printf("-- read_sram\n");
        static const uint32_t sram_base = STM32_SRAM_BASE;
        uint32_t off;
        for (off = 0; off < 16; off += 4)
            stlink_read_mem32(sl, sram_base + off, 4);

        printf("FP_CTRL\n");
        stlink_read_mem32(sl, STLINK_REG_CM3_FP_CTRL, 4);

        // no idea what reg this is..  */
        //     stlink_read_mem32(sl, 0xe000ed90, 4);
        // no idea what register this is...
        //     stlink_read_mem32(sl, 0xe000edf0, 4);
        // offset 0xC into TIM11 register? TIMx_DIER?
        //     stlink_read_mem32(sl, 0x4001100c, 4); */

        /* Test 32 bit Write */
        write_uint32(sl->q_buf,0x01234567);
        stlink_write_mem32(sl,0x200000a8,4);
        write_uint32(sl->q_buf,0x89abcdef);
        stlink_write_mem32(sl,0x200000ac, 4);
        stlink_read_mem32(sl, 0x200000a8, 4);
        stlink_read_mem32(sl, 0x200000ac, 4);

        /* Test 8 bit write */
        write_uint32(sl->q_buf,0x01234567);
        stlink_write_mem8(sl,0x200001a8,3);
        write_uint32(sl->q_buf,0x89abcdef);
        stlink_write_mem8(sl, 0x200001ac, 3);
        stlink_read_mem32(sl, 0x200001a8, 4);
        stlink_read_mem32(sl, 0x200001ac, 4);

        printf("-- status\n");
        stlink_status(sl);

        printf("-- reset\n");
        stlink_reset(sl);
        stlink_force_debug(sl);
        /* Test reg write*/
        stlink_write_reg(sl, 0x01234567, 3);
        stlink_write_reg(sl, 0x89abcdef, 4);
        stlink_write_reg(sl, 0x12345678, 15);
        for (off = 0; off < 21; off += 1)
            stlink_read_reg(sl, off, &regs);


        stlink_read_all_regs(sl, &regs);

        printf("-- status\n");
        stlink_status(sl);

        printf("-- step\n");
        stlink_step(sl);

        printf("-- run\n");
        stlink_run(sl);

        printf("-- exit_debug_mode\n");
        stlink_exit_debug_mode(sl);

        stlink_close(sl);
    }

    return 0;
}
