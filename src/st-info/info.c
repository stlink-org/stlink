/**
  ******************************************************************************
  * @file           : info.c
  * @brief          : Tool: st-info
  * @copyright      : Copyright (c) 2026 stlink-org. All rights reserved.
  * @date           : 2026-07-27
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */

#include "info.h"


static void usage(void) {
    puts("st-info --version");
    puts("st-info --probe [--connect-under-reset] [--hot-plug] [--freq=<kHz>]");
    puts("st-info --serial");
    puts("st-info --flash  [--connect-under-reset] [--hot-plug] [--freq=<kHz>]");
    puts("st-info --pagesize  [--connect-under-reset] [--hot-plug] [--freq=<kHz>]");
    puts("st-info --sram  [--connect-under-reset] [--hot-plug] [--freq=<kHz>]");
    puts("st-info --chipid  [--connect-under-reset] [--hot-plug] [--freq=<kHz>]");
    puts("st-info --descr  [--connect-under-reset] [--hot-plug] [--freq=<kHz>]");
    puts("st-info --voltage  [--connect-under-reset] [--hot-plug] [--freq=<kHz>]");
}

static void stlink_print_version(stlink_t *sl) {
    // Implementation of version printing is minimalistic
    // but contains all available information from sl->version
    printf("V%u", sl->version.stlink_v);
    if(sl->version.jtag_v > 0)
        printf("J%u", sl->version.jtag_v);
    if(sl->version.swim_v > 0)
        printf("S%u", sl->version.swim_v);
    printf("\n");
}

static void stlink_print_info(stlink_t *sl) {
    const struct stlink_chipid_params *params = NULL;
    if(!sl) { return; }

    printf("  version:    "); stlink_print_version(sl);
    printf("  serial:     %s\n", sl->serial);
    printf("  flash:      %u (pagesize: %u)\n", sl->flash_size, sl->flash_pgsz);
    printf("  sram:       %u\n", sl->sram_size);
    printf("  chipid:     0x%.3x\n", sl->chip_id);

    params = stlink_chipid_get_params(sl->chip_id);
    if(params) { printf("  dev-type:   %s\n", params->dev_type); }

    // Print target voltage if supported
    int32_t voltage_mv = -1;
    if(sl->version.stlink_v != 1) {
        voltage_mv = stlink_target_voltage(sl);
    }
    if(voltage_mv >= 0) {
        printf("  voltage:    %d mV\n", voltage_mv);
    } else {
        printf("  voltage:    n/a\n");
    }
}

static void stlink_probe(enum connect_type connect, int32_t freq) {
    stlink_t **stdevs;
    uint32_t size;

    size = (uint32_t) stlink_probe_usb(&stdevs, connect, freq);

    printf("Found %u stlink programmers\n", size);

    for(uint32_t n = 0; n < size; n++) {
        if(size > 1) printf("%u.\n", n+1);
        stlink_print_info(stdevs[n]);
    }

    stlink_probe_usb_free(&stdevs, size);
}

static int32_t is_info_command(const char *a) {
    return (strcmp(a, "--version") == 0  || strcmp(a, "--probe") == 0 ||
            strcmp(a, "--serial") == 0   || strcmp(a, "--flash") == 0 ||
            strcmp(a, "--pagesize") == 0 || strcmp(a, "--sram") == 0  ||
            strcmp(a, "--chipid") == 0   || strcmp(a, "--descr") == 0 ||
            strcmp(a, "--voltage") == 0);
}

static int32_t print_data(int32_t ac, char **av) {
    stlink_t* sl = NULL;
    enum connect_type connect = CONNECT_NORMAL;
    int32_t freq = 0;
    const char *remote = NULL;
    const char *cmd = NULL;

    // The command (--probe, --flash, ...) and options may appear in any order.
    for(int32_t i = 1; i < ac; i++) {
        if(strcmp(av[i], "--connect-under-reset") == 0) {
            connect = CONNECT_UNDER_RESET;
        } else if(strcmp(av[i], "--hot-plug") == 0) {
            connect = CONNECT_HOT_PLUG;
        } else if(strcmp(av[i], "--freq") == 0) {
            if(++i >= ac || (freq = arg_parse_freq(av[i])) < 0) {
                printf("Incorrect argument: --freq\n\n"); usage(); return (-1);
            }
        } else if(strncmp(av[i], "--freq=", 7) == 0) {
            if((freq = arg_parse_freq(av[i] + 7)) < 0) {
                printf("Incorrect argument: %s\n\n", av[i]); usage(); return (-1);
            }
        } else if(strcmp(av[i], "--remote") == 0) {
            if(++i >= ac) { printf("Incorrect argument: --remote\n\n"); usage(); return (-1); }
            remote = av[i];
        } else if(strncmp(av[i], "--remote=", 9) == 0) {
            remote = av[i] + 9;
        } else if(is_info_command(av[i]) && cmd == NULL) {
            cmd = av[i];
        } else {
            printf("Incorrect argument: %s\n\n", av[i]);
            usage();
            return (-1);
        }
    }

    if(cmd == NULL) { usage(); return (-1); }

    if(strcmp(cmd, "--version") == 0) {
        printf("v%s\n", STLINK_VERSION);
        return (0);
    }

    init_chipids(NULL);

    // probe needs all devices unclaimed (local only; a remote serves one device)
    if(strcmp(cmd, "--probe") == 0 && remote == NULL) {
        stlink_probe(connect, freq);
        return (0);
    }

    // open first st-link device (or the remote one)
    if(remote) {
        sl = stlink_open_remote_str(0, remote, connect, freq);
    } else {
        sl = stlink_open_usb(0, connect, NULL, freq);
    }
    if(sl == NULL) { return (-1); }

    if(strcmp(cmd, "--probe") == 0) {
        stlink_print_info(sl);
    } else if(strcmp(cmd, "--serial") == 0) {
        printf("%s\n", sl->serial);
    } else if(strcmp(cmd, "--flash") == 0) {
        printf("0x%x\n", sl->flash_size);
    } else if(strcmp(cmd, "--pagesize") == 0) {
        printf("0x%x\n", sl->flash_pgsz);
    } else if(strcmp(cmd, "--sram") == 0) {
        printf("0x%x\n", sl->sram_size);
    } else if(strcmp(cmd, "--chipid") == 0) {
        printf("0x%.4x\n", sl->chip_id);
    } else if(strcmp(cmd, "--descr") == 0) {
        const struct stlink_chipid_params *params = stlink_chipid_get_params(sl->chip_id);
        if(params == NULL) { return (-1); }

        printf("%s\n", params->dev_type);
    } else if(strcmp(cmd, "--voltage") == 0) {
        int32_t voltage_mv = -1;
        if(sl->version.stlink_v != 1) {
            voltage_mv = stlink_target_voltage(sl);
        }
        if(voltage_mv >= 0) {
            printf("%d\n", voltage_mv);
        } else {
            fprintf(stderr, "Failed to read target voltage\n");
            if(sl) {
                stlink_exit_debug_mode(sl);
                stlink_close(sl);
            }
            return (-1);
        }
    }

    if(sl) {
        stlink_exit_debug_mode(sl);
        stlink_close(sl);
    }

    return (0);
}

int32_t main(int32_t ac, char** av) {
    int32_t err = -1;

    if(ac < 2) {
        usage();
        return (-1);
    }

    err = print_data(ac, av);

    return (err);
}
