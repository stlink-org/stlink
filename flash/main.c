/* simple wrapper around the stlink_flash_write function */


#include <stdio.h>
#include <stdlib.h>
#include "stlink-common.h"


int main(int ac, char** av)
{
  /* stlinkv1 command line: ./flash /dev/sgX path addr */
  /* stlinkv2 command line: ./flash path addr */

  stlink_t* sl = NULL;
  stm32_addr_t addr;
  const char* path;
  int err;

  if (ac == 4) /* stlinkv1 */
  {
    static const int scsi_verbose = 2;
    sl = stlink_quirk_open(av[1], scsi_verbose);
    path = av[2];
    addr = strtoul(av[3], NULL, 16);
  }
  else if (ac == 3) /* stlinkv2 */
  {
    sl = stlink_open_usb(NULL, 10);
    path = av[1];
    addr = strtoul(av[2], NULL, 16);
  }
  else /* invalid */
  {
    printf("invalid command line\n");
    goto on_error;
  }

  if (sl == NULL) goto on_error;

  err = stlink_fwrite_flash(sl, path, addr);
  if (err == -1)
  {
    printf("stlink_fwrite_flash() == -1\n");
    goto on_error;
  }

 on_error:
  if (sl != NULL) stlink_close(sl);

  return err;
}
