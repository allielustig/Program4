#include "libTinyFS.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>


int currentMounted = 0;
int fileSize = 0;

/*only call if nBytes > 0*/
int tfs_mkfs(char *filename, int nBytes){
	int i=0;
	char * buf = calloc(BLOCKSIZE, 1);
	
	if (nBytes <= 0){
		return -3;
	}

	int diskNum = openDisk( filename, nBytes);

	if (diskNum > 0){
		currentMounted = diskNum;
		fileSize = nBytes;
	}

	for(i=0; i<fileSize; i+= BLOCKSIZE){
		writeBlock(diskNum, i/BLOCKSIZE, buf)
	}

}