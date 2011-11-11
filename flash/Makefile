CFLAGS+=-g
CFLAGS+=-DCONFIG_USE_LIBUSB=1
CFLAGS+=-DDEBUG
CFLAGS+=-std=gnu99
CFLAGS+=-Wall -Wextra
CFLAGS+=-I../src

LDFLAGS=-lusb-1.0 -L.. -lstlink 

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
