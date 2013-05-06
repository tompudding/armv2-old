CC=gcc
CFLAGS=-std=c99 -pedantic -Wall -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes

armv2: armv2.c armv2.h
	${CC} ${CFLAGS} -o $@ $<