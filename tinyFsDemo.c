#include "libTinyFS.h"
#include "libDisk.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "TinyFS_errno.h"

int main(){

	printf("Allie Lustig | Maci Miri\n");
	printf("CPE 453 | Spring 2016\n");
	printf("-------------------------\n");
	printf("Begin TinyFsDemo\n\n");

	char demofilename[] = "DemoDisk.bin";
	int demosize = 51200;

	printf("Creating a new Disk file.\n");
	printf("Name is %s, size is %d\n", demofilename, demosize);
	int demoFD = tfs_mkfs(demofilename, 51200);
	printf("%s created at file descriptor %d\n\n", demofilename, demoFD);

	printf("Now mounting %s and verifying.\n", demofilename);
	int mntStat = tfs_mount(demofilename);
	printf("Mounted successfully? %s\n", !mntStat ? "SUCCESS" : "FAIL");
	if (!mntStat) {
		printf("All blocks were checked for correct Magic Number: 0x%X\n", MAGIC_NUMBER);
	}
	else {
		return FAILURE;
	}

	char demofile1[] = "file1";
	printf("\nNow opening a new file %s on disk.\n", demofile1);
	fileDescriptor fd1 = tfs_openFile(demofile1);
	printf("%s opened successfully? %s\n",demofile1, !fd1 ? "SUCCESS" : "FAIL");
	if (!fd1) {
		printf("iNode for %s created and logged by superblock and root iNode\n", demofile1);
	}
	else {
		return FAILURE;
	}

	printf("\nCreate buffer for file contents to be written to %s\n", demofile1);
	printf("Buffer hold incremental binary count of 0 to 255\n");
	char *filebuf = malloc(BLOCKSIZE);
	int i;
	for (i = 0; i < BLOCKSIZE; i++) {
		filebuf[i] = i;
	}
	printf("Writing buffer to %s\n", demofile1);
	int wrtStat = tfs_writeFile(fd1, filebuf, BLOCKSIZE);
	printf("Written successfully? %s\n", !wrtStat ? "SUCCESS" : "FAILURE");
	if (wrtStat) {
		return FAILURE;
	}

	printf("\nSeeking file cursor to 37th byte in %s\n", demofile1);
	int skStat = tfs_seek(fd1, 37);
	printf("Read the 37th byte from file.\n");
	char out;
	int rdBStat = tfs_readByte(fd1, &out);
	rdBStat += 0;
	skStat += 0;
	printf("Expected 37th byte: 0x25 (hex) '%%' (ascii)\n");
	printf("Actual   37th byte: 0x%X (hex) '%c' (ascii)\n", out, out);

	printf("\nClosing %s\n", demofile1);
	int clsStat = tfs_closeFile(fd1);
	printf("File closed successfully? %s\n", !clsStat ? "SUCCESS" : "FAILURE");
	if (clsStat) {
		return FAILURE;
	}

	printf("\n-------------------------\n\n");

	char demofile2[] = "file2";
	printf("\nNow opening a new file %s on disk.\n", demofile2);
	fileDescriptor fd2 = tfs_openFile(demofile2);
	printf("%s opened successfully? %s\n",demofile2, !fd2 ? "SUCCESS" : "FAIL");
	if (!fd2) {
		printf("iNode for %s created and logged by superblock and root iNode\n", demofile2);
	}
	else {
		return FAILURE;
	}

	printf("\nCreate buffer for file contents to be written to %s\n", demofile2);
	printf("Buffer holds 312 bytes of '*'.\nChosen to demonstrate multiple data blocks per file.\n");
	char *filebuf2 = malloc(312);
	for (i = 0; i < 312; i++) {
		filebuf2[i] = '*';
	}
	printf("Writing buffer to %s\n", demofile2);
	int wrtStat2 = tfs_writeFile(fd2, filebuf2, 312);
	printf("Written successfully? %s\n", !wrtStat2 ? "SUCCESS" : "FAILURE");
	if (wrtStat2) {
		return FAILURE;
	}

	printf("\nSeeking file cursor to 312th byte in %s\n", demofile2);
	int skStat2 = tfs_seek(fd2, 311);
	printf("Read the 312th byte from file.\n");
	char out2;
	int rdBStat2 = tfs_readByte(fd2, &out2);
	rdBStat2 += 0;
	skStat2 += 0;
	printf("Expected 312th byte: 0x2A (hex) '*' (ascii)\n");
	printf("Actual   312th byte: 0x%X (hex) '%c' (ascii)\n", out2, out2);

	printf("\nClosing %s\n", demofile2);
	int clsStat2 = tfs_closeFile(fd2);
	printf("File closed successfully? %s\n", !clsStat2 ? "SUCCESS" : "FAILURE");
	if (clsStat2) {
		return FAILURE;
	}

	printf("\n-------------------------\n\n");

	printf("Deleting %s\n", demofile1);
	int delStat = tfs_deleteFile(fd1);
	printf("Deleted %s successfully? %s\n", demofile1, !delStat ? "SUCCESS" : "FAILURE");
	if (delStat) {
		return FAILURE;
	}
	printf("Show %s was deleted and %s remains by tfs_readdir()\n", demofile1, demofile2);
	int lsStat = tfs_readdir();
	if (lsStat) {
		return FAILURE;
	}

	printf("\nRename %s to THELASTFILESTANDING", demofile2);
	int rnStat = tfs_rename(demofile2, "THELASTFILESTANDING");
	printf("Renamed successfully? %s\n", !rnStat ? "SUCCESS" : "FAILURE");
	tfs_readdir();

	printf("\nUnmounting %s\n", demofilename);
	int umntStat = tfs_unmount();
	printf("Unmounted successfully? %s\n", !umntStat ? "SUCCESS" : "FAILURE");
	if(umntStat) {
		return FAILURE;
	}

	printf("\nTHANKS FOR COMING TO OUR DEMO COME AGAIN\n");
	return 0;
}


