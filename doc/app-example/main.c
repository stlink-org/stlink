#include <stdio.h>
#include <stdlib.h>
#include <stlink.h>

static stlink_t *stlink_open_first(void)
{
    stlink_t* sl = NULL;
    sl = stlink_v1_open(0, 1);
    if (sl == NULL)
        sl = stlink_open_usb(0, 1, NULL);

    return sl;
}


int main()
{
    stlink_t* sl = NULL;
    sl = stlink_open_first();

    if (sl == NULL) {
        fprintf(stderr, "Failed to open stlink device ;(\n");
        exit(1);
    }

    fprintf(stderr, "STlink device opened, that's cool!\n");
    stlink_close(sl);
    return 0;
}
