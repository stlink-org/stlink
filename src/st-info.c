#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <stlink-common.h>

static void usage(void)
{
    puts("st-info --flash");
    puts("st-info --sram");
    puts("st-info --descr");
    puts("st-info --pagesize");
    puts("st-info --chipid");
    puts("st-info --serial");
    puts("st-info --hla-serial");
    puts("st-info --probe");
}

/* Print normal or OpenOCD hla_serial with newline */
static void stlink_print_serial(stlink_t *sl, bool openocd)
{
    const char *fmt;

    if (openocd) {
       printf("\"");
       fmt = "\\x%02x";
    } else {
       fmt = "%02x";
    }

    for (int n = 0; n < sl->serial_size; n++)
        printf(fmt, sl->serial[n]);

    if (openocd)
       printf("\"");
    printf("\n");
}

static void stlink_print_info(stlink_t *sl)
{
    const chip_params_t *params = NULL;

    if (!sl)
        return;

    printf(" serial: ");
    stlink_print_serial(sl, false);
    printf("openocd: ");
    stlink_print_serial(sl, true);

    printf("  flash: %zu (pagesize: %zu)\n", sl->flash_size, sl->flash_pgsz);
    printf("   sram: %zu\n",       sl->sram_size);
    printf(" chipid: 0x%.4x\n",    sl->chip_id);

    for (size_t i = 0; i < sizeof(devices) / sizeof(devices[0]); i++) {
        if (devices[i].chip_id == sl->chip_id) {
            params = &devices[i];
            break;
        }
    }

    if (params)
        printf(" descr: %s\n", params->description);
}

static void stlink_probe(void)
{
    stlink_t **stdevs;
    size_t size;

    size = stlink_probe_usb(&stdevs);

    printf("Found %zu stlink programmers\n", size);

    for (size_t n = 0; n < size; n++)
        stlink_print_info(stdevs[n]);

    stlink_probe_usb_free(&stdevs, size);
}

static int print_data(stlink_t *sl, char **av)
{
    if (strcmp(av[1], "--flash") == 0)
        printf("0x%zx\n", sl->flash_size);
    else if (strcmp(av[1], "--sram") == 0)
        printf("0x%zx\n", sl->sram_size);
    else if (strcmp(av[1], "--pagesize") == 0)
        printf("0x%zx\n", sl->flash_pgsz);
    else if (strcmp(av[1], "--chipid") == 0)
        printf("0x%.4x\n", sl->chip_id);
    else if (strcmp(av[1], "--probe") == 0)
        stlink_probe();
    else if (strcmp(av[1], "--serial") == 0)
        stlink_print_serial(sl, false);
    else if (strcmp(av[1], "--hla-serial") == 0)
        stlink_print_serial(sl, true);
    else if (strcmp(av[1], "--descr") == 0) {
        const chip_params_t *params = NULL;
        for (size_t i = 0; i < sizeof(devices) / sizeof(devices[0]); i++) {
            if(devices[i].chip_id == sl->chip_id) {
                params = &devices[i];
                break;
            }
        }
        if (params == NULL) {
            return -1;
        }
        printf("%s\n", params->description);
    }

    return 0;
}


stlink_t* open_sl(void)
{
    stlink_t* sl;
    sl = stlink_v1_open(0, 1);
    if (sl == NULL)
        sl = stlink_open_usb(0, 1, NULL);
    return sl;
}


int main(int ac, char** av)
{
    stlink_t* sl = NULL;
    int err = -1;
    if (ac < 2) {
        usage();
        return -1;
    }

    sl = open_sl();

    if (sl == NULL) {
        return -1;
    }

    sl->verbose = 0;

    if (stlink_current_mode(sl) == STLINK_DEV_DFU_MODE)
        stlink_exit_dfu_mode(sl);

    if (stlink_current_mode(sl) != STLINK_DEV_DEBUG_MODE)
        stlink_enter_swd_mode(sl);

    err = print_data(sl, av);

    if (sl != NULL)
    {
        stlink_exit_debug_mode(sl);
        stlink_close(sl);
    }

    return err;
}
