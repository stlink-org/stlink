# make ... for both libusb and libsg
#
# make CONFIG_USE_LIBSG=0 ...
# for just libusb
#
VPATH=src

SOURCES_LIB=stlink-common.c stlink-usb.c
OBJS_LIB=$(SOURCES_LIB:.c=.o)
TEST_PROGRAMS=test_usb
LDFLAGS=-L. -lstlink -lusb-1.0

ifeq ($(CONFIG_USE_LIBSG),)
CONFIG_USE_LIBSG=1
endif

ifneq ($(CONFIG_USE_LIBSG),0)
SOURCES_LIB+=stlink-sg.c
CFLAGS+=-DCONFIG_USE_LIBSG=1
LDFLAGS+=-lsgutils2
TEST_PROGRAMS+=test_sg
endif

CFLAGS+=-g
CFLAGS+=-DCONFIG_USE_LIBUSB=1
CFLAGS+=-DDEBUG=1
CFLAGS+=-std=gnu99
CFLAGS+=-Wall -Wextra


LIBRARY=libstlink.a

all:  $(LIBRARY) flash gdbserver $(TEST_PROGRAMS)

$(LIBRARY): $(OBJS_LIB)
	@echo "objs are $(OBJS_LIB)"
	$(AR) -cr $@ $^
	@echo "done making library"
	

test_sg: test_sg.o $(LIBRARY)
	@echo "building test_sg"
	$(CC) test_sg.o $(LDFLAGS) -o $@

test_usb: test_usb.o $(LIBRARY)
	@echo "building test_usb"
	$(CC) test_usb.o $(LDFLAGS) -o $@
	@echo "done linking"

%.o: %.c
	@echo "building $^ into $@"
	$(CC) $(CFLAGS) -c $^ -o $@
	@echo "done compiling"

clean:
	rm -rf $(OBJS_LIB)
	rm -rf $(LIBRARY)
	rm -rf test_usb*
	rm -rf test_sg*

distclean: clean
	$(MAKE) -C flash clean
	$(MAKE) -C gdbserver clean
	
flash:
	$(MAKE) -C flash CONFIG_USE_LIBSG="$(CONFIG_USE_LIBSG)" 

gdbserver:
	$(MAKE) -C gdbserver CONFIG_USE_LIBSG="$(CONFIG_USE_LIBSG)"

.PHONY: clean all flash gdbserver
