#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "stlink-common.h"

void stlink_print_info(stlink_t *sl)
{
	const chip_params_t *params = NULL;

	for (size_t n = 0; n < sizeof(sl->serial); n++)
		printf("%02x", sl->serial[n]);
	printf("\n");

	printf("\t flash: %zu (pagesize: %zu)\n", sl->flash_size, sl->flash_pgsz);
	printf("\t  sram: %zu\n",       sl->sram_size);
	printf("\tchipid: 0x%.4x\n",    sl->chip_id);

	for (size_t i = 0; i < sizeof(devices) / sizeof(devices[0]); i++) {
		if(devices[i].chip_id == sl->chip_id) {
			params = &devices[i];
			break;
		}
	}

	if (params)
		printf("\t descr: %s\n", params->description);
}

int main(void)
{
	stlink_t **stdevs;
	size_t size;

	size = stlink_probe_usb(&stdevs);

	printf("Found %zu stlink programmers\n", size);

	for (size_t n = 0; n < size; n++)
		stlink_print_info(stdevs[n]);

	stlink_probe_usb_free(&stdevs, size);

	return EXIT_SUCCESS;
}
