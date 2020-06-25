###
# Makefile for STlink-v1 support
###

VPATH=src

SOURCES_LIB=common.c usb.c sg.c logging.c
OBJS_LIB=$(SOURCES_LIB:.c=.o)
TEST_PROGRAMS=test-flash test-sg test-usb
LDFLAGS=-L. -lstlink -lusb-1.0

CFLAGS+=-g
CFLAGS+=-DDEBUG=1
CFLAGS+=-std=gnu11
CFLAGS+=-Wall -Wextra


LIBRARY=libstlink.a

all:  $(LIBRARY) flash gdbserver $(TEST_PROGRAMS)

$(LIBRARY): $(OBJS_LIB)
	@echo "objs are $(OBJS_LIB)"
	$(AR) -cr $@ $^
	@echo "Compilation of library completed."


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
	rm -rf test-flash* test-sg* test-usb*
	$(MAKE) -C flash clean
	$(MAKE) -C gdbserver clean

flash:
	$(MAKE) -C flash

gdbserver:
	$(MAKE) -C gdbserver CONFIG_USE_LIBSG="$(CONFIG_USE_LIBSG)"

macos_stlink_shield:
	./install.sh

.PHONY: clean all flash gdbserver
