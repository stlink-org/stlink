PRG := st-util
DEBUG := #-DDEBUG

all: $(PRG)

LIBS := \
	-lsgutils2

OBJS += \
	stlink-hw.o gdb-remote.o gdb-server.o

$(PRG): $(OBJS)
	gcc -o $(PRG) $(OBJS) $(LIBS)

%.o: ../src/%.c
	gcc -O3 -g3 -Wall -Werror -c -std=gnu99 -MMD -MP \
		-fno-strict-aliasing -Wno-unused $(DEBUG) \
		-MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)"\
		-o "$@" "$<"

clean:
	@rm -vf *.d *.o $(PRG)
