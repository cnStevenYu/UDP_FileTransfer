#include "packet.h"
#include "public.h"
/*convert filesize to network byte order*/
void setIntToNetChar(uint32_t filesize, char *dataToSend){
	uint32_t fileszn = htonl(filesize);
	for(int i = 0; i < FILESIZE; i++){
		dataToSend[i] = *((BYTE *)(&fileszn)+i);
	}
}
uint32_t getIntFromNetChar(char *dataRecvd){
	int i;
	uint32_t filesize;
	for(i = 0; i<FILESIZE; i++)
		((BYTE *)&filesize)[i] = dataRecvd[i];
	return ntohl(filesize);
}


Packet* pack(File *file, SEND_TYPE type, int blkNum, Args* args){
	Packet *pk;
	if((pk = (Packet*)malloc(sizeof(Packet))) == NULL) 
		return NULL;
	int pos = 0;
	int filenameLen;

	switch(type){
		case S_FILE:
			filenameLen	= strlen(file->name);
			pk->len = TYPE+filenameLen+ZEROBYTE+FILESIZE+ZEROBYTE;
			pk->data = (char *)malloc(pk->len);
			pos = 0;
			pk->data[pos++] = 0x1;
			for(int i = 0; i < filenameLen; )
				pk->data[pos++] = file->name[i++];
			pk->data[pos++] = 0x0;	
			/*convert filesize to network byte order*/
			setIntToNetChar(file->size, &pk->data[pos]);
			pos += FILESIZE;
			pk->data[pos++] = 0x0;
			break;
		case S_DATA:
			pk->len = TYPE+BLOCKNUM+BLOCK;	
			pk->data = (BYTE*)malloc(pk->len);
			pk->data[0] = 0x02;
			setIntToNetChar(blkNum, &pk->data[1]);
			/*fetch data from file*/
			if(file->fp == NULL) {//not open
				printf("open file :%s\n", file->path);
				if((file->fp = fopen(file->path, "rb")) == NULL)
					perror("file open failed!\n");
			}
			fseek(file->fp, blkNum*BLOCK, SEEK_SET);
			ssize_t nread = fread(&pk->data[TYPE+BLOCKNUM], sizeof(BYTE), BLOCK, file->fp);
			break;
		case S_ACK:
			pk->len = TYPE+BLOCKNUM;
			pk->data = (BYTE *)malloc(pk->len);
			pk->data[0] = 0x04;
			setIntToNetChar(blkNum, &pk->data[1]);
			break;
		case S_MSG:
			pk->len = TYPE + strlen(args->data.msg) + ZEROBYTE;
			pk->data = (BYTE*)malloc(pk->len);
			pk->data[0] = 0x08;
			pos = 1;
			for(int i=0; i<strlen(args->data.msg); ){
				pk->data[pos++] = args->data.msg[i++];
			}
			pk->data[pos] = '\0';
			break;
		case S_BCAST_REQ:
			pk->len = TYPE;
			pk->data = (BYTE*)malloc(pk->len);
			pk->data[0] = 0x10;
			break;
		case S_BCAST_ACK:
			pk->len = TYPE + IP_LEN;
			pk->data = (BYTE*)malloc(pk->len);
			pk->data[0] = 0x12;
			if(getLocalIp(&(pk->data[1])) < 0)
				perror("get local ip error!\n");
			pk->data[pk->len - 1] = '\0';
	}
	return pk;
}

void freePacket(Packet *pk) 
{
	if(pk->data){
		free(pk->data);
	}
	if(pk){
		free(pk);
	}
}