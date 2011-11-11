PRG := st-util
OBJS = gdb-remote.o gdb-server.o

CFLAGS+=-g -Wall -Werror -std=gnu99 -I../src
CFLAGS+=-DCONFIG_USE_LIBUSB=1
LDFLAGS=-lusb-1.0 -L.. -lstlink 

all: $(PRG)

$(PRG): $(OBJS)
	$(CC) -o $@  $^ $(LDFLAGS)

clean:
	rm -rf $(OBJS)
	rm -rf $(PRG)

.PHONY: clean all
