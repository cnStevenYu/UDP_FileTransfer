#pragma once
#include "public.h"
int getLocalIp(char *ipaddr);
int getBroadcastAddr(int sockfd, struct sockaddr_in *broadcast_addr);