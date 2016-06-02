#include "libDisk.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "TinyFS_errno.h"

int openDisk(char *filename, int nBytes){
	int diskNum;

	if (nBytes > 0){
		char * buf = calloc(nBytes, 1);
		diskNum = open(filename, O_RDWR | O_CREAT, S_IRUSR|S_IWUSR| S_IROTH| S_IWOTH);

		write(diskNum, buf, nBytes);
		free(buf);
	}

	else if (nBytes == 0){
		diskNum = open(filename, O_RDWR);
	}

	return diskNum < 0 ? OPEN_ERROR : diskNum;
}


int readBlock(int disk, int bNum, void *block){
	int seek_error = -1;
	int read_error = -1;
	//int fileSize = lseek(disk, 0, SEEK_END);
	seek_error = lseek(disk, bNum * BLOCKSIZE, SEEK_SET);

	if (seek_error >= 0){
		
		read_error = read(disk, block, BLOCKSIZE);
		if (read_error > 1){
			return SUCCESS;
		}
		else {
			return READ_ERROR;
		}

	}
	return READ_ERROR;
}

int writeBlock(int disk, int bNum, void *block){

	int seekRetVal = lseek(disk, bNum * BLOCKSIZE, SEEK_SET);
	int writeRetVal = -1;
	if (seekRetVal == EBADF) {
		return WRITE_ERROR;
		//-1 means the file is not open
	}
	else if (seekRetVal == EOVERFLOW) {
		return WRITE_ERROR;
		//-2 means the index is out of bounds?
	}
	else {
		//lseek was successful. Write buffer
		writeRetVal = write(disk, block, BLOCKSIZE);
		return writeRetVal > 0 ? SUCCESS : WRITE_ERROR;
		//return 0 on success. Still need to define all errors
	}
}


void closeDisk(int disk){
	close(disk);
}
