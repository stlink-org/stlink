/* simple wrapper around the stlink_flash_write function */

// TODO - this should be done as just a simple flag to the st-util command line...


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "stlink-common.h"

enum st_cmds {DO_WRITE = 0, DO_READ = 1, DO_ERASE = 2};
struct opts
{
    enum st_cmds cmd;
    const char* devname;
    const char* filename;
    stm32_addr_t addr;
    size_t size;
    int reset;
};

static void usage(void)
{
    puts("stlinkv1 command line: ./flash [--reset] {read|write} /dev/sgX path addr <size>");
    puts("stlinkv1 command line: ./flash /dev/sgX erase");
    puts("stlinkv2 command line: ./flash [--reset] {read|write} path addr <size>");
    puts("stlinkv2 command line: ./flash erase");
    puts("                       use hex format for addr and <size>");
}

static int get_opts(struct opts* o, int ac, char** av)
{
    /* stlinkv1 command line: ./flash {read|write} /dev/sgX path addr <size> */
    /* stlinkv2 command line: ./flash {read|write} path addr <size> */

    unsigned int i = 0;

    if (ac < 1) return -1;

    if (strcmp(av[0], "--reset") == 0)
    {
        o->reset = 1;
        ac--;
        av++;
    }
    else
    {
        o->reset = 0;
    }

    if (ac < 1) return -1;

    /* stlinkv2 */
    o->devname = NULL;

    if (strcmp(av[0], "erase") == 0)
    {
        o->cmd = DO_ERASE;

        /* stlinkv1 mode */
        if (ac == 2)
        {
            o->devname = av[1];
            i = 1;
        }
    }
    else {
        if (ac < 3) return -1;
        if (strcmp(av[0], "read") == 0)
        {
            o->cmd = DO_READ;

            /* stlinkv1 mode */
            if (ac == 5)
            {
                o->devname = av[1];
                i = 1;
            }
            if (ac > 3)
                o->size = strtoul(av[i + 3], NULL, 16);
        }
        else if (strcmp(av[0], "write") == 0)
        {
            o->cmd = DO_WRITE;

            /* stlinkv1 mode */
            if (ac == 4)
            {
                o->devname = av[1];
                i = 1;
            }
        }
        else
        {
            return -1;
        }
    }

    o->filename = av[i + 1];
    o->addr = strtoul(av[i + 2], NULL, 16);

    return 0;
}


int main(int ac, char** av)
{
    stlink_t* sl = NULL;
    struct opts o;
    int err = -1;

    o.size = 0;
    if (get_opts(&o, ac - 1, av + 1) == -1)
    {
        printf("invalid command line\n");
        usage();
        goto on_error;
    }

    if (o.devname != NULL) /* stlinkv1 */
    {
        sl = stlink_v1_open(50, 1);
        if (sl == NULL) goto on_error;
        sl->verbose = 50;
    }
    else /* stlinkv2 */
    {
        sl = stlink_open_usb(50, 1);
        if (sl == NULL) goto on_error;
        sl->verbose = 50;
    }

    if (stlink_current_mode(sl) == STLINK_DEV_DFU_MODE)
        stlink_exit_dfu_mode(sl);

    if (stlink_current_mode(sl) != STLINK_DEV_DEBUG_MODE)
        stlink_enter_swd_mode(sl);

    if (o.reset){
        stlink_jtag_reset(sl,2);
        stlink_reset(sl);
    }

    // Disable DMA - Set All DMA CCR Registers to zero. - AKS 1/7/2013
    if (sl->chip_id == STM32_CHIPID_F4)
    {
        memset(sl->q_buf,0,4);
        for (int i=0;i<8;i++) {
            stlink_write_mem32(sl,0x40026000+0x10+0x18*i,4);
            stlink_write_mem32(sl,0x40026400+0x10+0x18*i,4);
            stlink_write_mem32(sl,0x40026000+0x24+0x18*i,4);
            stlink_write_mem32(sl,0x40026400+0x24+0x18*i,4);
        }
    }
    if (o.cmd == DO_WRITE) /* write */
    {
        if ((o.addr >= sl->flash_base) &&
                (o.addr < sl->flash_base + sl->flash_size)) {
            err = stlink_fwrite_flash(sl, o.filename, o.addr);
            if (err == -1)
            {
                printf("stlink_fwrite_flash() == -1\n");
                goto on_error;
            }
        }
        else if ((o.addr >= sl->sram_base) &&
                (o.addr < sl->sram_base + sl->sram_size)) {
            err = stlink_fwrite_sram(sl, o.filename, o.addr);
            if (err == -1)
            {
                printf("stlink_sram_flash() == -1\n");
                goto on_error;
            }
        }
    } else if (o.cmd == DO_ERASE)
    {
        err = stlink_erase_flash_mass(sl);
        if (err == -1)
        {
            printf("stlink_fwrite_flash() == -1\n");
            goto on_error;
        }
    }
    else /* read */
    {
        if ((o.addr >= sl->flash_base) && (o.size == 0) &&
                (o.addr < sl->flash_base + sl->flash_size))
            o.size = sl->flash_size;
        else if ((o.addr >= sl->sram_base) && (o.size == 0) &&
                (o.addr < sl->sram_base + sl->sram_size))
            o.size = sl->sram_size;
        err = stlink_fread(sl, o.filename, o.addr, o.size);
        if (err == -1)
        {
            printf("stlink_fread() == -1\n");
            goto on_error;
        }
    }

    if (o.reset){
        stlink_jtag_reset(sl,2);
        stlink_reset(sl);
    }

    /* success */
    err = 0;

on_error:
    if (sl != NULL)
    {
        stlink_exit_debug_mode(sl);
        stlink_close(sl);
    }

    return err;
}
