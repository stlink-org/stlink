PRG := st-util
OBJS = gdb-remote.o gdb-server.o

CFLAGS+=-g -Wall -Werror -std=gnu99 -I../src
LDFLAGS=-L.. -lstlink -lusb-1.0

all: $(PRG)

$(PRG): $(OBJS)  ../libstlink.a
	$(CC) -o $@  $^ $(LDFLAGS)

clean:
	rm -rf $(OBJS)
	rm -rf $(PRG)

.PHONY: clean all
