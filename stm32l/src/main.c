#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <libusb-1.0/libusb.h>

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

  ctx->flags = TRANS_FLAGS_IS_DONE;
}


static int submit_wait(struct libusb_transfer* trans)
{
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

  while (!(trans_ctx.flags & TRANS_FLAGS_IS_DONE))
  {
    if (libusb_handle_events(NULL))
    {
      printf("libusb_handle_events()\n");
      return -1;
    }
  }

  return 0;
}

static int send_recv
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

  if (submit_wait(handle->req_trans)) return -1;

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

  return submit_wait(handle->rep_trans);
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

      if (libusb_set_configuration(slu->usb_handle, 1))
      {
	printf("libusb_set_configuration()\n");
	goto on_libusb_error;
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

      slu->ep_req = 1 /* ep req */ | LIBUSB_ENDPOINT_OUT;
      slu->ep_rep = 2 /* ep rep */ | LIBUSB_ENDPOINT_IN;

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

      unsigned int i;
      unsigned char buf[6];

      for (i = 0; i < sizeof(buf); ++i) buf[i] = 0;

      buf[0] = 0xf1;
      if (send_recv(slu, buf, sizeof(buf), buf, sizeof(buf)) == -1)
      {
	printf("[!] send_recv\n");
	return ;
      }

      for (i = 0; i < 6; ++i) printf("%02x", buf[i]);
      printf("\n");

      break ;
    }
#endif /* CONFIG_USE_LIBUSB */

  default: break ;
  }
}

int stlink_current_mode(struct stlink *sl)
{
  return -1;
}

void stlink_enter_swd_mode(struct stlink *sl)
{
}

void stlink_enter_jtag_mode(struct stlink *sl)
{
}

void stlink_exit_debug_mode(struct stlink *sl)
{
}

void stlink_core_id(struct stlink *sl)
{
}

void stlink_status(struct stlink *sl)
{
}


/* main */

int main(int ac, char** av)
{
  struct stlink* sl;

  stlink_initialize(TRANSPORT_TYPE_LIBUSB);
  sl = stlink_quirk_open(TRANSPORT_TYPE_LIBUSB, NULL, 0);
  if (sl != NULL)
  {
    stlink_version(sl);
    stlink_status(sl);
    stlink_current_mode(sl);
    stlink_close(sl);
  }
  stlink_finalize(TRANSPORT_TYPE_LIBUSB);

  return 0;
}
