#include <stdio.h>
#include "stlink-common.h"

#define STLINKY_MAGIC 0xDEADF00D

struct stlinky {
	stlink_t *sl;
	uint32_t off;
	size_t bufsize;
};

void dump_qbuf(stlink_t* s)
{
	printf("== 0x%x\n", *(uint32_t*)s->q_buf);
}

/* Detects stlinky in RAM, returns handler */ 
struct stlinky*  stlinky_detect(stlink_t* sl) 
{
        static const uint32_t sram_base = 0x20000000;
	struct stlinky* st = malloc(sizeof(struct stlinky));
	st->sl = sl;
	printf("sram: 0x%x bytes @ 0x%x\n", sl->sram_base, sl->sram_size);
        uint32_t off;
        for (off = 0; off < sl->sram_size; off += 4) {
		stlink_read_mem32(sl, sram_base + off, 4);
		if ( STLINKY_MAGIC== *(uint32_t*) sl->q_buf)
		{
			printf("stlinky detected at 0x%x\n", sram_base + off);
			st->off = sram_base + off;
			stlink_read_mem32(sl, st->off + 4, 4);
			st->bufsize = (size_t) *(unsigned char*) sl->q_buf;
			printf("stlinky buffer size 0x%zu (%hhx)\n", st->bufsize);
			return st;
		}
	}
	return NULL;
}


size_t stlinky_rx(struct stlinky *st, char* buffer)
{
	unsigned char tx = 0;
	while(tx == 0) {
		stlink_read_mem32(st->sl, st->off+4, 4);
		tx = (unsigned char) st->sl->q_buf[1];
	}
	size_t rs = tx + (4 - (tx % 4)); /* voodoo */
	stlink_read_mem32(st->sl, st->off+8, rs);
	printf(st->sl->q_buf);
	*st->sl->q_buf=0x0;
	stlink_write_mem8(st->sl, st->off+5, 1);
}


int main(int ac, char** av) {
    stlink_t* sl;
    reg regs;

    /* unused */
    ac = ac;
    av = av;

    sl = stlink_open_usb(10);
    if (sl != NULL) {
        printf("ST Link terminal :: Created by Necromant\n");
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

	stlink_reset(sl);
	stlink_force_debug(sl);
	stlink_run(sl);
	stlink_status(sl);
	/* wait for device to boot */

	sleep(1);

	struct stlinky *st = stlinky_detect(sl);
	if (st == NULL)
	{
		printf("stlinky magic not found in sram :(");
	}

	while(1) {
		stlinky_rx(st, NULL);
	}
	//sleep(90);

        printf("-- read_sram\n");
	//     static const uint32_t sram_base = 0x8000000;
	// uint32_t off;
        //for (off = 0; off < 16; off += 4)
        //    stlink_read_mem32(sl, sram_base + off, 4);
	
        //printf("FP_CTRL\n");
        //stlink_read_mem32(sl, CM3_REG_FP_CTRL, 4);
        
        // no idea what reg this is..  */
        //     stlink_read_mem32(sl, 0xe000ed90, 4);
        // no idea what register this is...
        //     stlink_read_mem32(sl, 0xe000edf0, 4);
        // offset 0xC into TIM11 register? TIMx_DIER?
        //     stlink_read_mem32(sl, 0x4001100c, 4); */

        /* Test 32 bit Write */
        write_uint32(sl->q_buf,0x01234567);
        stlink_write_mem32(sl,0x200000a8,4);
#if 0
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
#endif
        stlink_exit_debug_mode(sl);

        stlink_close(sl);
    }

    return 0;
}
