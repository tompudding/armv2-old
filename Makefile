CC=gcc
AR=ar
CFLAGS=-std=c99 -pedantic -Wall -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes -O3
AS=arm-none-eabi-as
COPY=arm-none-eabi-objcopy

all: armtest _armv2.so boot.rom rijndael

_armv2.so: armv2.a
	python setup.py build_ext --inplace

armtest: armtest.c armv2.a
	${CC} ${CFLAGS} -o $@ $^

armv2.a: armv2.o instructions.o init.o armv2.h
	${AR} rcs $@ armv2.o instructions.o init.o

boot.rom: boot.S rijndael
	${AS} -march=armv2 -mapcs-26 -o boot.o $<
	${COPY} -O binary boot.o boot.bin
	python create.py boot.bin rijndael $@

rijndael: encrypt.c rijndael-alg-fst.c rijndael-alg-fst.h
	arm-none-eabi-gcc -march=armv2 -static -Wa,-mapcs-26 -mno-thumb-interwork -marm -o $@ $^ 

clean:
	rm -f armv2 rijndael boot.rom armtest armv2.o instructions.o init.o _armv2.c _armv2.so *~ armv2.a boot.bin boot.o
	python setup.py clean
