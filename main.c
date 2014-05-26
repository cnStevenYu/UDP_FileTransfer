#define _XOPEN_SOURCE
#define DEBUG
#include "main.h"

int main(int argc, char *argv[]){
	struct sockaddr_in localAddr;
	/*remote ip address*/
	struct sockaddr_in remoteAddr;
	socklen_t remoteAddrLen;
	/*init timer*/
	curInterval = INIT_INTERVAL;
	repeat = 0;
	//fileToSend.fp = NULL;
	/*socket*/
	sockfd = Socket(AF_INET, SOCK_DGRAM, 0);
	/*set localAddr*/
	memset(&localAddr, 0, sizeof(localAddr));
	localAddr.sin_family = AF_INET;
	localAddr.sin_port = htons(RECV_PORT);
	localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	/*bind*/
	Bind(sockfd, (struct sockaddr *)&localAddr, sizeof(localAddr));
	/*create recv thread*/
	pthread_t recvThr;
	recvThr = pthread_create(&recvThr, NULL, &recvProc, NULL);
	/*arguments*/
	Args args;

	char comLine[BUF_SIZE];
	while(fgets(comLine, BUF_SIZE, stdin) != NULL){
		memset(&args, 0, sizeof(Args));
		Packet *pk;
		pthread_t sendFileThr;
		switch(argsProc(&args, comLine, BUF_SIZE)){
			case OK:
				switch(args.type){
					case SEND_MSG:
						pk = pack(NULL, S_MSG, -1, &args);
						setRemoteIP(&remoteAddr, args.remoteIp, RECV_PORT);
						Sendto(sockfd, pk->data, pk->len, &remoteAddr, 
							   sizeof(struct sockaddr_in));
						freePacket(pk);
						continue;
					case SEND_FILE:
#ifdef DEBUG
						printf("send file\n");
#endif
						/*create recv thread*/
						sendFileThr = pthread_create(&sendFileThr, NULL, &sendFileProc, (void *)&args);	
						continue;
					case LIST_FRIENDS:

						if(getBroadcastAddr(sockfd, &remoteAddr) < 0)
							continue;
						//设置广播端口号  
#ifdef DEBUG
						printf("\nBroadcast-IP: %s\n", inet_ntoa(remoteAddr.sin_addr));  
#endif
						remoteAddr.sin_family = AF_INET;  
						remoteAddr.sin_port = htons(RECV_PORT);  
 
						pk = pack(NULL, S_BCAST_REQ, -1, NULL);
						Sendto(sockfd, pk->data, pk->len, &remoteAddr, 
							   sizeof(struct sockaddr_in));
						freePacket(pk);
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
#ifdef _TIMER
	registerTimer(retransmit);
#endif

	struct sockaddr_in *remoteAddr;
	socklen_t remoteAddrLen;
	BYTE *dataRecvd;
	char ipBuf[IP_LEN];
	//File fileToRecv;
	while(1){
		int pos = 0;
		remoteAddr = (struct sockaddr_in*)malloc(sizeof (struct sockaddr_in));
		remoteAddrLen = sizeof(struct sockaddr_in);
		dataRecvd = (BYTE*)malloc(sizeof(TYPE + BLOCKNUM + BLOCK));
		ssize_t n = recvfrom(sockfd, dataRecvd,TYPE+BLOCKNUM+BLOCK , 0, 
					 (struct sockaddr*) remoteAddr, &remoteAddrLen);
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
#ifdef DEBUG

			printf("received from %s at %d packet is 01\n", 
				    inet_ntop(AF_INET, &remoteAddr->sin_addr, 
				    ipBuf, sizeof(ipBuf)), ntohs(remoteAddr->sin_port));
#endif
			/*create new thread*/
			DataAndIp data_and_ip;
			data_and_ip.remoteIp = remoteAddr;
			data_and_ip.dataRecvd = dataRecvd;
			pthread_t recvFileThr;
			recvFileThr = pthread_create(&recvFileThr, NULL, &recvFileProc, (void *)&data_and_ip);
#ifdef _TIMER
			/*start timer*/
			setTimer(curInterval, &orignalTime);
#endif
		} 
		else{
			
			if(dataRecvd[0] == 0x08){
#ifdef DEBUG
				printf("received from %s at %d packet is 08\n", 
					 	inet_ntop(AF_INET, &remoteAddr->sin_addr, 
					 	ipBuf, sizeof(ipBuf)), ntohs(remoteAddr->sin_port));
#endif
				/*msg*/
				printf("msg:%s\n", &dataRecvd[1]);
					
			}
			else{
				if(dataRecvd[0] == 0x10){
#ifdef DEBUG
					printf("received from %s at %d packet is 10\n", 
							inet_ntop(AF_INET, &remoteAddr->sin_addr, 
							ipBuf, sizeof(ipBuf)), ntohs(remoteAddr->sin_port));
#endif
					//create broadcast ack pack
					Packet *pk = pack(NULL, S_BCAST_ACK, -1, NULL);
					Sendto(sockfd, pk->data, pk->len, remoteAddr, sizeof(struct sockaddr_in));
					freePacket(pk);
				}
				else{
					if(dataRecvd[0] == 0x12) {
#ifdef DEBUG
						printf("received from %s at %d packet is 12\n", 
								inet_ntop(AF_INET, &remoteAddr->sin_addr, 
								ipBuf, sizeof(ipBuf)), ntohs(remoteAddr->sin_port));
#endif
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
			free(dataRecvd);
			free(remoteAddr);	
		}	
	}
}

void setRemoteIP(struct sockaddr_in *remoteAddr, char *remoteIp, int port){
	memset(remoteAddr, 0, sizeof(struct sockaddr_in));
	remoteAddr->sin_family = AF_INET;
	remoteAddr->sin_port = htons(port);
	remoteAddr->sin_addr.s_addr = inet_addr(remoteIp);
}
#ifdef _TIMER
void retransmit(int signo)
{
	repeat++;
	//printf("retransmit ack %d\n", fileToRecv.curBlk);
	/*retransmit ack pack*/
	if(repeat <= MAX_REPEAT){//continuous repeat should not more than max
		if(fileToRecv.curBlk < fileToRecv.blkSum){	
			Packet *pk = pack(NULL, S_ACK, fileToRecv.curBlk, NULL);
			Sendto(sockfd, pk->data, pk->len, &remoteAddr, 
					sizeof(struct sockaddr_in));
			freePacket(pk);
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
#endif
void *sendFileProc(void *args)
{
	/*local ip address*/
	struct sockaddr_in localAddr;
	/*remote ip address*/
	struct sockaddr_in remoteAddr;
	socklen_t remoteAddrLen;

	/*socket*/
	int sendFileSock = Socket(AF_INET, SOCK_DGRAM, 0);
	/*set localAddr*/
	memset(&localAddr, 0, sizeof(localAddr));
	localAddr.sin_family = AF_INET;
	localAddr.sin_port = htons(0);
	localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	/*bind*/
	Bind(sendFileSock, (struct sockaddr *)&localAddr, sizeof(localAddr));
	
	File fileToSend;
	memset(&fileToSend, 0, sizeof(File));
	/*copy path*/
	strncpy(fileToSend.path, ((Args*)args)->data.filepath, FILE_PATH);
	/*fetch filename from the args*/
	if(!fetchFilenameFromPath(fileToSend.path, fileToSend.name, FILE_NAME)){
		printf("filepath is too long\n"); 
		return NULL;
	}
#ifdef DEBUG
	printf("filepath:%s\n", fileToSend.path);
	printf("filename:%s\n", fileToSend.name);
#endif
	/*open file and fetch file length*/	
	fileToSend.size = getFilesize(fileToSend.path);
	/*count the sum of blocks*/
	fileToSend.blkSum = countBlocks(fileToSend.size, BLOCK);
	/*blk to be sent*/
	fileToSend.curBlk = 0;	
	/*data to send*/
	Packet *pk = pack(&fileToSend, S_FILE, -1, NULL);
	/*send data*/
	setRemoteIP(&remoteAddr, ((Args*)args)->remoteIp, RECV_PORT);
	Sendto(sendFileSock, pk->data, pk->len, &remoteAddr, 
		   sizeof(struct sockaddr_in));
	freePacket(pk);
	BYTE dataRecvd[TYPE+BLOCKNUM+BLOCK];
	char ipBuf[IP_LEN];
	while(1){
		memset(dataRecvd, 0, TYPE+BLOCKNUM+BLOCK);
		int pos = 0;
		remoteAddrLen = sizeof(remoteAddr);
		ssize_t n = recvfrom(sendFileSock, dataRecvd,TYPE+BLOCKNUM+BLOCK , 0, 
					 (struct sockaddr*) &remoteAddr, &remoteAddrLen);
		if (n == -1 || errno == EINTR){
			if(errno == EINTR) {
				perror("interrupted!\n");
			}
			if(n == -1)
				perror("error!\n");
		}
		
		if(dataRecvd[0] == 0x04){
#ifdef DEBUG
			printf("received from %s at %d packet is 04\n", 
				 	inet_ntop(AF_INET, &remoteAddr.sin_addr, 
								ipBuf, sizeof(ipBuf)), ntohs(remoteAddr.sin_port));
#endif
					
			/*recv ack*/
			uint32_t reqNum = getIntFromNetChar(&dataRecvd[1]);
			if(fileToSend.curBlk == reqNum){
				if(reqNum < fileToSend.blkSum) {
					/*send data*/
					Packet *pk = pack(&fileToSend, S_DATA, reqNum, NULL);
					Sendto(sendFileSock, pk->data, pk->len, &remoteAddr, sizeof(struct sockaddr_in));
					freePacket(pk);

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
	}
}
void *recvFileProc(void *args) 
{
	BYTE *buffer = ((DataAndIp*)args)->dataRecvd;
	struct sockaddr_in *remoteIp = ((DataAndIp*)args)->remoteIp;
	/*local ip address*/
	struct sockaddr_in localAddr;
	/*remote ip address*/
//	struct sockaddr_in remoteAddr;
//	socklen_t remoteAddrLen;

	/*socket*/
	int recvFileSock = Socket(AF_INET, SOCK_DGRAM, 0);
	/*set localAddr*/
	memset(&localAddr, 0, sizeof(localAddr));
	localAddr.sin_family = AF_INET;
	localAddr.sin_port = htons(0);
	localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	/*bind*/
	Bind(recvFileSock, (struct sockaddr *)&localAddr, sizeof(localAddr));


	int pos = 0;
	File fileToRecv;
	/*fetch file name*/
	memset(&fileToRecv, 0, sizeof(File));
	pos++;
	int i = 0;
	while(buffer[pos] != '\0')
		fileToRecv.name[i++] = buffer[pos++];
	pos++;
	fileToRecv.size = getIntFromNetChar(&buffer[pos]);
	fileToRecv.blkSum = countBlocks(fileToRecv.size, BLOCK);
	/*file path*/
	char dstDir [19] = "/home/yh/projects/";
	//int filepathLen = strlen(dstDir) + strlen(fileToRecv.name) + 1;	
	//fileToRecv.path = (char*)malloc(filepathLen);
	//memset(fileToRecv.path, 0, filepathLen); 
	strncat(fileToRecv.path, dstDir, strlen(dstDir));
	strncat(fileToRecv.path, fileToRecv.name, strlen(fileToRecv.name));
	/*open the file*/
	if((fileToRecv.fp = fopen(fileToRecv.path, "wb+")) == NULL){
		perror("Error:open file failed!\n");
	}
	/*create ack pack*/
	fileToRecv.preBlk = -1;
	fileToRecv.curBlk = 0;
	Packet *pk = pack(NULL, S_ACK, fileToRecv.curBlk, NULL);
	Sendto(recvFileSock, pk->data, pk->len, remoteIp, sizeof(struct sockaddr_in));
	freePacket(pk);
	free(buffer);
	free(remoteIp);
#ifdef _TIMER
	/*start timer*/
	setTimer(curInterval, &orignalTime);
#endif
	/*remote ip address*/
	struct sockaddr_in remoteAddr;
	socklen_t remoteAddrLen;
	BYTE dataRecvd[TYPE+BLOCKNUM+BLOCK];
	char ipBuf[IP_LEN];
	while(1){
		ssize_t n = recvfrom(recvFileSock, dataRecvd,TYPE+BLOCKNUM+BLOCK , 0, 
					 (struct sockaddr*) &remoteAddr, &remoteAddrLen);
		if (n == -1 || errno == EINTR){
			if(errno == EINTR) {
				perror("interrupted!\n");
			}
			if(n == -1)
				perror("error!\n");
		}
		if(dataRecvd[pos] == 0x02){
#ifdef DEBUG
			printf("received from %s at %d packet is 02\n", 
				   inet_ntop(AF_INET, &remoteAddr.sin_addr, 
				   ipBuf, sizeof(ipBuf)), ntohs(remoteAddr.sin_port));
#endif
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
				Sendto(sockfd, pk->data, pk->len, &remoteAddr, sizeof(struct sockaddr_in));
				freePacket(pk);
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
	}
}
