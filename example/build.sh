#!/usr/bin/env sh

CHAIN=$HOME/sat/bin/arm-none-eabi
PATH=$HOME/sat/arm-none-eabi/bin:$PATH

$CHAIN-gcc \
    -O2 \
    -mlittle-endian \
    -mthumb \
    -mcpu=cortex-m3 \
    -ffreestanding \
    -nostdlib \
    -nostdinc \
    main.c

$CHAIN-objcopy \
    -O binary \
    a.out \
    /tmp/foobar