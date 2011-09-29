PRG := stm32l
DEBUG := # -DDEBUG
CONFIG := -DCONFIG_USE_LIBUSB=1
CFLAGS+= -I../../src

all: $(PRG)

LIBS := \
	-L$(HOME)/install/lib -lusb-1.0

OBJS += \
	main.o

$(PRG): $(OBJS)
	gcc -o $(PRG) $(OBJS) $(LIBS)

%.o: ../src/%.c
	gcc -O3 -g3 -Wall -Werror -c -std=gnu99 -MMD -MP \
		-fno-strict-aliasing -Wno-unused $(DEBUG) $(CONFIG) \
                $(CFLAGS) \
		-MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)"\
		-o "$@" "$<"

clean:
	@rm -vf *.d *.o $(PRG)
