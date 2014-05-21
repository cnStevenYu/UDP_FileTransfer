#define _XOPEN_SOURCE
#include "main.h"
struct itimerval orignalTime;
void setTimer(int interval, struct itimerval* oval);
void retransmit(int signo);

int main(int argc, char *argv[]){

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

	char comLine[BUF_SIZE];
	pthread_t recvThr;
	recvThr = pthread_create(&recvThr, NULL, &recvProc, NULL);
	/*arguments*/
	Args args;
	int pos = 0;
	int len = 0;
	while(fgets(comLine, BUF_SIZE, stdin) != NULL){
		memset(&args, 0, sizeof(args));
		ssize_t n = 0;
		Packet *pk;
		switch(argsProc(&args, comLine, BUF_SIZE)){
			case OK:
				switch(args.type){
					case SEND_MSG:
						pk = pack(NULL, S_MSG, -1, &args);
						//set remoteAddr
						memset(&remoteAddr, 0, sizeof(remoteAddr));
						remoteAddr.sin_family = AF_INET;
						remoteAddr.sin_port = htons(RECV_PORT);
						remoteAddr.sin_addr.s_addr = inet_addr(args.remoteIp);
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
						memset(&remoteAddr, 0, sizeof(remoteAddr));
						remoteAddr.sin_family = AF_INET;
						remoteAddr.sin_port = htons(RECV_PORT);
						remoteAddr.sin_addr.s_addr = inet_addr(args.remoteIp);

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
						printf("list \n");
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
	/*register timer*/
	struct sigaction act;
	act.sa_handler = retransmit;
	sigemptyset(&act.sa_mask);
	act.sa_flags |= SA_RESTART;
	sigaction(SIGALRM, &act, NULL);

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
			/*start timer*/
			setTimer(curInterval, &orignalTime);
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
					setTimer(curInterval, NULL);
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
				}
			}
		}


		//unpack(buf, BUF_SIZE);
		/*	
			for(int i=0; i<n; i++)
			buf[i] = toupper(buf[i]);
			n = sendto(sockfd, buf, n, 0, (struct sockaddr*)&remoteAddr, sizeof(remoteAddr));
			if(n == -1)
			perr_exit("sendto error");
			*/
	}	
}

ERR argsProc(Args* args, char *comLine, int len){

	/*get rid of blanks in front of comline*/
	int pos = 0;
	while(comLine[pos] == ' ')
		pos++;
	if(comLine[pos] == '\n')
		return ARGS_FORMAT_ERR;
	/*send file: s remote_ip filepath*/
	if(comLine[pos] == 's'){
		args->type = SEND_FILE;
		pos++;
		while(comLine[pos] == ' ')
			pos++;
		if(!verifyIP(&comLine[pos], args->remoteIp, BUF_SIZE))
			return IP_FORMAT_ERR;
		pos += strlen(args->remoteIp);

		while(comLine[pos] == ' ')
			pos++;
		if(comLine[pos] == '\n')
			return ARGS_FORMAT_ERR;
		int i = 0;
		while(comLine[pos] != '\n'){
			args->data.filepath[i++] = comLine[pos++];
		}
		return OK;
	}
	/*send msg: m remote_ip msg*/
	if(comLine[pos] == 'm'){
		args->type = SEND_MSG;
		pos++;
		while(comLine[pos] == ' ')
			pos++;
		if(!verifyIP(&comLine[pos], args->remoteIp, BUF_SIZE))
			return IP_FORMAT_ERR;
		pos += strlen(args->remoteIp);

		while(comLine[pos] == ' ')
			pos++;
		if(comLine[pos] == '\n')
			return ARGS_FORMAT_ERR;
		int msgIdx = 0;
		while(comLine[pos] != '\n'){
			args->data.msg[msgIdx++] = comLine[pos++];
		}
		return OK;
	}
	if(comLine[pos] == 'q'){	
		args->type = QUIT;
		return OK;
	}
	return ARGS_FORMAT_ERR;
}
/*fetch ip address from src to dst, if ip format is wrong, return false*/
BOOL verifyIP(char *src, char *dst, int len){
	int pos = 0;
	int numCnt = 0;
	int dotCnt = 0;
	int ipIdx = 0;
	while(src[pos] == '.' || (src[pos] >= '0' && src[pos] <= '9')) {
		if(src[pos] == '.'){
			numCnt = 0;
			dotCnt++;
		} else{
			numCnt++;	
			/*the number of digit nums must be no more than 3*/
			if(numCnt > 3)
				return FALSE;
		}
		dst[ipIdx++] = src[pos++];
	}	
	/*there must be 3 dots in the ip*/
	if(dotCnt != 3)
		return FALSE;
	return TRUE;
}
/*fetch filename from fielpath to filename, return false if filename it too long*/
BOOL fetchFilenameFromPath(char *filepath, char *filename, int nameLen){
	memset(filename, 0, nameLen);
	int pathLen = strlen(filepath);
	int pos = pathLen - 1;	
	while(filepath[pos] != '/' && pos != 0)
		pos--;
	pos++;
	/*if the length of filename if more than nameLen, return false*/
	if(pathLen - pos >= nameLen) 
		return FALSE;		
	int filenameLen = pathLen - pos; 
	for(int i = 0;i < filenameLen; ){
		filename[i++] = filepath[pos++];
	}
	return TRUE;
}
uint32_t getFilesize(char *filepath){
	FILE *fp;
	uint32_t size;
	if((fp = fopen(filepath, "rb")) == NULL)
		perror("file open failed!\n");	
	if(fseek(fp, 0L, SEEK_END) < 0)
		perror("file seek failed!\n");
	size = ftell(fp);	
	if(fclose(fp) < 0)
		perror("file close failed!\n");
	return size;
}
void setRemoteIP(struct sockaddr_in *remoteAddr, char *remoteIp, int port){
	remoteAddr->sin_family = AF_INET;
	remoteAddr->sin_port = htons(port);
	remoteAddr->sin_addr.s_addr = inet_addr(remoteIp);
}
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
uint32_t countBlocks(uint32_t filesize, uint32_t block){
	uint32_t r = filesize % (block+1);
	return (filesize - r) / block + 1;
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
					perr_exit("file open failed!\n");
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
	}
	return pk;
}
void setTimer(int interval, struct itimerval *oval) 
{
	struct itimerval val;
	val.it_value.tv_sec=interval;
	val.it_value.tv_usec=0;
	val.it_interval.tv_sec=0; 
	val.it_interval.tv_usec=0;
	setitimer(ITIMER_REAL,&val,oval);
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
