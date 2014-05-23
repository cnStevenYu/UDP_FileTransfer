#include "file.h"
uint32_t countBlocks(uint32_t filesize, uint32_t block){
	uint32_t r = filesize % (block+1);
	return (filesize - r) / block + 1;
}
uint32_t getFilesize(char *filepath){
	FILE *fp;
	uint32_t size;
	if((fp = fopen(filepath, "rb")) == NULL)
		perror("file open failed!\n");	
	if(fseek(fp, 0L, SEEK_END) < 0)
		perror("file seek failed!\n");
	size = ftell(fp);	
	if(fclose(fp) < 0)
		perror("file close failed!\n");
	return size;
}

BOOL fetchFilenameFromPath(char *filepath, char *filename, int nameLen){
	memset(filename, 0, nameLen);
	int pathLen = strlen(filepath);
	int pos = pathLen - 1;	
	while(filepath[pos] != '/' && pos != 0)
		pos--;
	pos++;
	/*if the length of filename if more than nameLen, return false*/
	if(pathLen - pos >= nameLen) 
		return FALSE;		
	int filenameLen = pathLen - pos; 
	for(int i = 0;i < filenameLen; ){
		filename[i++] = filepath[pos++];
	}
	return TRUE;
}
