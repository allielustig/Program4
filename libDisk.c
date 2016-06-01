#include "libDisk.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


int openDisk(char *filename, int nBytes){
	int diskNum = -1;

	if (nBytes > 0){
		diskNum = open(filename, O_RDWR | O_CREAT);
	}

	else if (nBytes == 0){
		diskNum = open(filename, O_RDWR);
	}

	return diskNum;
}


int readBlock(int disk, int bNum, void *block){

}


void closeDisk(int disk){
	close(disk);
}