CC=gcc
AR=ar
CFLAGS=-std=c99 -pedantic -Wall -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes -O3 -fPIC
AS=arm-none-eabi-as
COPY=arm-none-eabi-objcopy

all: armtest armv2.so boot.rom rijndael

run: armv2.so boot.rom emulate.py debugger.py
	python emulate.py

armv2.so: libarmv2.a armv2.pyx carmv2.pxd
	python setup.py build_ext --inplace

armtest: armtest.c libarmv2.a
	${CC} ${CFLAGS} -o $@ $^

libarmv2.a: step.o instructions.o init.o armv2.h
	${AR} rcs $@ step.o instructions.o init.o

boot.rom: boot.S rijndael
	${AS} -march=armv2a -mapcs-26 -o boot.o $<
	${COPY} -O binary boot.o boot.bin
	python create.py boot.bin rijndael $@

rijndael: encrypt.c rijndael-alg-fst.c rijndael-alg-fst.h
	arm-none-eabi-gcc -march=armv2 -static -Wa,-mapcs-26 -mno-thumb-interwork -marm -o $@ $^ 

clean:
	rm -f armv2 rijndael boot.rom armtest step.o instructions.o init.o armv2.c armv2.so *~ libarmv2.a boot.bin boot.o
	python setup.py clean
