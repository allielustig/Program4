#include "libTinyFS.h"
#include "libDisk.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define SUCCESS 0
#define SUPERBLOCK_OFFSET 0
#define ROOTINODE_OFFSET 1


int currentMounted = 0;
int diskSize = 0;
int open_files_table[256];

int main(){
	tfs_mkfs("bintest.bin", 1024);
	tfs_mount("bintest.bin");
	fileDescriptor tempFD = tfs_openFile("testFile");
	tfs_closeFile(tempFD);
	tfs_unmount();

	return 0;
}
/*only call if nBytes > 0*/
int tfs_mkfs(char *filename, int nBytes){
	int i=0;
	
	if (nBytes <= 0){
		return -3;
	}

	int diskNum = openDisk( filename, nBytes);

	/*if file cannot be opened*/
	if (diskNum < 0){
		return -1;
	}

	/* Writing and Initializing superblock*/
	struct superBlock sb;
	sb.blockCode = SUPERBLOCK_CODE;
	sb.magicNumber = MAGIC_NUMBER;
	sb.rootInodeOffset = 1;
	//initialize to -1
	for (i = 0; i < 253; i++) {
		sb.freeBlockArray[i] = -1;
	}
	//0 means block is used (super & rootiNode)
	sb.freeBlockArray[0] = 0;
	sb.freeBlockArray[1] = 0;
	//set to 1 for existing free blocks
	for (i = 2; i < nBytes/BLOCKSIZE; i++) {
		sb.freeBlockArray[i] = 1;
	}

	struct iNode r_iNode;
	r_iNode.blockCode = INODE_CODE;
	r_iNode.magicNumber = MAGIC_NUMBER;
	memset(r_iNode.fileName, 0, 9);
	strcpy(r_iNode.fileName, "/");
	//r_iNode.fileName = "/";
	r_iNode.fileByteSize = 9; //root iNode not a file
	//initialize file Descriptors map to -1 (no files present)
	for (i = 0; i < 241; i++) {
		r_iNode.dataBlockMap[i] = -1;
	}

	struct freeBlock fb;
	fb.blockCode = FREEBLOCK_CODE;
	fb.magicNumber = MAGIC_NUMBER;
	//initialize to no data
	for (i = 0; i < 254; i++) {
		fb.emptyBytes[i] = -1;
	}

	//write all structs to disk
	if (writeBlock(diskNum, 0, &sb) < 0) {
		return -3;
	}

	if (writeBlock(diskNum, 1, &r_iNode) < 0) {
		return -3;
	}

	for (i = 2; i < nBytes/BLOCKSIZE; i++) {
		if (writeBlock(diskNum, i, &fb) < 0) {
			return -3;
		}
	}

	//initialize open files to -1
	for ( i = 0; i < 256; i++) {
		open_files_table[i] = -1;
	}

	closeDisk(diskNum);

	return SUCCESS;
}

int tfs_mount(char *filename){
	printf("filename is: %s\n", filename);
	if (currentMounted != 0){
		return -2;
	}
	currentMounted = openDisk(filename, 0);
	printf("SETTING CURRENT MOUNTED TO: %d\n", currentMounted);
	return 0;
}

int tfs_unmount(void){
	int i;
	closeDisk(currentMounted);
	currentMounted = 0;

	//initialize open files to -1
	for ( i = 0; i < 256; i++) {
		open_files_table[i] = -1;
	}
	return 0;
}


fileDescriptor tfs_openFile(char *name){
	//search through root iNode to file name
	//if exists, update open file table
	//if not exists, create fd, do all this stuff

	int currentFD;
	int exists = searchINodesByName(name);
	if (!exists) {
		printf("File doesn't exist. Creating Now.\n");
		currentFD = findOpenFD();
		printf("Current FD: %d\n", currentFD);
		if (currentFD >= 0) {
			open_files_table[currentFD] = 0;

			//find free block and make it an iNode
			int fb = getFreeBlock();
			printf("next free block is: %d\n", fb);
			updateRootINode(fb, currentFD);

			createFileINode(fb, name);
		}
	}
	else {
		//it already exists, just need to 'open' it
		open_files_table[exists] = 0;
		currentFD = exists;
	}


	return currentFD;

}

int tfs_closeFile(fileDescriptor FD) {
	open_files_table[FD] = -1;
	return 0;
}

//returns 0 if not found, else return fd
int searchINodesByName(char *name) {
	printf("FILENAME: %s\n", name);
	struct iNode *root = malloc(BLOCKSIZE);
	struct iNode *file = malloc(BLOCKSIZE);

	printf("currentMounted: %d\n", currentMounted);
	readBlock(currentMounted, ROOTINODE_OFFSET, root);
	int i;
	for (i = 0; i < 241; i++) {
		if(root->dataBlockMap[i] != -1){
			//need to read in that file iNode, check its name
			readBlock(currentMounted, i, file);
			//check name
			if(!strcmp(file->fileName, name)){
				return root->dataBlockMap[i];
			}
		}
	}
	free(root);
	free(file);
	return 0;
}

int getFreeBlock() {
	struct superBlock *superBuf = malloc(BLOCKSIZE);
	readBlock(currentMounted, SUPERBLOCK_OFFSET, superBuf);

	int i = 0;
	while (superBuf->freeBlockArray[i] != 1 && superBuf->freeBlockArray[i] != -1) {
		i++;
	}

	if (superBuf->freeBlockArray[i] == -1) {
		return -4;//out of data error
	}
	else {
		superBuf->freeBlockArray[i] = 0; //now it's going to be used
		writeBlock(currentMounted, SUPERBLOCK_OFFSET, superBuf);
		return i;
	}

}

void updateRootINode(int freeBlock, int currentFD) {
	struct iNode *ibuf = malloc(BLOCKSIZE);
	readBlock(currentMounted, ROOTINODE_OFFSET, ibuf);

	printf("--update root node--\n");
	printf("freeblock: %d\n", freeBlock);
	printf("currentFD: %d\n", currentFD);
	printf("----------------------\n");
	(ibuf->dataBlockMap)[freeBlock] = currentFD;
	int writestat = writeBlock(currentMounted, ROOTINODE_OFFSET, ibuf);
	printf("writestat: %d\n", writestat);
}

void createFileINode(int fb, char *name) {
	struct iNode fileINode;
	fileINode.blockCode = INODE_CODE;
	fileINode.magicNumber = MAGIC_NUMBER;
	strcpy(fileINode.fileName, name);
	fileINode.fileByteSize = 0;
	int i;
	for (i = 0; i < 241; i++) {
		fileINode.dataBlockMap[i] = -1;
	}

	writeBlock(currentMounted, fb, &fileINode);
}

int findOpenFD(){
	int i=0;

	//index to first open file descriptor
	while(open_files_table[i] != -1){
		i++;
	}

	return i;
}





