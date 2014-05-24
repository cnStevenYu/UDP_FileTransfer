#include "host.h"
int getLocalIp(char *ipaddr)
{
	int sock_get_ip;  
	struct   sockaddr_in *sin;  
	struct   ifreq ifr_ip;     

	if ((sock_get_ip=socket(AF_INET, SOCK_STREAM, 0)) == -1)  
	{  
		printf("socket create failse...GetLocalIp!/n");  
		return -1;  
	}  

	memset(&ifr_ip, 0, sizeof(ifr_ip));     
	strncpy(ifr_ip.ifr_name, "eth0", sizeof(ifr_ip.ifr_name) - 1);     

	if( ioctl( sock_get_ip, SIOCGIFADDR, &ifr_ip) < 0 )     
	{     
		return -1;     
	}       
	sin = (struct sockaddr_in *)&ifr_ip.ifr_addr;     
	strcpy(ipaddr,inet_ntoa(sin->sin_addr));         

	printf("local ip:%s \n",ipaddr);      
	close( sock_get_ip );  
	return 0;  
}

int getBroadcastAddr(int sockfd, struct sockaddr_in *broadcast_addr)
{
	 
	char buffer[1024];
	int so_broadcast = 1; 
	struct ifreq *ifr;  
	struct ifconf ifc;  
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
	/*默认的套接字描述符sock是不支持广播，必须设置套接字描述符以支持广播*/
	int n = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &so_broadcast,  
				sizeof(so_broadcast));
	return n;
}