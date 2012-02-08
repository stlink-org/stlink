CFLAGS+=-g
CFLAGS+=-DDEBUG
CFLAGS+=-std=gnu99
CFLAGS+=-Wall -Wextra
CFLAGS+=-I../src

LDFLAGS=-L.. -lstlink

# libusb location
LDFLAGS+=`pkg-config --libs libusb-1.0`
CFLAGS+=`pkg-config --cflags libusb-1.0`

SRCS=main.c
OBJS=$(SRCS:.c=.o)

NAME=st-flash

all: $(NAME)

$(NAME): $(OBJS) ../libstlink.a
	$(CC) $(CFLAGS) -o $(NAME) $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	rm -f $(OBJS)
	rm -f $(NAME)

.PHONY: clean all
