CC=gcc
CFLAGS=-std=c99 -pedantic -Wall -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes
AS=arm-none-eabi-as
COPY=arm-none-eabi-objcopy

all: armv2 boot.rom

armv2: armv2.c instructions.c init.c armv2.h
	${CC} ${CFLAGS} -g -o $@ armv2.c instructions.c init.c

boot.rom: boot.S
	${AS} -march=armv2 -o boot.o $<
	${COPY} -O binary boot.o $@

clean:
	rm armv2 boot.rom