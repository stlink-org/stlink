/*
 * File: flash.c
 *
 * Tool st-flash - Simple wrapper around the stlink_flash_write function
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <signal.h>

#if defined(_WIN32)
#include <win32_socket.h>
#else
#include <unistd.h>
#endif // _WIN32

#include <stm32.h>
#include <stlink.h>
#include "flash.h"
#include "flash_opts.h"

#include <chipid.h>
#include <common_flash.h>
#include <map_file.h>
#include <option_bytes.h>
#include <usb.h>

static stlink_t *connected_stlink = NULL;

static void cleanup(int32_t signum) {
    (void)signum;

    if (connected_stlink) { // switch back to mass storage mode before closing
        stlink_run(connected_stlink, RUN_NORMAL);
        stlink_exit_debug_mode(connected_stlink);
        stlink_close(connected_stlink);
    }

    exit(1);
}

static void usage(void) {
    puts("command line:   ./st-flash [--debug] [--reset] [--connect-under-reset] [--hot-plug] [--opt] [--serial <serial>] [--format <format>] [--flash=<fsize>] [--freq=<kHz>] [--area=<area>] {read|write} [path] [addr] [size]");
    puts("command line:   ./st-flash [--debug] [--connect-under-reset] [--hot-plug] [--freq=<kHz>] [--serial <serial>] erase [addr] [size]");
    puts("command line:   ./st-flash [--debug] [--freq=<kHz>] [--serial <serial>] reset");
    puts("   <addr>, <serial> and <size>: Use hex format.");
    puts("   <fsize>: Use decimal, octal or hex (prefix 0xXXX) format, optionally followed by k=KB, or m=MB (eg. --flash=128k)");
    puts("   <format>: Can be 'binary' (default) or 'ihex', although <addr> must be specified for binary format only.");
    puts("   <area>: Can be 'main' (default), 'system', 'otp', 'optcr', 'optcr1', 'option' or 'option_boot_add'");
    puts("print tool version info:   ./st-flash [--version]");
    puts("example read option byte: ./st-flash --area=option read [path] [size]");
    puts("example write option byte: ./st-flash --area=option write 0xXXXXXXXX");
    puts("On selected targets:");
    puts("example read boot_add option byte:  ./st-flash --area=option_boot_add read");
    puts("example write boot_add option byte: ./st-flash --area=option_boot_add write 0xXXXXXXXX");
    puts("example read option control register byte:  ./st-flash --area=optcr read");
    puts("example write option control register1 byte:  ./st-flash --area=optcr write 0xXXXXXXXX");
    puts("example read option control register1 byte:  ./st-flash --area=optcr1 read");
    puts("example write option control register1 byte:  ./st-flash --area=optcr1 write 0xXXXXXXXX");
    puts("example read OTP area:  ./st-flash --area=otp read [path]");
    puts("example write OTP area: ./st-flash --area=otp write [path] 0xXXXXXXXX");
}

int32_t main(int32_t ac, char** av) {
    stlink_t* sl = NULL;
    struct flash_opts o;
    int32_t err = -1;
    uint8_t * mem = NULL;

    o.size = 0;
    o.connect = CONNECT_NORMAL;

    if (flash_get_opts(&o, ac - 1, av + 1) == -1) {
        printf("invalid command line\n");
        usage();
        return (-1);
    }

    printf("st-flash %s\n", STLINK_VERSION);
    init_chipids (STLINK_CHIPS_DIR);

    sl = stlink_open_usb(o.log_level, o.connect, (char *)o.serial, o.freq);

    if (sl == NULL) { return (-1); }

    if (sl->flash_type == STM32_FLASH_TYPE_UNKNOWN) {
        printf("Failed to connect to target\n");
        fprintf(stderr, "Failed to parse flash type or unrecognized flash type\n");
        goto on_error;
    }

    if ( o.flash_size != 0u && o.flash_size != sl->flash_size ) {
        sl->flash_size = o.flash_size;
        printf("Forcing flash size: --flash=0x%08X\n", sl->flash_size);
    }

    sl->verbose = o.log_level;
    sl->opt = o.opt;

    connected_stlink = sl;
    signal(SIGINT, &cleanup);
    signal(SIGTERM, &cleanup);
    signal(SIGSEGV, &cleanup);

    // core must be halted to use RAM based flashloaders
    if (stlink_force_debug(sl)) {
        printf("Failed to halt the core\n");
        goto on_error;
    }

    if (stlink_status(sl)) {
        printf("Failed to get Core's status\n");
        goto on_error;
    }

    if (o.cmd == FLASH_CMD_WRITE) {
        uint32_t size = 0;

        // write
        if (o.format == FLASH_FORMAT_IHEX) {
            err = stlink_parse_ihex(o.filename, stlink_get_erased_pattern(sl), &mem, &size, &o.addr);

            if (err == -1) {
                printf("Cannot parse %s as Intel-HEX file\n", o.filename);
                goto on_error;
            }
        }
        if ((o.addr >= sl->flash_base) && (o.addr < sl->flash_base + sl->flash_size)) {
            if (o.format == FLASH_FORMAT_IHEX) {
                err = stlink_mwrite_flash(sl, mem, size, o.addr);
            } else {
                err = stlink_fwrite_flash(sl, o.filename, o.addr);
            }

            if (err == -1) {
                printf("stlink_fwrite_flash() == -1\n");
                goto on_error;
            }
        } else if ((o.addr >= sl->sram_base) && (o.addr < sl->sram_base + sl->sram_size)) {
            if (o.format == FLASH_FORMAT_IHEX) {
                err = stlink_mwrite_sram(sl, mem, size, o.addr);
            } else {
                err = stlink_fwrite_sram(sl, o.filename, o.addr);
            }

            if (err == -1) {
                printf("stlink_fwrite_sram() == -1\n");
                goto on_error;
            }
        } else if ((o.addr >= sl->option_base) && (o.addr < sl->option_base + sl->option_size)) {
            err = stlink_fwrite_option_bytes(sl, o.filename, o.addr);

            if (err == -1) {
                printf("stlink_fwrite_option_bytes() == -1\n");
                goto on_error;
            }
        } else if (o.area == FLASH_OPTION_BYTES) {
            if (o.val == 0) {
                printf("attempting to set option byte to 0, abort.\n");
                goto on_error;
            }

            err = stlink_write_option_bytes32(sl, o.val);

            if (err == -1) {
                printf("stlink_write_option_bytes32() == -1\n");
                goto on_error;
            }
        } else if (o.area == FLASH_OPTCR) {
            DLOG("@@@@ Write %d (%0#10x) to option control register\n", o.val, o.val);

            err = stlink_write_option_control_register32(sl, o.val);
        } else if (o.area == FLASH_OPTCR1) {
            DLOG("@@@@ Write %d (%0#10x) to option control register 1\n", o.val, o.val);

            err = stlink_write_option_control_register1_32(sl, o.val);
        } else if (o.area == FLASH_OPTION_BYTES_BOOT_ADD) {
            DLOG("@@@@ Write %d (%0#10x) to option bytes boot address\n", o.val, o.val);

            err = stlink_write_option_bytes_boot_add32(sl, o.val);
        } else if (o.area == FLASH_OTP) {
            if(sl->otp_base == 0) {
                err = -1;
                printf("OTP Write NOT implemented\n");
                goto on_error;
            }
            err = stlink_fwrite_flash(sl, o.filename,  o.addr);
        
            if (err == -1) {
                printf("stlink_fwrite_flash() == -1\n");
                goto on_error;
            }
        } else {
            err = -1;
            printf("Unknown memory region\n");
            goto on_error;
        }
    
    } else if (o.cmd == FLASH_CMD_ERASE) {

        // erase
        if (o.size > 0 && o.addr > 0) {
          err = stlink_erase_flash_section(sl, o.addr, o.size, false);
        } else {
          err = stlink_erase_flash_mass(sl);
        }
        if (err == -1) {
            printf("stlink_erase_flash_mass() == -1\n");
            goto on_error;
        }
        printf("Mass erase completed successfully.\n");

        // reset after erase
        if (stlink_reset(sl, RESET_AUTO)) {
            printf("Failed to reset device\n");
            goto on_error;
        }
    
    } else if (o.cmd == CMD_RESET) {

        // reset
        if (stlink_reset(sl, RESET_AUTO)) {
            printf("Failed to reset device\n");
            goto on_error;
        }
    
    } else {

        // read
        if ((o.area == FLASH_MAIN_MEMORY) || (o.area == FLASH_SYSTEM_MEMORY)) {
            if ((o.size == 0) && (o.addr >= sl->flash_base) && (o.addr < sl->flash_base + sl->flash_size)) {
                o.size = sl->flash_size;
            }
            else if ((o.size == 0) && (o.addr >= sl->sram_base) && (o.addr < sl->sram_base + sl->sram_size)) {
                o.size = sl->sram_size;
            }
            err = stlink_fread(sl, o.filename, o.format == FLASH_FORMAT_IHEX, o.addr, o.size);

            if (err == -1) {
                printf("could not read main memory (%d)\n", err);
                goto on_error;
            }
        } else if (o.area == FLASH_OPTION_BYTES) {
            uint32_t remaining_option_length = sl->option_size / 4;
            DLOG("@@@@ Read %u (%#x) option bytes from %#10x\n",
                remaining_option_length,
                remaining_option_length,
                sl->option_base);

            uint32_t option_byte = 0;
            err = stlink_read_option_bytes32(sl, &option_byte);
            if (err == -1) {
                printf("could not read option bytes (%d)\n", err);
                goto on_error;
            } else if (NULL != o.filename) {
                int32_t fd = open(o.filename, O_RDWR | O_TRUNC | O_CREAT | O_BINARY, 00700);
                if (fd == -1) {
                    fprintf(stderr, "open(%s) == -1\n", o.filename);
                    goto on_error;
                }
                err = (uint32_t)write(fd, &option_byte, 4);
                if (err == -1) {
                    printf("could not write buffer to file (%d)\n", err);
                    goto on_error;
                }
                close(fd);
            } else {
                printf("%08x\n", option_byte);
            }
        } else if (o.area == FLASH_OPTION_BYTES_BOOT_ADD) {
            uint32_t option_byte = 0;
            err = stlink_read_option_bytes_boot_add32(sl, &option_byte);
            if (err == -1) {
                printf("could not read option bytes boot address (%d)\n", err);
                goto on_error;
            } else {
                printf("%08x\n",option_byte);
            }
        } else if (o.area == FLASH_OPTCR) {
            uint32_t option_byte = 0;
            err = stlink_read_option_control_register32(sl, &option_byte);
            if (err == -1) {
                printf("could not read option control register (%d)\n", err);
                goto on_error;
            } else {
                printf("%08x\n",option_byte);
            }
        } else if (o.area == FLASH_OPTCR1) {
            uint32_t option_byte = 0;
            err = stlink_read_option_control_register1_32(sl, &option_byte);
            if (err == -1) {
                printf("could not read option control register (%d)\n", err);
                goto on_error;
            } else {
                printf("%08x\n",option_byte);
            }
        } else if (o.area == FLASH_OTP) {
            if(sl->otp_base == 0) {
                err = -1;
                printf("OTP Read NOT implemented\n");
                goto on_error;
            }
            err = stlink_fread(sl, o.filename, 0, sl->otp_base, sl->otp_size);
            if (err == -1) {
                printf("could not read OTP area (%d)\n", err);
                goto on_error;
            }
        }
    }

    if (o.reset) stlink_reset(sl, RESET_AUTO);

    stlink_run(sl, RUN_NORMAL);

    err = 0; // success

on_error:
    stlink_exit_debug_mode(sl);
    stlink_close(sl);
    free(mem);

    return (err);
}
