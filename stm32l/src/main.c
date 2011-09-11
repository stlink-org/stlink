#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <sys/types.h>
#include <libusb-1.0/libusb.h>
#include "stlink-hw.h"

/* endianess related */
static inline unsigned int is_bigendian(void)
{
  static volatile const unsigned int i = 1;
  return *(volatile const char*) &i == 0;
}

static void write_uint32(unsigned char* buf, uint32_t ui)
{
  if (!is_bigendian()) { // le -> le (don't swap)
    buf[0] = ((unsigned char*) &ui)[0];
    buf[1] = ((unsigned char*) &ui)[1];
    buf[2] = ((unsigned char*) &ui)[2];
    buf[3] = ((unsigned char*) &ui)[3];
  } else {
    buf[0] = ((unsigned char*) &ui)[3];
    buf[1] = ((unsigned char*) &ui)[2];
    buf[2] = ((unsigned char*) &ui)[1];
    buf[3] = ((unsigned char*) &ui)[0];
  }
}

static void write_uint16(unsigned char* buf, uint16_t ui)
{
  if (!is_bigendian()) { // le -> le (don't swap)
    buf[0] = ((unsigned char*) &ui)[0];
    buf[1] = ((unsigned char*) &ui)[1];
  } else {
    buf[0] = ((unsigned char*) &ui)[1];
    buf[1] = ((unsigned char*) &ui)[0];
  }
}

static uint32_t read_uint32(const unsigned char *c, const int pt)
{
  uint32_t ui;
  char *p = (char *) &ui;

  if (!is_bigendian()) { // le -> le (don't swap)
    p[0] = c[pt];
    p[1] = c[pt + 1];
    p[2] = c[pt + 2];
    p[3] = c[pt + 3];
  } else {
    p[0] = c[pt + 3];
    p[1] = c[pt + 2];
    p[2] = c[pt + 1];
    p[3] = c[pt];
  }
  return ui;
}

static uint16_t read_uint16(const unsigned char *c, const int pt)
{
  uint32_t ui;
  char *p = (char *) &ui;

  if (!is_bigendian()) { // le -> le (don't swap)
    p[0] = c[pt];
    p[1] = c[pt + 1];
  } else {
    p[0] = c[pt + 1];
    p[1] = c[pt];
  }
  return ui;
}

/* libusb transport layer */

libusb_context* libusb_ctx =  NULL;

struct stlink_libusb
{
  libusb_device_handle* usb_handle;
  struct libusb_transfer* req_trans;
  struct libusb_transfer* rep_trans;
  unsigned int ep_req;
  unsigned int ep_rep;
};

struct trans_ctx
{
#define TRANS_FLAGS_IS_DONE (1 << 0)
#define TRANS_FLAGS_HAS_ERROR (1 << 1)
  volatile unsigned long flags;
};

static void on_trans_done(struct libusb_transfer* trans)
{
  struct trans_ctx* const ctx = trans->user_data;

  if (trans->status != LIBUSB_TRANSFER_COMPLETED)
    ctx->flags |= TRANS_FLAGS_HAS_ERROR;

  ctx->flags |= TRANS_FLAGS_IS_DONE;
}


static int submit_wait(struct libusb_transfer* trans)
{
  struct timeval start;
  struct timeval now;
  struct timeval diff;
  struct trans_ctx trans_ctx;
  enum libusb_error error;

  trans_ctx.flags = 0;

  /* brief intrusion inside the libusb interface */
  trans->callback = on_trans_done;
  trans->user_data = &trans_ctx;

  if ((error = libusb_submit_transfer(trans)))
  {
    printf("libusb_submit_transfer(%d)\n", error);
    return -1;
  }

  gettimeofday(&start, NULL);

  while (trans_ctx.flags == 0)
  {
    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    if (libusb_handle_events_timeout(libusb_ctx, &timeout))
    {
      printf("libusb_handle_events()\n");
      return -1;
    }

    gettimeofday(&now, NULL);
    timersub(&now, &start, &diff);
    if (diff.tv_sec >= 3)
    {
      printf("libusb_handle_events() timeout\n");
      return -1;
    }
  }

  if (trans_ctx.flags & TRANS_FLAGS_HAS_ERROR)
  {
    printf("libusb_handle_events() | has_error\n");
    return -1;
  }

  return 0;
}

static ssize_t send_recv
(
 struct stlink_libusb* handle,
 unsigned char* txbuf, size_t txsize,
 unsigned char* rxbuf, size_t rxsize
)
{
  /* note: txbuf and rxbuf can point to the same area */

  libusb_fill_bulk_transfer
  (
   handle->req_trans,
   handle->usb_handle,
   handle->ep_req,
   txbuf, txsize,
   NULL, NULL,
   0
  );

  printf("submit_wait(req)\n");

  if (submit_wait(handle->req_trans)) return -1;

  /* send_only */
  if (rxsize == 0) return 0;

  /* read the response */

  libusb_fill_bulk_transfer
  (
   handle->rep_trans,
   handle->usb_handle,
   handle->ep_rep,
   rxbuf, rxsize,
   NULL, NULL,
   0
  );

  printf("submit_wait(rep)\n");

  if (submit_wait(handle->rep_trans)) return -1;

  return handle->rep_trans->actual_length;
}


static inline int send_only
(struct stlink_libusb* handle, unsigned char* txbuf, size_t txsize)
{
  return send_recv(handle, txbuf, txsize, NULL, 0);
}


/* stlink layer independant interface */

enum transport_type
{
  TRANSPORT_TYPE_ZERO = 0,
#if CONFIG_USE_LIBSG
  TRANSPORT_TYPE_LIBSG,
#endif /* CONFIG_USE_LIBSG */
#if CONFIG_USE_LIBUSB
  TRANSPORT_TYPE_LIBUSB,
#endif /* CONFIG_USE_LIBUSB */
  TRANSPORT_TYPE_INVALID
};

struct stlink
{
  enum transport_type tt;
  union
  {
#if CONFIG_USE_LIBUSB
    struct stlink_libusb libusb;
#endif /* CONFIG_USE_LIBUSB */
#if CONFIG_USE_LIBSG
    void* libsg;
#endif /* CONFIG_USE_LIBSG */
  } transport;

  unsigned char q_buf[64];

  /* layer independant */
  uint32_t core_id;
};

int stlink_initialize(enum transport_type tt)
{
  switch (tt)
  {
#if CONFIG_USE_LIBUSB
  case TRANSPORT_TYPE_LIBUSB:
    {
      if (libusb_ctx != NULL) return -1;
      if (libusb_init(&libusb_ctx))
      {
	printf("libusb_init()\n");
	return -1;
      }
      break ;
    }
#endif /* CONFIG_USE_LIBUSB */

  default: break ;
  }

  return 0;
}

void stlink_finalize(enum transport_type tt)
{
  switch (tt)
  {
#if CONFIG_USE_LIBUSB
  case TRANSPORT_TYPE_LIBUSB:
    {
      libusb_exit(libusb_ctx);
      break ;
    }
#endif /* CONFIG_USE_LIBUSB */

  default: break;
  }
}

#if CONFIG_USE_LIBUSB
static int is_stlink_device(libusb_device* dev)
{
  struct libusb_device_descriptor desc;

  if (libusb_get_device_descriptor(dev, &desc))
    return 0;

  printf("device: 0x%04x, 0x%04x\n", desc.idVendor, desc.idProduct);
 
  if (desc.idVendor != 0x0483)
    return 0;

  if (desc.idProduct != 0x3748)
    return 0;

  return 1;
}
#endif /* CONFIG_USE_LIBUSB */

/* fwd decl */
void stlink_close(struct stlink*);

struct stlink* stlink_quirk_open
(enum transport_type tt, const char *dev_name, const int verbose)
{
  struct stlink* sl = NULL;

  sl = malloc(sizeof(struct stlink));
  if (sl == NULL) goto on_error;

  sl->tt = tt;

  switch (tt)
  {
#if CONFIG_USE_LIBUSB
  case TRANSPORT_TYPE_LIBUSB:
    {
      struct stlink_libusb* const slu = &sl->transport.libusb;

      int error = -1;

      libusb_device** devs = NULL;
      libusb_device* dev;
      ssize_t i;
      ssize_t count;
      int config;

      count = libusb_get_device_list(libusb_ctx, &devs);
      if (count < 0)
      {
	printf("libusb_get_device_list\n");
	goto on_libusb_error;
      }

      for (i = 0; i < count; ++i)
      {
	dev = devs[i];
	if (is_stlink_device(dev)) break;
      }
      if (i == count) return NULL;

      if (libusb_open(dev, &slu->usb_handle))
      {
	printf("libusb_open()\n");
	goto on_libusb_error;
      }

      if (libusb_get_configuration(slu->usb_handle, &config))
      {
	/* this may fail for a previous configured device */
	printf("libusb_get_configuration()\n");
	goto on_libusb_error;
      }

      if (config != 1)
      {
	printf("setting new configuration (%d -> 1)\n", config);
	if (libusb_set_configuration(slu->usb_handle, 1))
	{
	  /* this may fail for a previous configured device */
	  printf("libusb_set_configuration()\n");
	  goto on_libusb_error;
	}
      }

      if (libusb_claim_interface(slu->usb_handle, 0))
      {
	printf("libusb_claim_interface()\n");
	goto on_libusb_error;
      }

      slu->req_trans = libusb_alloc_transfer(0);
      if (slu->req_trans == NULL)
      {
	printf("libusb_alloc_transfer\n");
	goto on_libusb_error;
      }

      slu->rep_trans = libusb_alloc_transfer(0);
      if (slu->rep_trans == NULL)
      {
	printf("libusb_alloc_transfer\n");
	goto on_libusb_error;
      }

      slu->ep_rep = 1 /* ep rep */ | LIBUSB_ENDPOINT_IN;
      slu->ep_req = 2 /* ep req */ | LIBUSB_ENDPOINT_OUT;

      /* libusb_reset_device(slu->usb_handle); */

      /* success */
      error = 0;

    on_libusb_error:
      if (devs != NULL) libusb_free_device_list(devs, 1);

      if (error == -1)
      {
	stlink_close(sl);
	return NULL;
      }

      break ;
    }
#endif /* CONFIG_USE_LIBUSB */

#if CONFIG_USE_LIBSG
  case transport_type_libsg:
    {
      break ;
    }
#endif /* CONFIG_USE_LIBSG */

  default: break ;
  }

  /* success */
  return sl;

 on_error:
  if (sl != NULL) free(sl);
  return 0;
}

void stlink_close(struct stlink *sl)
{
  switch (sl->tt)
  {
#if CONFIG_USE_LIBUSB
  case TRANSPORT_TYPE_LIBUSB:
    {
      struct stlink_libusb* const handle = &sl->transport.libusb;

      if (handle->req_trans != NULL)
	libusb_free_transfer(handle->req_trans);

      if (handle->rep_trans != NULL)
	libusb_free_transfer(handle->rep_trans);

      if (handle->usb_handle != NULL)
	libusb_close(handle->usb_handle);

      break ;
    }
#endif /* CONFIG_USE_LIBUSB */

#if CONFIG_USE_LIBSG
  case TRANSPORT_TYPE_LIBSG:
    {
      break ;
    }
#endif /* CONFIG_USE_LIBSG */

  default: break ;
  }

  free(sl);
}

void stlink_version(struct stlink* sl)
{
  switch (sl->tt)
  {
#if CONFIG_USE_LIBUSB
  case TRANSPORT_TYPE_LIBUSB:
    {
      struct stlink_libusb* const slu = &sl->transport.libusb;
      unsigned char* const buf = sl->q_buf;
      ssize_t size;

      memset(buf, 0, sizeof(sl->q_buf));
      buf[0] = STLINK_GET_VERSION;
      buf[1] = 0x80;

      size = send_recv(slu, buf, 16, buf, sizeof(sl->q_buf));
      if (size == -1)
      {
	printf("[!] send_recv\n");
	return ;
      }

#if 1 /* DEBUG */
      {
	unsigned int i;
	for (i = 0; i < size; ++i) printf("%02x", buf[i]);
	printf("\n");
      }
#endif /* DEBUG */

      break ;
    }
#endif /* CONFIG_USE_LIBUSB */

  default: break ;
  }
}

int stlink_current_mode(struct stlink *sl)
{
  int mode = -1;

  switch (sl->tt)
  {
#if CONFIG_USE_LIBUSB
  case TRANSPORT_TYPE_LIBUSB:
    {
      struct stlink_libusb* const slu = &sl->transport.libusb;
      unsigned char* const buf = sl->q_buf; 
      ssize_t size;

      memset(buf, 0, sizeof(sl->q_buf));

      buf[0] = STLINK_GET_CURRENT_MODE;

      size = send_recv(slu, buf, 16, buf, sizeof(sl->q_buf));
      if (size == -1)
      {
	printf("[!] send_recv\n");
	return -1;
      }

      /* mode = (int)read_uint16(buf, 0); */
      mode = (int)buf[0];

#if 1 /* DEBUG */
      printf("mode == 0x%x\n", mode);
#endif /* DEBUG */

      break ;
    }
#endif /* CONFIG_USE_LIBUSB */

  default: break ;
  }

  return mode;
}

void stlink_core_id(struct stlink *sl)
{
  switch (sl->tt)
  {
#if CONFIG_USE_LIBUSB
  case TRANSPORT_TYPE_LIBUSB:
    {
      struct stlink_libusb* const slu = &sl->transport.libusb;
      unsigned char* const buf = sl->q_buf; 
      ssize_t size;

      memset(buf, 0, sizeof(sl->q_buf));
      buf[0] = STLINK_DEBUG_COMMAND;
      buf[1] = STLINK_DEBUG_READCOREID;

      size = send_recv(slu, buf, 16, buf, sizeof(sl->q_buf));
      if (size == -1)
      {
	printf("[!] send_recv\n");
	return ;
      }

      sl->core_id = read_uint32(buf, 0);

#if 1 /* DEBUG */
      printf("core_id == 0x%x\n", sl->core_id);
#endif /* DEBUG */

      break ;
    }
#endif /* CONFIG_USE_LIBUSB */

  default: break ;
  }
}

void stlink_status(struct stlink *sl)
{
  switch (sl->tt)
  {
#if CONFIG_USE_LIBUSB
  case TRANSPORT_TYPE_LIBUSB:
    {
      struct stlink_libusb* const slu = &sl->transport.libusb;
      unsigned char* const buf = sl->q_buf;
      ssize_t size;

      memset(buf, 0, sizeof(sl->q_buf));

      buf[0] = STLINK_DEBUG_COMMAND;
      buf[1] = STLINK_DEBUG_GETSTATUS;

      size = send_recv(slu, buf, 16, buf, sizeof(sl->q_buf));
      if (size == -1)
      {
	printf("[!] send_recv\n");
	return ;
      }

      /* todo: stlink_core_stat */

#if 1 /* DEBUG */
      printf("status == 0x%x\n", buf[0]);
#endif /* DEBUG */

      break ;
    }
#endif /* CONFIG_USE_LIBUSB */

  default: break ;
  }
}

void stlink_enter_swd_mode(struct stlink *sl)
{
  switch (sl->tt)
  {
#if CONFIG_USE_LIBUSB
  case TRANSPORT_TYPE_LIBUSB:
    {
      struct stlink_libusb* const slu = &sl->transport.libusb;
      unsigned char* const buf = sl->q_buf;
      ssize_t size;

      memset(buf, 0, sizeof(sl->q_buf));

      buf[0] = STLINK_DEBUG_COMMAND;
      buf[1] = 0x30; /* magic byte */
      buf[2] = STLINK_DEBUG_ENTER_JTAG;

      size = send_recv(slu, buf, 16, buf, sizeof(sl->q_buf));
      if (size == -1)
      {
	printf("[!] send_recv\n");
	return ;
      }

      break ;
    }
#endif /* CONFIG_USE_LIBUSB */

  default: break ;
  }
}

void stlink_exit_dfu_mode(struct stlink *sl)
{
  switch (sl->tt)
  {
#if CONFIG_USE_LIBUSB
  case TRANSPORT_TYPE_LIBUSB:
    {
      struct stlink_libusb* const slu = &sl->transport.libusb;
      unsigned char* const buf = sl->q_buf;
      ssize_t size;

      memset(buf, 0, sizeof(sl->q_buf));
      buf[0] = STLINK_DFU_COMMAND;
      buf[1] = STLINK_DFU_EXIT;

      size = send_only(slu, buf, 16);
      if (size == -1)
      {
	printf("[!] send_recv\n");
	return ;
      }

      break ;
    }
#endif /* CONFIG_USE_LIBUSB */

  default: break ;
  }
}

void stlink_reset(struct stlink *sl)
{
  switch (sl->tt)
  {
#if CONFIG_USE_LIBUSB
  case TRANSPORT_TYPE_LIBUSB:
    {
      struct stlink_libusb* const slu = &sl->transport.libusb;
      unsigned char* const buf = sl->q_buf;
      ssize_t size;

      memset(buf, 0, sizeof(sl->q_buf));
      buf[0] = STLINK_DEBUG_COMMAND;
      buf[1] = STLINK_DEBUG_RESETSYS;

      size = send_recv(slu, buf, 16, buf, sizeof(sl->q_buf));
      if (size == -1)
      {
	printf("[!] send_recv\n");
	return ;
      }

      break ;
    }
#endif /* CONFIG_USE_LIBUSB */

  default: break ;
  }
}

void stlink_step(struct stlink *sl)
{
  switch (sl->tt)
  {
#if CONFIG_USE_LIBUSB
  case TRANSPORT_TYPE_LIBUSB:
    {
      struct stlink_libusb* const slu = &sl->transport.libusb;
      unsigned char* const buf = sl->q_buf;
      ssize_t size;

      memset(buf, 0, sizeof(sl->q_buf));
      buf[0] = STLINK_DEBUG_COMMAND;
      buf[1] = STLINK_DEBUG_STEPCORE;

      size = send_recv(slu, buf, 16, buf, sizeof(sl->q_buf));
      if (size == -1)
      {
	printf("[!] send_recv\n");
	return ;
      }

      break ;
    }
#endif /* CONFIG_USE_LIBUSB */

  default: break ;
  }
}

void stlink_run(struct stlink *sl)
{
  switch (sl->tt)
  {
#if CONFIG_USE_LIBUSB
  case TRANSPORT_TYPE_LIBUSB:
    {
      struct stlink_libusb* const slu = &sl->transport.libusb;
      unsigned char* const buf = sl->q_buf;
      ssize_t size;

      memset(buf, 0, sizeof(sl->q_buf));
      buf[0] = STLINK_DEBUG_COMMAND;
      buf[1] = STLINK_DEBUG_RUNCORE;

      size = send_recv(slu, buf, 16, buf, sizeof(sl->q_buf));
      if (size == -1)
      {
	printf("[!] send_recv\n");
	return ;
      }

      break ;
    }
#endif /* CONFIG_USE_LIBUSB */

  default: break ;
  }
}

void stlink_exit_debug_mode(struct stlink *sl)
{
  switch (sl->tt)
  {
#if CONFIG_USE_LIBUSB
  case TRANSPORT_TYPE_LIBUSB:
    {
      struct stlink_libusb* const slu = &sl->transport.libusb;
      unsigned char* const buf = sl->q_buf;
      ssize_t size;

      memset(buf, 0, sizeof(sl->q_buf));
      buf[0] = STLINK_DEBUG_COMMAND;
      buf[1] = STLINK_DEBUG_EXIT;

      size = send_only(slu, buf, 16);
      if (size == -1)
      {
	printf("[!] send_only\n");
	return ;
      }

      break ;
    }
#endif /* CONFIG_USE_LIBUSB */

  default: break ;
  }
}


/* main */

int main(int ac, char** av)
{
  struct stlink* sl;

  stlink_initialize(TRANSPORT_TYPE_LIBUSB);
  sl = stlink_quirk_open(TRANSPORT_TYPE_LIBUSB, NULL, 0);
  if (sl != NULL)
  {
    printf("-- version\n");
    stlink_version(sl);

    if (stlink_current_mode(sl) == STLINK_DEV_DFU_MODE)
    {
      printf("-- exit_dfu_mode\n");
      stlink_exit_dfu_mode(sl);
    }

    printf("-- enter_swd_mode\n");
    stlink_enter_swd_mode(sl);

    printf("-- current_mode\n");
    stlink_current_mode(sl);

    printf("-- core_id\n");
    stlink_core_id(sl);

    printf("-- status\n");
    stlink_status(sl);

    printf("-- reset\n");
    stlink_reset(sl);

    printf("-- status\n");
    stlink_status(sl);

    printf("-- step\n");
    stlink_step(sl);
    getchar();

    printf("-- run\n");
    stlink_run(sl);

    printf("-- exit_debug_mode\n");
    stlink_exit_debug_mode(sl);

    stlink_close(sl);
  }
  stlink_finalize(TRANSPORT_TYPE_LIBUSB);

  return 0;
}
