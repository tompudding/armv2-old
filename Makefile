CC=gcc
CFLAGS=-std=c99 -pedantic -Wall -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes
AS=arm-none-eabi-as
COPY=arm-none-eabi-objcopy

all: armv2 boot.rom rijndael

armv2: armv2.c instructions.c init.c armv2.h
	${CC} ${CFLAGS} -O3 -o $@ armv2.c instructions.c init.c

boot.rom: boot.S rijndael
	${AS} -march=armv2 -o boot.o $<
	${COPY} -O binary boot.o boot.bin
	python create.py boot.bin rijndael $@

rijndael: encrypt.c rijndael-alg-fst.c rijndael-alg-fst.h
	arm-none-eabi-gcc -march=armv2 -static -Wa,-mapcs-26 -mno-thumb-interwork -marm -o $@ $^ 

clean:
	rm -f armv2 rijndael boot.rom

