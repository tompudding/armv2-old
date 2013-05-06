CC=gcc
CFLAGS=-std=c99 -pedantic -Wall -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes
AS=arm-none-eabi-as

armv2: armv2.c armv2.h
	${CC} ${CFLAGS} -o $@ $<

boot.rom: boot.S
	${AS} -march=armv2 -o boot.o $<
	${COPY} -O binary boot.o $@