#pragma once
#include "public.h"
#include "timer.h"
#include "args.h"
#include "packet.h"
#include "file.h"
#include "host.h"
#include "wrap.h"
#include <pthread.h>

typedef struct FriendList {
	/*the ip of friends in the LAN*/
	char data[255][IP_LEN];
	int curPos;
} FriendList;
typedef struct DataAndIp{
	struct sockaddr_in *remoteIp;
	BYTE * dataRecvd;
} DataAndIp;
int sockfd;
/*local ip address*/
//struct sockaddr_in localAddr;
/*remote ip address*/
//struct sockaddr_in remoteAddr;
//socklen_t remoteAddrLen;
/*file*/
//File fileToSend;
//File fileToRecv;

FriendList friendList;
/*set sockaddr_in*/
void setRemoteIP(struct sockaddr_in *remoteAddr, char *remoteIp, int port);
/*recv function*/
void* recvProc(void *args);
void *sendFileProc(void *args);
void *recvFileProc(void *args);
void retransmit(int signo);
