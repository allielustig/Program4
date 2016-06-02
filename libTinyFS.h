#ifndef TINYFSH
#define TINYFSH

#define BLOCKSIZE 256
#define DEFAULT_DISK_SIZE 10240
#define DEFAULT_DISK_NAME "tinyFSDisk"
#define MAX_FILE_SYSTEMS 100
#define MAGIC_NUMBER 0x45
#define SUPERBLOCK_CODE 1
#define INODE_CODE 2
#define DATABLOCK_CODE 3
#define FREEBLOCK_CODE 4

typedef int fileDescriptor;

struct __attribute__((__packed__)) superBlock {
	char blockCode;
	char magicNumber;
	char rootInodeOffset;
	char freeBlockArray[253];
};

struct __attribute__((__packed__)) iNode{
	char blockCode;
	char magicNumber;
	char fileName[9];
	int fileByteSize; //fileSize in Bytes
	char dataBlockMap[241]; //index is a data block. maps to disk offset.
	//root node: index i = disk block i. dataBlockMap[i]=fileDescriptor of corresponding file iNode -Allie Lustig -Abraham Lincoln -Michael Scott
};

struct __attribute__((__packed__)) fileExt{
	char blockCode;
	char magicNumber;
	char data[254];
};

struct __attribute__((__packed__)) freeBlock{
	char blockCode;
	char magicNumber;
	char emptyBytes[254];
};

/* Makes a blank TinyFS file system of size nBytes on the file specified by ‘filename’. This function should use the 
emulated disk library to open the specified file, and upon success, format the file to be mountable. This includes 
initializing all data to 0x00, setting magic numbers, initializing and writing the superblock and inodes, etc. Must 
return a specified success/error code. */
int tfs_mkfs(char *filename, int nBytes);

/* tfs_mount(char *filename) “mounts” a TinyFS file system located within ‘filename’. tfs_unmount(void) “unmounts” 
the currently mounted file system. As part of the mount operation, tfs_mount should verify the file system is the 
correct type. Only one file system may be mounted at a time. Use tfs_unmount to cleanly unmount the currently mounted 
file system. Must return a specified success/error code. */
int tfs_mount(char *filename);
int tfs_unmount(void);

/* Opens a file for reading and writing on the currently mounted file system. Creates a dynamic resource table entry 
for the file, and returns a file descriptor (integer) that can be used to reference this file while the filesystem is mounted. */
fileDescriptor tfs_openFile(char *name);

/* Closes the file, de-allocates all system/disk resources, and removes table entry */
int tfs_closeFile(fileDescriptor FD);

/* Writes buffer ‘buffer’ of size ‘size’, which represents an entire file’s content, to the file system. Sets the 
file pointer to 0 (the start of file) when done. Returns success/error codes. */
int tfs_writeFile(fileDescriptor FD, char *buffer, int size);

/* deletes a file and marks its blocks as free on disk. */
int tfs_deleteFile(fileDescriptor FD);

/* reads one byte from the file and copies it to buffer, using the current file pointer location and incrementing it by
one upon success. If the file pointer is already at the end of the file then tfs_readByte() should return an error and 
not increment the file pointer. */
int tfs_readByte(fileDescriptor FD, char *buffer);

/* change the file pointer location to offset (absolute). Returns success/error codes.*/
int tfs_seek(fileDescriptor FD, int offset);

int findOpenFD();

int getFreeBlock();

int searchINodesByName(char *name);

int updateRootINode();

int createFileINode(int fb, char *name);

int searchINodesByFD(fileDescriptor FD);

int returnToFree(int diskOffset);

int checkFSType();

int tfs_readdir();

int tfs_rename(char *oldname, char *newname);

#endif











