#include "libTinyFS.h"
#include "libDisk.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "TinyFS_errno.h"

#define SUPERBLOCK_OFFSET 0
#define ROOTINODE_OFFSET 1


int currentMounted = 0;
int diskSize = 0;
int open_files_table[256];

/*only call if nBytes > 0*/
int tfs_mkfs(char *filename, int nBytes){
	int i=0;
	
	if (nBytes <= 0){
		return INVALID_PARAM;
	}

	int diskNum = openDisk( filename, nBytes);

	/*if file cannot be opened*/
	if (diskNum < 0){
		return OPEN_ERROR;
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
		return WRITE_ERROR;
	}

	if (writeBlock(diskNum, 1, &r_iNode) < 0) {
		return WRITE_ERROR;
	}

	for (i = 2; i < nBytes/BLOCKSIZE; i++) {
		if (writeBlock(diskNum, i, &fb) < 0) {
			return WRITE_ERROR;
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
	if (currentMounted != 0){
		return MOUNT_ERROR;
	}
	currentMounted = openDisk(filename, 0);
	return checkFSType();
}


int checkFSType(){
	//loop through each block and check that its 0x45
	int diskSize = lseek(currentMounted, 0, SEEK_END);
	int numBlocks = diskSize/BLOCKSIZE;
	int i=0;
	struct freeBlock *fb = calloc(BLOCKSIZE,1);
	for(i=0; i<numBlocks; i++){
		if(readBlock(currentMounted, i, fb) < 0){
			return READ_ERROR;
		}
		if (fb->magicNumber != 0x45){
			return MOUNT_ERROR; //wrong type
		}

	}
	return SUCCESS;
}

int tfs_unmount(void){
	int i;
	closeDisk(currentMounted);
	currentMounted = 0;

	//initialize open files to -1
	for ( i = 0; i < 256; i++) {
		open_files_table[i] = -1;
	}
	return SUCCESS;
}


fileDescriptor tfs_openFile(char *name){
	//search through root iNode to file name
	//if exists, update open file table
	//if not exists, create fd, do all this stuff

	int currentFD;
	int exists = searchINodesByName(name);
	if (!exists) {
		//create file
		currentFD = findOpenFD();
		if (currentFD >= 0) {
			open_files_table[currentFD] = 0;

			//find free block and make it an iNode
			int fb = getFreeBlock();
			if (updateRootINode(fb, currentFD) < 0) {
				return WRITE_ERROR;
			}

			if (createFileINode(fb, name) < 0) {
				return WRITE_ERROR;
			}
		}
		else {
			return OUT_OF_SPACE;
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
	return SUCCESS;
}

int tfs_writeFile(fileDescriptor FD, char *buffer, int size){
	//
	int fileINodeLoc;
	if ((fileINodeLoc = searchINodesByFD(FD)) < 0){
		return WRITE_ERROR;
	}
	struct iNode *file = malloc(BLOCKSIZE);
	if (readBlock(currentMounted, fileINodeLoc, file) < 0) {
		return READ_ERROR;
	}
	file->blockCode = INODE_CODE;
	file->magicNumber = MAGIC_NUMBER;
	file->fileByteSize = size;

	struct fileExt *dataBlock = calloc(BLOCKSIZE,1);
	dataBlock->blockCode = DATABLOCK_CODE;
	dataBlock->magicNumber = MAGIC_NUMBER;

	int iNodeArrayIndex = 0;
	int fb;

	while(size > 0){
		//find free block, remove from freeblocklist
		if ((fb = getFreeBlock()) < 0) {
			return OUT_OF_SPACE;
		}
		//write 254 bytes OR size, whichever is least to free block

		memcpy(dataBlock->data, buffer, size < 254 ? size : 254);
		//write data block to disk
		if (writeBlock(currentMounted, fb, dataBlock) < 0) {
			return WRITE_ERROR;
		}
		//add free block to file inode array
		file->dataBlockMap[iNodeArrayIndex] = fb;
		iNodeArrayIndex++;
		//decrement by 254. if size <= 0, no more data left, you're chill
		size -= 254;
	}
	//write file inode back to disk
	if (writeBlock(currentMounted, fileINodeLoc, file) < 0) {
		return WRITE_ERROR;
	}

	free(file);
	free(dataBlock);
	open_files_table[FD] = 0;
	return SUCCESS;
}

int tfs_deleteFile(fileDescriptor FD){
	//use FD to find fileINodeLoc (by searching through rootINode)
	int fileINodeLoc;
	if ((fileINodeLoc = searchINodesByFD(FD)) < 0){
		return WRITE_ERROR;
	}
	//for each datablock in fileiNode list:
	struct iNode *file = malloc(BLOCKSIZE);
	if (readBlock(currentMounted, fileINodeLoc, file) < 0) {
		return READ_ERROR;
	}
	int i;
	for (i = 0; i < 241 && file->dataBlockMap[i] != -1; i++) {
		if (returnToFree(file->dataBlockMap[i]) < 0){
			return WRITE_ERROR;
		}
	}
		//read fileInode into buffer
		//clear data block information (rewrite whole block)
		//add fileExt block(now freed) back to supernode freelist
	//rewrite whole fileInode to be free
	if (returnToFree(fileINodeLoc) < 0) {
		return WRITE_ERROR;
	}

	//add to freelist
	//update rootINode table 
	struct iNode *root = malloc(BLOCKSIZE);
	if (readBlock(currentMounted, ROOTINODE_OFFSET, root) < 0) {
		return READ_ERROR;
	}
	root->dataBlockMap[fileINodeLoc] = -1;
	if (writeBlock(currentMounted, ROOTINODE_OFFSET, root) < 0) {
		return WRITE_ERROR;
	}
	//update open_files_table
	open_files_table[FD] = -1;

	free(file);
	free(root);
	return SUCCESS;
}

int returnToFree(int diskOffset) {
	struct freeBlock *fb = malloc(BLOCKSIZE);
	fb->blockCode = FREEBLOCK_CODE;
	fb->magicNumber = MAGIC_NUMBER;
	int i;
	for (i = 0; i < 254; i++) {
		fb->emptyBytes[i] = -1;
	}
	if (writeBlock(currentMounted, diskOffset, fb) < 0) {
		return WRITE_ERROR;
	}

	struct superBlock *sb = malloc(BLOCKSIZE);
	if (readBlock(currentMounted, SUPERBLOCK_OFFSET, sb) < 0) {
		return READ_ERROR;
	}
	sb->freeBlockArray[diskOffset] = 1;
	if (writeBlock(currentMounted, SUPERBLOCK_OFFSET, sb) < 0) {
		return WRITE_ERROR;
	}
	free(fb);
	free(sb);
	return SUCCESS;
}

int tfs_readByte(fileDescriptor FD, char *buffer){
	int cursor_offset = open_files_table[FD];
	int iNode_location = searchINodesByFD(FD);

	struct iNode *file = malloc(BLOCKSIZE);
	if(readBlock(currentMounted, iNode_location, file) < 0){
		return READ_ERROR;
	}

	//get cursor offset byte out of data portion of file

	int desiredBlock = file->dataBlockMap[cursor_offset / 254];

	struct fileExt *datablock = malloc(BLOCKSIZE);
	if (readBlock(currentMounted, desiredBlock, datablock) < 0){
		return READ_ERROR;
	}

	*buffer = datablock->data[cursor_offset % 254];
	open_files_table[FD] = cursor_offset + 1;

	free(file);
	free(datablock);
	return SUCCESS;
}

int tfs_seek(fileDescriptor FD, int offset){
	open_files_table[FD] = offset;
	return SUCCESS;
}

//returns block location on disk
int searchINodesByFD(fileDescriptor FD){
	struct iNode *root = malloc(BLOCKSIZE);
	if (readBlock(currentMounted, ROOTINODE_OFFSET, root) < 0) {
		return READ_ERROR;
	}
	int i = 0;

	while(root->dataBlockMap[i] != FD && i < 241){
		i++;
	}
	free(root);
	if(i == 241){
		return FILE_NOT_OPEN;
	}
	else{
		return i;
	}
}

//returns 0 if not found, else return fd
int searchINodesByName(char *name) {
	struct iNode *root = malloc(BLOCKSIZE);
	struct iNode *file = malloc(BLOCKSIZE);

	readBlock(currentMounted, ROOTINODE_OFFSET, root);
	int i;
	for (i = 0; i < 241; i++) {
		if(root->dataBlockMap[i] != -1){
			//need to read in that file iNode, check its name
			if (readBlock(currentMounted, i, file) < 0) {
				return READ_ERROR;
			}
			//check name
			if(!strcmp(file->fileName, name)){
				return root->dataBlockMap[i];
			}
		}
	}
	free(root);
	free(file);
	//in this case 0 != SUCCESS
	return 0;
}

//finds free block, removes from freeBlockArray
int getFreeBlock() {
	struct superBlock *superBuf = malloc(BLOCKSIZE);
	if (readBlock(currentMounted, SUPERBLOCK_OFFSET, superBuf) < 0) {
		return READ_ERROR;
	}

	int i = 0;
	while (superBuf->freeBlockArray[i] != 1 && superBuf->freeBlockArray[i] != -1) {
		i++;
	}

	if (superBuf->freeBlockArray[i] == -1) {
		return OUT_OF_SPACE;//out of data error
	}
	else {
		superBuf->freeBlockArray[i] = 0; //now it's going to be used
		if (writeBlock(currentMounted, SUPERBLOCK_OFFSET, superBuf) < 0) {
			return WRITE_ERROR;
		}
		return i;
	}

}

int updateRootINode(int freeBlock, int currentFD) {
	struct iNode *ibuf = malloc(BLOCKSIZE);
	if (readBlock(currentMounted, ROOTINODE_OFFSET, ibuf) < 0) {
		return READ_ERROR;
	}
	(ibuf->dataBlockMap)[freeBlock] = currentFD;
	int writestat = writeBlock(currentMounted, ROOTINODE_OFFSET, ibuf);
	if (writestat < 0) {
		return WRITE_ERROR;
	}
	return SUCCESS;
}

int createFileINode(int fb, char *name) {
	struct iNode fileINode;
	fileINode.blockCode = INODE_CODE;
	fileINode.magicNumber = MAGIC_NUMBER;
	strcpy(fileINode.fileName, name);
	fileINode.fileByteSize = 0;
	int i;
	for (i = 0; i < 241; i++) {
		fileINode.dataBlockMap[i] = -1;
	}

	if (writeBlock(currentMounted, fb, &fileINode) < 0) {
		return WRITE_ERROR;
	}
	return SUCCESS;
}

int findOpenFD(){
	int i = 0;

	//index to first open file descriptor
	while(open_files_table[i] != -1){
		i++;
	}
	return i;
}




