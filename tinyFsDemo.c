#include "libTinyFS.h"
#include "libDisk.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "TinyFS_errno.h"

int main(){
	char * buffer = malloc(2*BLOCKSIZE);
	char * buf2 = malloc(1);
	int i;
	for (i=0; i<2*BLOCKSIZE; i++){
		buffer[i] = i;
	}

	tfs_mkfs("bintest.bin", 2048);
	tfs_mount("bintest.bin");
	fileDescriptor tempFD = tfs_openFile("testFile");
	tfs_writeFile(tempFD, buffer, 2*BLOCKSIZE);
	tfs_seek(tempFD, 50);
	tfs_readByte(tempFD, buf2);
	tfs_closeFile(tempFD);
	tfs_unmount();

	return 0;
}


