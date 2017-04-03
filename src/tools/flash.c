/* simple wrapper around the stlink_flash_write function */

// TODO - this should be done as just a simple flag to the st-util command line...


#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <stlink.h>
#include <stlink/tools/flash.h>

static stlink_t *connected_stlink = NULL;

static void cleanup(int signum) {
    (void)signum;

    if (connected_stlink) {
        /* Switch back to mass storage mode before closing. */
        stlink_run(connected_stlink);
        stlink_exit_debug_mode(connected_stlink);
        stlink_close(connected_stlink);
    }

    exit(1);
}

static void usage(void)
{
    puts("stlinkv1 command line: ./st-flash [--debug] [--reset] [--format <format>] [--flash=<fsize>] {read|write} /dev/sgX <path> <addr> <size>");
    puts("stlinkv1 command line: ./st-flash [--debug] /dev/sgX erase");
    puts("stlinkv2 command line: ./st-flash [--debug] [--reset] [--serial <serial>] [--format <format>] [--flash=<fsize>] {read|write} <path> <addr> <size>");
    puts("stlinkv2 command line: ./st-flash [--debug] [--serial <serial>] erase");
    puts("stlinkv2 command line: ./st-flash [--debug] [--serial <serial>] reset");
    puts("                       Use hex format for addr, <serial> and <size>.");
    puts("                       fsize: Use decimal, octal or hex by prefix 0xXXX for hex, optionally followed by k=KB, or m=MB (eg. --flash=128k)");
    puts("                       Format may be 'binary' (default) or 'ihex', although <addr> must be specified for binary format only.");
    puts("                       ./st-flash [--version]");
}

int main(int ac, char** av)
{
    stlink_t* sl = NULL;
    struct flash_opts o;
    int err = -1;
    uint8_t * mem = NULL;

    o.size = 0;
    if (flash_get_opts(&o, ac - 1, av + 1) == -1)
    {
        printf("invalid command line\n");
        usage();
        return -1;
    }

    printf("st-flash %s\n", STLINK_VERSION);

    if (o.devname != NULL) /* stlinkv1 */
        sl = stlink_v1_open(o.log_level, 1);
    else /* stlinkv2 */
        sl = stlink_open_usb(o.log_level, 1, (char *)o.serial);

    if (sl == NULL)
        return -1;

    if ( o.flash_size != 0u && o.flash_size != sl->flash_size ) {
        sl->flash_size = o.flash_size;
        printf("Forcing flash size: --flash=0x%08X\n",(unsigned int)sl->flash_size);
    }

    sl->verbose = o.log_level;

    connected_stlink = sl;
    signal(SIGINT, &cleanup);
    signal(SIGTERM, &cleanup);
    signal(SIGSEGV, &cleanup);

    if (stlink_current_mode(sl) == STLINK_DEV_DFU_MODE) {
        if (stlink_exit_dfu_mode(sl)) {
            printf("Failed to exit DFU mode\n");
            goto on_error;
        }
    }

    if (stlink_current_mode(sl) != STLINK_DEV_DEBUG_MODE) {
        if (stlink_enter_swd_mode(sl)) {
            printf("Failed to enter SWD mode\n");
            goto on_error;
        }
    }

    if (o.reset){
        if (stlink_jtag_reset(sl, 2)) {
            printf("Failed to reset JTAG\n");
            goto on_error;
        }

        if (stlink_reset(sl)) {
            printf("Failed to reset device\n");
            goto on_error;
        }
    }

    // Disable DMA - Set All DMA CCR Registers to zero. - AKS 1/7/2013
    if (sl->chip_id == STLINK_CHIPID_STM32_F4)
    {
        memset(sl->q_buf,0,4);
        for (int i=0;i<8;i++) {
            stlink_write_mem32(sl,0x40026000+0x10+0x18*i,4);
            stlink_write_mem32(sl,0x40026400+0x10+0x18*i,4);
            stlink_write_mem32(sl,0x40026000+0x24+0x18*i,4);
            stlink_write_mem32(sl,0x40026400+0x24+0x18*i,4);
        }
    }

    // Core must be halted to use RAM based flashloaders
    if (stlink_force_debug(sl)) {
        printf("Failed to halt the core\n");
        goto on_error;
    }

    if (stlink_status(sl)) {
        printf("Failed to get Core's status\n");
        goto on_error;
    }

    if (o.cmd == FLASH_CMD_WRITE) /* write */
    {
        size_t size = 0;

        if(o.format == FLASH_FORMAT_IHEX) {
            err = stlink_parse_ihex(o.filename, stlink_get_erased_pattern(sl), &mem, &size, &o.addr);
            if (err == -1) {
                printf("Cannot parse %s as Intel-HEX file\n", o.filename);
                goto on_error;
            }
        }

        if ((o.addr >= sl->flash_base) &&
                (o.addr < sl->flash_base + sl->flash_size)) {
            if(o.format == FLASH_FORMAT_IHEX)
                err = stlink_mwrite_flash(sl, mem, (uint32_t)size, o.addr);
            else
                err = stlink_fwrite_flash(sl, o.filename, o.addr);
            if (err == -1)
            {
                printf("stlink_fwrite_flash() == -1\n");
                goto on_error;
            }
        }
        else if ((o.addr >= sl->sram_base) &&
                (o.addr < sl->sram_base + sl->sram_size)) {
            if(o.format == FLASH_FORMAT_IHEX)
                err = stlink_mwrite_sram(sl, mem, (uint32_t)size, o.addr);
            else
                err = stlink_fwrite_sram(sl, o.filename, o.addr);
            if (err == -1)
            {
                printf("stlink_fwrite_sram() == -1\n");
                goto on_error;
            }
        }
        else {
            err = -1;
            printf("Unknown memory region\n");
            goto on_error;
        }
    } else if (o.cmd == FLASH_CMD_ERASE)
    {
        err = stlink_erase_flash_mass(sl);
        if (err == -1)
        {
            printf("stlink_erase_flash_mass() == -1\n");
            goto on_error;
        }
    } else if (o.cmd == CMD_RESET)
    {
        if (stlink_jtag_reset(sl, 2)) {
            printf("Failed to reset JTAG\n");
            goto on_error;
        }

        if (stlink_reset(sl)) {
            printf("Failed to reset device\n");
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
        err = stlink_fread(sl, o.filename, o.format == FLASH_FORMAT_IHEX, o.addr, o.size);
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
    stlink_exit_debug_mode(sl);
    stlink_close(sl);
    free(mem);

    return err;
}
