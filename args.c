#include "public.h"
#include "args.h"
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
	if(comLine[pos] == 'l'){
		args->type = LIST_FRIENDS;
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