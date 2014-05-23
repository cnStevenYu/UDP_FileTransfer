#pragma once
#include "public.h"
/*file to be sent or recvd*/
typedef struct File{
	FILE *fp ;
	char name[FILE_NAME];
	char *path;
	uint32_t size;
	uint32_t blkSum;
	/*for the sender it's the block to be sent;
	 * for the receiver it's the block to be recvd*/
	uint32_t curBlk;//for sender it's the block to be sent;
	uint32_t preBlk;
}File;

/*fetch filename from fielpath to filename, return false if filename it too long*/
BOOL fetchFilenameFromPath(char *filepath, char *filename, int nameLen);
/*fetch the size of file, 
 * return size if file is openned successfully
 * or -1 when failed */
uint32_t getFilesize(char *filepath);
/*count the number of blocks */
uint32_t countBlocks(uint32_t filesize, uint32_t block);