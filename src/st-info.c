/* simple wrapper around the stlink_flash_write function */

// TODO - this should be done as just a simple flag to the st-util command line...


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "stlink-common.h"

static void usage(void)
{
	puts("st-info --flash");
	puts("st-info --sram");
	puts("st-info --descr");
	puts("st-info --pagesize");
	puts("st-info --chipid");
}

static int print_data(stlink_t* sl, char** av)
{
	int ret = 0;
	if (strcmp(av[1], "--flash") == 0) 
		printf("0x%zx\n", sl->flash_size);
	else if (strcmp(av[1], "--sram") == 0)
		printf("0x%zx\n", sl->sram_size);
	else if (strcmp(av[1], "--pagesize") == 0)
		printf("0x%zx\n", sl->flash_pgsz);
	else if (strcmp(av[1], "--chipid") == 0)
		printf("0x%.4x\n", sl->chip_id);
	else if (strcmp(av[1], "--descr")==0) {
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
	return ret;
} 


stlink_t* open_sl(void) 
{
	stlink_t* sl;
	sl = stlink_v1_open(0, 1);
	if (sl == NULL)
		sl = stlink_open_usb(0, 1);
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
	sl->verbose=0;
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
