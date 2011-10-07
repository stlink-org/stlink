
VPATH=src

SOURCES_LIB=stlink-common.c stlink-usb.c stlink-sg.c
OBJS_LIB=$(SOURCES_LIB:.c=.o)

CFLAGS+=-g
CFLAGS+=-DCONFIG_USE_LIBUSB
CFLAGS+=-DCONFIG_USE_LIBSG
CFLAGS+=-DDEBUG
CFLAGS+=-std=gnu99
CFLAGS+=-Wall -Wextra

LDFLAGS=-lstlink -lusb-1.0 -lsgutils2 -L.

LIBRARY=libstlink.a

all:  $(LIBRARY) test_usb test_sg 

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
	
.PHONY: clean all
