/* simple wrapper around the stlink_flash_write function */

// TODO - this should be done as just a simple flag to the st-util command line...


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "stlink-common.h"


struct opts
{
  unsigned int do_read;
  const char* devname;
  const char* filename;
  stm32_addr_t addr;
  size_t size;
};

static void usage(void)
{
    puts("stlinkv1 command line: ./flash {read|write} /dev/sgX path addr <size>");
    puts("stlinkv2 command line: ./flash {read|write} path addr <size>");
    puts("                       use hex format for addr and <size>");
}

static int get_opts(struct opts* o, int ac, char** av)
{
  /* stlinkv1 command line: ./flash {read|write} /dev/sgX path addr <size> */
  /* stlinkv2 command line: ./flash {read|write} path addr <size> */

  unsigned int i = 0;

  if (ac < 3) return -1;

  /* stlinkv2 */
  o->devname = NULL;

  if (strcmp(av[0], "read") == 0)
  {
    o->do_read = 1;

    /* stlinkv1 mode */
    if (ac == 5)
    {
      o->devname = av[1];
      i = 1;
    }

    o->size = strtoul(av[i + 3], NULL, 16);
  }
  else if (strcmp(av[0], "write") == 0)
  {
    o->do_read = 0;

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
    sl = stlink_v1_open(50);
    sl->verbose = 50;
    if (sl == NULL) goto on_error;
  }
  else /* stlinkv2 */
  {
    sl = stlink_open_usb(50);
    sl->verbose = 50;
    if (sl == NULL) goto on_error;
  }

  if (stlink_current_mode(sl) == STLINK_DEV_DFU_MODE)
    stlink_exit_dfu_mode(sl);

  if (stlink_current_mode(sl) != STLINK_DEV_DEBUG_MODE)
    stlink_enter_swd_mode(sl);

  stlink_reset(sl);

  if (o.do_read == 0) /* write */
  {
    err = stlink_fwrite_flash(sl, o.filename, o.addr);
    if (err == -1)
    {
      printf("stlink_fwrite_flash() == -1\n");
      goto on_error;
    }
  }
  else /* read */
  {
    err = stlink_fread(sl, o.filename, o.addr, o.size);
    if (err == -1)
    {
      printf("stlink_fread() == -1\n");
      goto on_error;
    }
  }

  /* success */
  err = 0;

 on_error:
  if (sl != NULL)
  {
    stlink_reset(sl);
    stlink_run(sl);
    stlink_close(sl);
  }

  return err;
}
