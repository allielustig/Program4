

CC = gcc
CFLAGS = -g -Wall -Werror


all: tinyFsDemo

tinyFsDemo: libTinyFS tinyFsDemo.c 
	$(CC) $(CFLAGS) -o tinyFsDemo libDisk.o libTinyFS.o tinyFsDemo.c


libTinyFS: libDisk libTinyFS.c libTinyFS.h libTinyFS.o 
	$(CC) $(CFLAGS) -c libTinyFS.c libDisk.c


libDisk: libDisk.c libDisk.h libDisk.o TinyFS_errno.h
	$(CC) $(CFLAGS) -c libDisk.c

clean:
	rm -f tinyFsDemo libDisk.o libTinyFS.o
