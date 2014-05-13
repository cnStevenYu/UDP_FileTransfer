#include "main.h"

#define BLOCK 1024
#define ZEROBYTE 1
#define BLOCKNUM 4
#define FILESIZE 4
#define TYPE 1
int main(int argc, char *argv[]){
	fileToSend.fp = NULL;
	//socket
	sockfd = Socket(AF_INET, SOCK_DGRAM, 0);
	//set localAddr
	bzero(&localAddr, sizeof(localAddr));
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
	unsigned char *dataToSend;
	while(fgets(comLine, BUF_SIZE, stdin) != NULL){
		bzero(&args, sizeof(args));
		ssize_t n = 0;
		switch(argsProc(&args, comLine, BUF_SIZE)){
			case OK:
				switch(args.type){
					case SEND_MSG:
						len = TYPE + strlen(args.data.msg) + ZEROBYTE;
						dataToSend = (unsigned char*)malloc(len);
						dataToSend[0] = 0x08;
						pos = 1;
						for(int i=0; i<strlen(args.data.msg); ){
							dataToSend[pos++] = args.data.msg[i++];
						}
						dataToSend[pos] = '\0';
						//set remoteAddr
						bzero(&remoteAddr, sizeof(remoteAddr));
						remoteAddr.sin_family = AF_INET;
						remoteAddr.sin_port = htons(RECV_PORT);
						remoteAddr.sin_addr.s_addr = inet_addr(args.remoteIp);
						n = sendto(sockfd, dataToSend, len, 0, (struct sockaddr*)&remoteAddr, sizeof(remoteAddr));
						if(n == -1){
							free(dataToSend);
							perr_exit("sendto error\n");
						}
						free(dataToSend);
						continue;
					case SEND_FILE:
						printf("send file\n");
						bzero(&fileToSend, sizeof(File));
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
						/*data to send*/
						bzero(&remoteAddr, sizeof(remoteAddr));
						remoteAddr.sin_family = AF_INET;
						remoteAddr.sin_port = htons(RECV_PORT);
						remoteAddr.sin_addr.s_addr = inet_addr(args.remoteIp);
						
						int filenameLen = strlen(fileToSend.name);
						len = 1+filenameLen+1+FILESIZE+1;
						dataToSend = (char *)malloc(len);
						pos = 0;
						dataToSend[pos++] = 0x1;
						for(int i = 0; i < filenameLen; )
							dataToSend[pos++] = fileToSend.name[i++];
						dataToSend[pos++] = 0x0;	
						/*convert filesize to network byte order*/
						setIntToNetChar(fileToSend.size, &dataToSend[pos]);
						pos += FILESIZE;
						dataToSend[pos++] = 0x0;
						/*send data*/
						n = 0;
						n = sendto(sockfd, dataToSend,len , 0, (struct sockaddr*)&remoteAddr, sizeof(remoteAddr));
						if(n == -1){
							free(dataToSend);
							perr_exit("sendto error\n");
						}
						free(dataToSend);
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
	ssize_t n = 0;
	unsigned char dataRecvd[TYPE+BLOCKNUM+BLOCK];
	char ipBuf[IP_LEN];
	File fileToRecv;
	while(1){
		remoteAddrLen = sizeof(remoteAddr);
		n = recvfrom(sockfd, dataRecvd,TYPE+BLOCKNUM+BLOCK , 0, (struct sockaddr*) &remoteAddr, &remoteAddrLen);
		if (n == -1)
			perr_exit("recvfrom error");
		int pos = 0;
		/*recv file*/
		if(dataRecvd[pos] == 0x01){
			printf("received from %s at %d packet is 01\n", inet_ntop(AF_INET, &remoteAddr.sin_addr, ipBuf, sizeof(ipBuf)), ntohs(remoteAddr.sin_port));
			/*fetch file name*/
			bzero(&fileToRecv, sizeof(File));
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
			bzero(fileToRecv.path,filepathLen); 
			strncat(fileToRecv.path, dstDir, strlen(dstDir));
			strncat(fileToRecv.path, fileToRecv.name, strlen(fileToRecv.name));
			/*open the file*/
			if((fileToRecv.fp = fopen(fileToRecv.path, "wb+")) == NULL){
				free(fileToRecv.path);
				perr_exit("fopen fail!\n");
			}
			free(fileToRecv.path);
			/*create ack pack*/
			int len = TYPE+BLOCKNUM;
			unsigned char *buf = (unsigned char *)malloc(len);
			buf[0] = 0x04;
			setIntToNetChar(0, &buf[1]);
			n = sendto(sockfd, buf, len, 0, (struct sockaddr*)&remoteAddr, sizeof(remoteAddr));
			free(buf);
			if (n == -1)
				perr_exit("recvfrom error");
		} 
		else{
			if(dataRecvd[pos] == 0x02){
				printf("received from %s at %d packet is 02\n", inet_ntop(AF_INET, &remoteAddr.sin_addr, ipBuf, sizeof(ipBuf)), ntohs(remoteAddr.sin_port));
				uint32_t blkNum = getIntFromNetChar(&dataRecvd[1]);
				unsigned char *dataBlock;
				ssize_t nwrite;
				/*write to file*/
				fseek(fileToRecv.fp, blkNum*BLOCK, SEEK_SET);
				if(blkNum == fileToRecv.blkSum - 1){
					int r = fileToRecv.size % BLOCK;
					if(r>0){
						dataBlock = (unsigned char*)malloc(r);
						for(int i=0; i < r;i++){
							dataBlock[i] = dataRecvd[TYPE+BLOCKNUM+i];
						}	
						nwrite = fwrite(dataBlock, sizeof(unsigned char), r, fileToRecv.fp);
					}
					else{
						dataBlock = (unsigned char *)malloc(BLOCK);
						for(int i=0; i < BLOCK;i++){
							dataBlock[i] = dataRecvd[TYPE+BLOCKNUM+i];
						}	
						nwrite = fwrite(dataBlock, sizeof(unsigned char), BLOCK, fileToRecv.fp);
					}
					fclose(fileToRecv.fp);
					fileToRecv.fp = NULL;

				}
				else{
					dataBlock = (unsigned char *)malloc(BLOCK);
					for(int i=0; i < BLOCK;i++){
						dataBlock[i] = dataRecvd[TYPE+BLOCKNUM+i];
					}	
					nwrite = fwrite(dataBlock, sizeof(unsigned char), BLOCK, fileToRecv.fp);
				}
				free(dataBlock);	
				/*request next block*/
				if(blkNum < fileToRecv.blkSum - 1){
					int len = TYPE+BLOCKNUM;
					unsigned char *buf = (unsigned char *)malloc(len);
					buf[0] = 0x04;
					setIntToNetChar(blkNum+1, &buf[1]);
					n = sendto(sockfd, buf, len, 0, (struct sockaddr*)&remoteAddr, sizeof(remoteAddr));
					free(buf);
					if (n == -1)
						perr_exit("recvfrom error");
				}	
			}
			else{
				if(dataRecvd[pos] == 0x04){
					printf("received from %s at %d packet is 04\n", inet_ntop(AF_INET, &remoteAddr.sin_addr, ipBuf, sizeof(ipBuf)), ntohs(remoteAddr.sin_port));
					int len = TYPE+BLOCKNUM+BLOCK;	
					unsigned char *dataToSend = (unsigned char*)malloc(len);
					dataToSend[0] = 0x02;
					/*recv ack*/
					uint32_t reqNum = getIntFromNetChar(&dataRecvd[1]);
					if(reqNum < fileToSend.blkSum) {
						setIntToNetChar(reqNum, &dataToSend[1]);
						/*fetch data from file*/
						if(fileToSend.fp == NULL) {//not open
							if((fileToSend.fp = fopen(fileToSend.path, "rb")) == NULL)
								perr_exit("file open failed!\n");
						}
						fseek(fileToSend.fp, reqNum*BLOCK, SEEK_SET);
						ssize_t nread = fread(&dataToSend[TYPE+BLOCKNUM], sizeof(unsigned char), BLOCK, fileToSend.fp);
						/*send*/
						n = sendto(sockfd, dataToSend, len, 0, (struct sockaddr*)&remoteAddr, sizeof(remoteAddr));
						free(dataToSend);
						if(n == -1)
							perr_exit("sendto error");
							
					}
					else{
						/*the receiver has received the last pack*/
						fclose(fileToSend.fp);
						fileToSend.fp = NULL;
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
	bzero(filename, nameLen);
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
		dataToSend[i] = *((unsigned char *)(&fileszn)+i);
	}
}
uint32_t getIntFromNetChar(char *dataRecvd){
	int i;
	uint32_t filesize;
	for(i = 0; i<FILESIZE; i++)
		((unsigned char *)&filesize)[i] = dataRecvd[i];
	return ntohl(filesize);
}
uint32_t countBlocks(uint32_t filesize, uint32_t block){
	uint32_t r = filesize % (block+1);
	return (filesize - r) / block + 1;
}
