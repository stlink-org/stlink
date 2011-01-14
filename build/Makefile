PRG := stlink-access-test
DEV := /dev/sg2

all: $(PRG)

LIBS := \
	-lsgutils2

OBJS += \
	stlink-access-test.o 

$(PRG): $(OBJS)
	@echo 'Invoking: GCC C Linker'
	gcc  -o$(PRG) $(OBJS) $(LIBS)

%.o: ../src/%.c
	@echo 'Building file: $<'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -std=gnu99 -MMD -MP \
		-MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)"\
		-o"$@" "$<"

clean:
	@rm -vf *.d *.o $(PRG)

run: all
	cp $(PRG) /tmp/
	sudo /tmp/$(PRG) $(DEV)
