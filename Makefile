

CC = gcc
CFLAGS = -g -Wall -Werror


all: tinyFsDemo

tinyFsDemo: libDisk tinyFS tinyFSDemo.c
	$(CC) $(CFLAGS) -o tinyFsDemo libDisk.c tinyFS.c tinyFSDemo.c


tinyFs: libDisk tinyFS.c tinyFS.h tinyFs.o
	$(CC) $(CFLAGS) -c libDisk.c tinyFS.c


libDisk: libDisk.c libDisk.h libDisk.o
	$(CC) $(CFLAGS) -c libDisk.c

clean:
	rm -f tinyFsDemo libDisk tinyFS
