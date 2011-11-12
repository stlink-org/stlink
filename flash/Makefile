# make ... for both libusb and libsg
#
# make CONFIG_USE_LIBSG=0 ...
# for just libusb
#
CC=gcc

CFLAGS+=-g
CFLAGS+=-DCONFIG_USE_LIBUSB=1
CFLAGS+=-DDEBUG
CFLAGS+=-std=gnu99
CFLAGS+=-Wall -Wextra
CFLAGS+=-I../src

LDFLAGS=-L.. -lstlink -lusb-1.0

ifeq ($(CONFIG_USE_LIBSG),)
CONFIG_USE_LIBSG=1
endif

ifneq ($(CONFIG_USE_LIBSG),0)
CFLAGS+=-DCONFIG_USE_LIBSG=1
LDFLAGS+=-lsgutils2
endif

SRCS=main.c
OBJS=$(SRCS:.c=.o)

NAME=flash

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) -o $(NAME) $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	rm -f $(OBJS)
	rm -f $(NAME)

.PHONY: clean all
