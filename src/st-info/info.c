/*
 * File: stinfo.c
 *
 * Tool st-info 
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <stlink.h>
#include "info.h"

#include <chipid.h>
#include <helper.h>
#include <usb.h>

static void usage(void) {
    puts("st-info --version");
    puts("st-info --probe [--connect-under-reset] [--hot-plug] [--freq=<kHz>]");
    puts("st-info --serial");
    puts("st-info --flash  [--connect-under-reset] [--hot-plug] [--freq=<kHz>]");
    puts("st-info --pagesize  [--connect-under-reset] [--hot-plug] [--freq=<kHz>]");
    puts("st-info --sram  [--connect-under-reset] [--hot-plug] [--freq=<kHz>]");
    puts("st-info --chipid  [--connect-under-reset] [--hot-plug] [--freq=<kHz>]");
    puts("st-info --descr  [--connect-under-reset] [--hot-plug] [--freq=<kHz>]");
}

static void stlink_print_version(stlink_t *sl) {
    // Implementation of version printing is minimalistic
    // but contains all available information from sl->version
    printf("V%u", sl->version.stlink_v);
    if (sl->version.jtag_v > 0)
        printf("J%u", sl->version.jtag_v);
    if (sl->version.swim_v > 0)
        printf("S%u", sl->version.swim_v);
    printf("\n");
}

static void stlink_print_info(stlink_t *sl) {
    const struct stlink_chipid_params *params = NULL;
    if (!sl) { return; }

    printf("  version:    "); stlink_print_version(sl);
    printf("  serial:     %s\n", sl->serial);
    printf("  flash:      %u (pagesize: %u)\n", sl->flash_size, sl->flash_pgsz);
    printf("  sram:       %u\n", sl->sram_size);
    printf("  chipid:     0x%.3x\n", sl->chip_id);

    params = stlink_chipid_get_params(sl->chip_id);
    if (params) { printf("  dev-type:   %s\n", params->dev_type); }
}

static void stlink_probe(enum connect_type connect, int32_t freq) {
    stlink_t **stdevs;
    uint32_t size;

    size = (uint32_t)stlink_probe_usb(&stdevs, connect, freq);

    printf("Found %u stlink programmers\n", size);

    for (uint32_t n = 0; n < size; n++) {
        if (size > 1) printf("%u.\n", n+1);
        stlink_print_info(stdevs[n]);
    }

    stlink_probe_usb_free(&stdevs, size);
}

static int32_t print_data(int32_t ac, char **av) {
    stlink_t* sl = NULL;
    enum connect_type connect = CONNECT_NORMAL;
    int32_t freq = 0;

    if (strcmp(av[1], "--version") == 0) {
        printf("v%s\n", STLINK_VERSION);
        return (0);
    }

    init_chipids(STLINK_CHIPS_DIR);

    for (int32_t i=2; i<ac; i++) {
        
        if (strcmp(av[i], "--connect-under-reset") == 0) {
            connect = CONNECT_UNDER_RESET;
            continue;
        } else if (strcmp(av[i], "--hot-plug") == 0) {
            connect = CONNECT_HOT_PLUG;
            continue;
        } else if (strcmp(av[i], "--freq") == 0) {
            if (++i < ac) {
                freq = arg_parse_freq(av[i]);
                if (freq >= 0) { continue; }
            }
        } else if (strncmp(av[i], "--freq=", 7) == 0) {
            freq = arg_parse_freq(av[i] + 7);
            if (freq >= 0) { continue; }
        }

        printf("Incorrect argument: %s\n\n", av[i]);
        usage();
        return (-1);
    }

    // probe needs all devices unclaimed
    if (strcmp(av[1], "--probe") == 0) {
        stlink_probe(connect, freq);
        return (0);
    }

    // open first st-link device
    sl = stlink_open_usb(0, connect, NULL, freq);
    if (sl == NULL) { return (-1); }

    if (strcmp(av[1], "--serial") == 0) {
        printf("%s\n", sl->serial);
    } else if (strcmp(av[1], "--flash") == 0) {
        printf("0x%x\n", sl->flash_size);
    } else if (strcmp(av[1], "--pagesize") == 0) {
        printf("0x%x\n", sl->flash_pgsz);
    } else if (strcmp(av[1], "--sram") == 0) {
        printf("0x%x\n", sl->sram_size);
    } else if (strcmp(av[1], "--chipid") == 0) {
        printf("0x%.4x\n", sl->chip_id);
    } else if (strcmp(av[1], "--descr") == 0) {
        const struct stlink_chipid_params *params = stlink_chipid_get_params(sl->chip_id);
        if (params == NULL) { return (-1); }

        printf("%s\n", params->dev_type);
    }

    if (sl) {
        stlink_exit_debug_mode(sl);
        stlink_close(sl);
    }

    return (0);
}

int32_t main(int32_t ac, char** av) {
    int32_t err = -1;

    if (ac < 2) {
        usage();
        return (-1);
    }

    err = print_data(ac, av);

    return (err);
}
