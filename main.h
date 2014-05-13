#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include "wrap.h"

#define BUF_SIZE 512
#define FILE_PATH 256
#define FILE_NAME 128
#define RECV_PORT 8081
#define IP_LEN 16
#define MAX_MSG_LEN (BUF_SIZE - IP_LEN - 24)
/*local ip address*/
struct sockaddr_in localAddr;
/*remote ip address*/
struct sockaddr_in remoteAddr;
socklen_t remoteAddrLen;

int sockfd;
/*recv function*/
void* recvProc(void *args);

typedef enum BOOL {FALSE=0, TRUE=1} BOOL;
enum MSGTYPE {FILE_RECV, DATA_RECV, MSG_RECV};
enum ARGTYPE{SEND_MSG, SEND_FILE, LIST_FRIENDS, QUIT} ;
enum ERR {OK, ARGS_FORMAT_ERR, IP_FORMAT_ERR};
typedef struct Args{
	enum ARGTYPE type;
	char remoteIp[IP_LEN];
	//char data[BUF_SIZE];
	union {
		char msg[BUF_SIZE];
		char filepath[FILE_PATH];
	}data;

}Args;

typedef enum ERR ERR;

ERR argsProc(Args* args, char *comLine, int len);
typedef struct File{
	FILE *fp ;
	char name[FILE_NAME];
	char *path;
	uint32_t size;
	uint32_t blkSum;
}File;
BOOL fetchFilenameFromPath(char *filepath, char *filename, int nameLen);

BOOL verifyIP(char *src, char *dst, int len);

/*fetch the size of file, 
 * return size if file is openned successfully
 * or -1 when failed */
uint32_t getFilesize(char *filepath);
/*convert little-end uint to big-end uint and set it to dataToSend*/
void setIntToNetChar(uint32_t filesize, char *dataToSend);
uint32_t getIntFromNetChar(char *dataRecvd);
uint32_t countBlocks(uint32_t filesize, uint32_t block);
/*set sockaddr_in*/
void setRemoteIP(struct sockaddr_in *remoteAddr, char *remoteIp, int port);
File fileToSend;
