#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <signal.h>
#include <sys/time.h>
#include "wrap.h"

#define BUF_SIZE 512
#define FILE_PATH 256
#define FILE_NAME 128
#define RECV_PORT 8081
#define IP_LEN 16
#define MAX_MSG_LEN (BUF_SIZE - IP_LEN - 24)
#define BLOCK 512
#define ZEROBYTE 1
#define BLOCKNUM 4
#define FILESIZE 4
#define TYPE 1

#define INIT_INTERVAL 3
#define MAX_REPEAT 3
#define INC_INTERVAL 1

typedef unsigned char BYTE;
typedef enum BOOL {FALSE=0, TRUE=1} BOOL;
typedef enum ERR {OK, ARGS_FORMAT_ERR, IP_FORMAT_ERR} ERR;
/*args type*/
enum ARGTYPE{SEND_MSG, SEND_FILE, LIST_FRIENDS, QUIT} ;
/*The type of data to be sent*/
typedef enum {S_FILE, S_ACK, S_DATA, S_MSG}SEND_TYPE;
/*arguments*/
typedef struct Args{
	enum ARGTYPE type;
	char remoteIp[IP_LEN];
	//char data[BUF_SIZE];
	union {
		char msg[BUF_SIZE];
		char filepath[FILE_PATH];
	}data;

}Args;
/*data to be sent*/
typedef struct {
	BYTE* data;
	int len;
}Packet;
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
typedef struct Task{
	enum {WAIT_ACK, WAIT_DATA} stat;
	File fileToSend;
	File fileToRecv;
}Task;

int sockfd;
/*local ip address*/
struct sockaddr_in localAddr;
/*remote ip address*/
struct sockaddr_in remoteAddr;
socklen_t remoteAddrLen;

File fileToSend;
File fileToRecv;

int curInterval = INIT_INTERVAL;
int repeat = 0;
/*recv function*/
void* recvProc(void *args);

ERR argsProc(Args* args, char *comLine, int len);

BOOL fetchFilenameFromPath(char *filepath, char *filename, int nameLen);

BOOL verifyIP(char *src, char *dst, int len);

/*fetch the size of file, 
 * return size if file is openned successfully
 * or -1 when failed */
uint32_t getFilesize(char *filepath);
/*convert little-end uint to big-end uint and set it to dataToSend*/
void setIntToNetChar(uint32_t filesize, char *dataToSend);
uint32_t getIntFromNetChar(char *dataRecvd);
/*count the number of blocks */
uint32_t countBlocks(uint32_t filesize, uint32_t block);
/*set sockaddr_in*/
void setRemoteIP(struct sockaddr_in *remoteAddr, char *remoteIp, int port);

Packet* pack(File *file, SEND_TYPE type, int blkNum, Args *args);	
