#define _XOPEN_SOURCE
#include "main.h"
int main(int argc, char *argv[]){
	/*init friends list*/
	//initFriendList(&friendList);

	/*timer*/
	curInterval = INIT_INTERVAL;
	repeat = 0;
	fileToSend.fp = NULL;
	//socket
	sockfd = Socket(AF_INET, SOCK_DGRAM, 0);
	//set localAddr
	memset(&localAddr, 0, sizeof(localAddr));
	localAddr.sin_family = AF_INET;
	localAddr.sin_port = htons(RECV_PORT);
	localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	//bind
	Bind(sockfd, (struct sockaddr *)&localAddr, sizeof(localAddr));

	pthread_t recvThr;
	recvThr = pthread_create(&recvThr, NULL, &recvProc, NULL);
	/*arguments*/
	Args args;
	char comLine[BUF_SIZE];
	int pos = 0;
	int len = 0;
	while(fgets(comLine, BUF_SIZE, stdin) != NULL){
		memset(&args, 0, sizeof(args));
		ssize_t n = 0;
		Packet *pk;
		int so_broadcast = 1;  
		char buffer[1024];
		struct ifreq *ifr;  
		struct ifconf ifc;  
		struct sockaddr_in broadcast_addr; //广播地址
		switch(argsProc(&args, comLine, BUF_SIZE)){
			case OK:
				switch(args.type){
					case SEND_MSG:
						pk = pack(NULL, S_MSG, -1, &args);
						//set remoteAddr
						setRemoteIP(&remoteAddr, args.remoteIp, RECV_PORT);
						n = sendto(sockfd, pk->data, pk->len, 0, (struct sockaddr*)&remoteAddr, sizeof(remoteAddr));
						if(n == -1){
							free(pk->data);
							free(pk);
							perr_exit("sendto error\n");
						}
						free(pk->data);
						free(pk);
						continue;
					case SEND_FILE:
						printf("send file\n");
						memset(&fileToSend, 0, sizeof(File));
						fileToSend.path = args.data.filepath;
						/*fetch filename from the args*/
						if(!fetchFilenameFromPath(fileToSend.path, fileToSend.name, FILE_NAME)){
							printf("filepath is too long\n"); 
							continue;
						}
						printf("filename:%s\n", fileToSend.name);
						/*fetch file length*/	
						fileToSend.size = getFilesize(fileToSend.path);
						/*count the sum of blocks*/
						fileToSend.blkSum = countBlocks(fileToSend.size, BLOCK);
						fileToSend.curBlk = 0;	
						/*data to send*/
						setRemoteIP(&remoteAddr, args.remoteIp, RECV_PORT);
						

						pk = pack(&fileToSend, S_FILE, -1, NULL);
						/*send data*/
						n = sendto(sockfd, pk->data, pk->len, 0, (struct sockaddr*)&remoteAddr, sizeof(remoteAddr));
						if(n == -1){
							perror("Error:send to failed!\n");
						}
						if(pk->data)
							free(pk->data);
						if(!pk)
							free(pk);
						continue;
					case LIST_FRIENDS:

						// 获取所有套接字接口  
						ifc.ifc_len = sizeof(buffer);  
						ifc.ifc_buf = buffer;  
						if (ioctl(sockfd, SIOCGIFCONF, (char *) &ifc) < 0)  
						{  
							perror("ioctl-conf:");  
							return -1;  
						}  
						ifr = ifc.ifc_req;  
						for (int j = ifc.ifc_len / sizeof(struct ifreq); --j >= 0; ifr++)  
						{  
							if (!strcmp(ifr->ifr_name, "eth0"))  
							{  
								if (ioctl(sockfd, SIOCGIFFLAGS, (char *) ifr) < 0)  
								{  
									perror("ioctl-get flag failed:");  
								}  
								break;  
							}  
						}  

						//将使用的网络接口名字复制到ifr.ifr_name中，由于不同的网卡接口的广播地址是不一样的，因此指定网卡接口  
						//strncpy(ifr.ifr_name, IFNAME, strlen(IFNAME));  
						//发送命令，获得网络接口的广播地址  
						if (ioctl(sockfd, SIOCGIFBRDADDR, ifr) == -1)  
						{  
							perror("ioctl error");  
							return -1;  
						}  
						//将获得的广播地址复制到broadcast_addr  
						memcpy(&broadcast_addr, (char *)&ifr->ifr_broadaddr, sizeof(struct sockaddr_in));  
						//设置广播端口号  
						printf("\nBroadcast-IP: %s\n", inet_ntoa(broadcast_addr.sin_addr));  
						broadcast_addr.sin_family = AF_INET;  
						broadcast_addr.sin_port = htons(RECV_PORT);  

						//默认的套接字描述符sock是不支持广播，必须设置套接字描述符以支持广播  
						n = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &so_broadcast,  
								sizeof(so_broadcast));  

						pk = pack(NULL, S_BCAST_REQ, -1, NULL);

						n = sendto(sockfd, pk->data, pk->len, 0, (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr));
						if(n == -1){
							perror("Error:send to failed!\n");
						}
						if(pk->data)
							free(pk->data);
						if(!pk)
							free(pk);

						continue;
					case QUIT:
						printf("quit!\n");
						exit(0);
				}
				continue;
			case ARGS_FORMAT_ERR:
				printf("args format error!\n");
				continue;
			case IP_FORMAT_ERR:
				printf("ip format error!\n");
				continue;
			default: continue;
		}
	}
}

void* recvProc(void *args){ 
	printf("Accepting connections ...\n");
	registerTimer(retransmit);

	ssize_t n = 0;
	BYTE dataRecvd[TYPE+BLOCKNUM+BLOCK];
	char ipBuf[IP_LEN];
	//File fileToRecv;
	while(1){
		int pos = 0;
		remoteAddrLen = sizeof(remoteAddr);
		n = recvfrom(sockfd, dataRecvd,TYPE+BLOCKNUM+BLOCK , 0, (struct sockaddr*) &remoteAddr, &remoteAddrLen);
		if (n == -1 || errno == EINTR){
			if(errno == EINTR) {
				perror("interrupted!\n");
			}
			if(n == -1)
				perror("error!\n");
		}
		/*parse the data*/
		//		parse(&fileToRecv, dataRecvd, TYPE+BLOCKNUM+BLOCK);
		if(dataRecvd[pos] == 0x01){
			printf("received from %s at %d packet is 01\n", inet_ntop(AF_INET, &remoteAddr.sin_addr, ipBuf, sizeof(ipBuf)), ntohs(remoteAddr.sin_port));
			/*fetch file name*/
			memset(&fileToRecv, 0, sizeof(File));
			pos++;
			int i = 0;
			while(dataRecvd[pos] != '\0')
				fileToRecv.name[i++] = dataRecvd[pos++];
			pos++;
			fileToRecv.size = getIntFromNetChar(&dataRecvd[pos]);
			fileToRecv.blkSum = countBlocks(fileToRecv.size, BLOCK);
			/*file path*/
			char dstDir [19] = "/home/yh/projects/";
			int filepathLen = strlen(dstDir) + strlen(fileToRecv.name) + 1;	
			fileToRecv.path = (char*)malloc(filepathLen);
			memset(fileToRecv.path, 0, filepathLen); 
			strncat(fileToRecv.path, dstDir, strlen(dstDir));
			strncat(fileToRecv.path, fileToRecv.name, strlen(fileToRecv.name));
			/*open the file*/
			if((fileToRecv.fp = fopen(fileToRecv.path, "wb+")) == NULL){
				perror("Error:open file failed!\n");
			}
			free(fileToRecv.path);
			/*create ack pack*/
			fileToRecv.preBlk = -1;
			fileToRecv.curBlk = 0;
			Packet *pk = pack(NULL, S_ACK, fileToRecv.curBlk, NULL);
			n = sendto(sockfd, pk->data, pk->len, 0, (struct sockaddr*)&remoteAddr, sizeof(remoteAddr));
			if (n == -1){
				perror("Error:sendto failed!\n");
			}
			free(pk->data);
			free(pk);
#ifdef _TIMER
			/*start timer*/
			setTimer(curInterval, &orignalTime);
#endif
		} 
		else{
			if(dataRecvd[pos] == 0x02){
				printf("received from %s at %d packet is 02\n", inet_ntop(AF_INET, &remoteAddr.sin_addr, ipBuf, sizeof(ipBuf)), ntohs(remoteAddr.sin_port));
				repeat = 0;//reset repeat
				uint32_t blkNum = getIntFromNetChar(&dataRecvd[1]);
				fileToRecv.curBlk = blkNum + 1;
				BYTE *dataBlock;
				ssize_t nwrite;
				/*write to file*/
				fseek(fileToRecv.fp, blkNum*BLOCK, SEEK_SET);
				if(blkNum < fileToRecv.blkSum - 1){
					dataBlock = (BYTE *)malloc(BLOCK);
					for(int i=0; i < BLOCK;i++){
						dataBlock[i] = dataRecvd[TYPE+BLOCKNUM+i];
					}	
					nwrite = fwrite(dataBlock, sizeof(BYTE), BLOCK, fileToRecv.fp);
					/*request next block*/
					fileToRecv.preBlk = fileToRecv.curBlk;
					Packet *pk = pack(NULL, S_ACK, fileToRecv.curBlk, NULL);
					n = sendto(sockfd, pk->data, pk->len, 0, (struct sockaddr*)&remoteAddr, sizeof(remoteAddr));
					if (n == -1){
						perror("Error:sendto failed!\n");
					}
					free(pk->data);
					free(pk);
					/*reset timer*/
#ifdef _TIMER
					setTimer(curInterval, NULL);
#endif
				}	
				else{
					int r = fileToRecv.size % BLOCK;
					int dataLen = r > 0 ? r :  BLOCK;
					dataBlock = (BYTE *)malloc(dataLen);
					for(int i=0; i < dataLen;i++){
						dataBlock[i] = dataRecvd[TYPE+BLOCKNUM+i];
					}	
					nwrite = fwrite(dataBlock, sizeof(BYTE), dataLen, fileToRecv.fp);
					fclose(fileToRecv.fp);
					fileToRecv.fp = NULL;
				}
				free(dataBlock);	
			}
			else{
				if(dataRecvd[pos] == 0x04){
					printf("received from %s at %d packet is 04\n", inet_ntop(AF_INET, &remoteAddr.sin_addr, ipBuf, sizeof(ipBuf)), ntohs(remoteAddr.sin_port));
					//sleep(14);
					/*recv ack*/
					uint32_t reqNum = getIntFromNetChar(&dataRecvd[1]);
					if(fileToSend.curBlk == reqNum){
						if(reqNum < fileToSend.blkSum) {
							/*send data*/
							Packet *pk = pack(&fileToSend, S_DATA, reqNum, NULL);
							n = sendto(sockfd, pk->data, pk->len, 0, (struct sockaddr*)&remoteAddr, sizeof(remoteAddr));
							if(n == -1){
								perror("Error:sendto failed!\n");
							}
							free(pk->data);
							free(pk);

						}
						else{
							/*the receiver has received the last pack*/
							fclose(fileToSend.fp);
							fileToSend.fp = NULL;
						}
						fileToSend.curBlk = reqNum + 1;
					}
					else{
						printf("drop duplicate packet\n");
					}

				}
				else{
					if(dataRecvd[pos] == 0x08){
						printf("received from %s at %d packet is 08\n", inet_ntop(AF_INET, &remoteAddr.sin_addr, ipBuf, sizeof(ipBuf)), ntohs(remoteAddr.sin_port));
						/*msg*/
						printf("msg:%s\n", &dataRecvd[1]);
					}
					else{
						if(dataRecvd[0] == 0x10){
							printf("received from %s at %d packet is 10\n", inet_ntop(AF_INET, &remoteAddr.sin_addr, ipBuf, sizeof(ipBuf)), ntohs(remoteAddr.sin_port));
							//create broadcast ack pack
							Packet *pk = pack(NULL, S_BCAST_ACK, -1, NULL);
							n = sendto(sockfd, pk->data, pk->len, 0, (struct sockaddr*)&remoteAddr, sizeof(remoteAddr));
							if(n == -1){
								perror("Error:sendto failed!\n");
							}
							free(pk->data);
							free(pk);
						}
						else{
							if(dataRecvd[0] == 0x12) {
								printf("received from %s at %d packet is 12\n", inet_ntop(AF_INET, &remoteAddr.sin_addr, ipBuf, sizeof(ipBuf)), ntohs(remoteAddr.sin_port));
								pos = 1;
								if(friendList.curPos < 255) {
									for(int i=0; i<IP_LEN && dataRecvd[pos] != '\0'; i++, pos++){
										friendList.data[friendList.curPos][i] = dataRecvd[pos]; 
									}
									friendList.curPos++;
								}
								/*print ip of friends*/
								for(int i=0; i<friendList.curPos; i++) {
									printf("$%d:%s\n", i,friendList.data[i]); 
								}
							}
						}
					}
				}
			}
		}	
	}
}

void setRemoteIP(struct sockaddr_in *remoteAddr, char *remoteIp, int port){
	memset(remoteAddr, 0, sizeof(struct sockaddr_in));
	remoteAddr.sin_family = AF_INET;
	remoteAddr.sin_port = htons(port);
	remoteAddr.sin_addr.s_addr = inet_addr(remoteIp);
}
void retransmit(int signo)
{
	repeat++;
	printf("retransmit ack %d\n", fileToRecv.curBlk);
	/*retransmit ack pack*/
	if(repeat <= MAX_REPEAT){//continuous repeat should not more than max
		if(fileToRecv.curBlk < fileToRecv.blkSum){	
			Packet *pk = pack(NULL, S_ACK, fileToRecv.curBlk, NULL);
			int n = sendto(sockfd, pk->data, pk->len, 0, (struct sockaddr*)&remoteAddr, sizeof(remoteAddr));
			if (n == -1){
				perror("Error:sendto failed!\n");
			}
			free(pk->data);
			free(pk);
			/*inc timer*/
			curInterval += INC_INTERVAL;
			setTimer(curInterval, NULL);
		}
		else{
			curInterval = INIT_INTERVAL;
			printf("recvd!\n");
		}
	}
	else{
		printf("network error!\n");
	}
}

/*
void initFriendList(FriendList *list, int length);
{
	list->len = length;
	list->data = (char**)malloc(list->len * sizeof(char*));
	for(int i=0; i<list->len; i++){
		list.data[i] = (char*)malloc(IP_LEN);
	}
	list->curPos = 0;
}
*/
