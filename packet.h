#pragma once
#include "public.h"
#include "args.h"
#include "file.h"

#define BLOCK 512
#define ZEROBYTE 1
#define BLOCKNUM 4
#define FILESIZE 4
#define TYPE 1

/*The type of data to be sent*/
typedef enum {S_FILE, S_ACK, S_DATA, S_MSG, S_BCAST_REQ, S_BCAST_ACK}SEND_TYPE;

/*data to be sent*/
typedef struct {
	BYTE* data;
	int len;
}Packet;

/*convert little-end uint to big-end uint and set it to dataToSend*/
void setIntToNetChar(uint32_t filesize, char *dataToSend);
uint32_t getIntFromNetChar(char *dataRecvd);
Packet* pack(File *file, SEND_TYPE type, int blkNum, Args *args);
void freePacket(Packet *pk);	
