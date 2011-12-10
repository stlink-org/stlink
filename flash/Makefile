CFLAGS+=-g
CFLAGS+=-DDEBUG
CFLAGS+=-std=gnu99
CFLAGS+=-Wall -Wextra
CFLAGS+=-I../src

LDFLAGS=-L.. -lstlink -lusb-1.0

SRCS=main.c
OBJS=$(SRCS:.c=.o)

NAME=flash

all: $(NAME)

$(NAME): $(OBJS) ../libstlink.a
	$(CC) $(CFLAGS) -o $(NAME) $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	rm -f $(OBJS)
	rm -f $(NAME)

.PHONY: clean all
