
PRG := st-util
OBJS = gdb-remote.o gdb-server.o

CFLAGS+=-g -Wall -Werror -std=gnu99 -I../src
CFLAGS+=-DCONFIG_USE_LIBUSB
CFLAGS+=-DCONFIG_USE_LIBSG
LIBS := -lstlink -lusb-1.0 -lsgutils2
LDFLAGS+=$(LIBS) -L..


all: $(PRG)

$(PRG): $(OBJS)
	$(CC) -o $@  $^ $(LDFLAGS)

clean:
	rm -rf $(OBJS)
	rm -rf $(PRG)

.PHONY: clean all
