#pragma once
#include "public.h"
typedef enum ERR {OK, ARGS_FORMAT_ERR, IP_FORMAT_ERR} ERR;
/*args type*/
enum ARGTYPE{SEND_MSG, SEND_FILE, LIST_FRIENDS, QUIT} ;
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

ERR argsProc(Args* args, char *comLine, int len);

BOOL verifyIP(char *src, char *dst, int len);