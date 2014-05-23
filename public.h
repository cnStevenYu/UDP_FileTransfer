#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <linux/if.h>

#define BUF_SIZE 512
#define FILE_PATH 256
#define FILE_NAME 128
#define RECV_PORT 8081
#define IP_LEN 16
#define MAX_MSG_LEN (BUF_SIZE - IP_LEN - 24)

typedef unsigned char BYTE;
typedef enum BOOL {FALSE=0, TRUE=1} BOOL;