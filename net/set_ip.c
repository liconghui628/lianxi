#include <stdio.h>  
#include <stdlib.h>  
#include <sys/ioctl.h>  
#include <string.h>  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <net/if.h>  
#include <netinet/in.h>  

//get ip address
int ether_get_ip(char *ifname, uint8_t *ip, int size)
{
	int skfd = -1;
	struct ifreq ifr;
	if(!ifname || !ip || size < 4)
		return -1;
	
	//skfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd < 0){
		perror("socket():");
		return -1;
	}
	
	memset(&ifr, 0, sizeof(struct ifreq));
	//ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, ifname,sizeof(ifr.ifr_name) - 1);
	if(ioctl(skfd, SIOCGIFADDR, &ifr) < 0){
		perror("ioctl():");
		close(skfd);
		return -1;
	}
	//printf("IP address is: \n%s\n", inet_ntoa(((struct sockaddr_in *)(&ifr.ifr_addr))->sin_addr));  
	/*int i;
	for( i = 0; i<16;i++){
		printf("%hhu ",ifr.ifr_addr.sa_data[i]);
	}
	printf("\n");
	*/
	memcpy(ip, &ifr.ifr_addr.sa_data[2], size);
	close(skfd);

	return  0;
}

//set ip address
int ether_set_ip(char *ifname, char *ip)
{
	int skfd = -1;
	struct ifreq ifr;
	if(!ifname || !ip )
		return -1;
	
	//skfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd < 0){
		perror("socket():");
		return -1;
	}
	
	memset(&ifr, 0, sizeof(struct ifreq));
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, ifname,sizeof(ifr.ifr_name) - 1);
	struct sockaddr_in *p = (struct sockaddr_in *)&(ifr.ifr_addr);
	inet_aton(ip, &(p->sin_addr));

	if(ioctl(skfd, SIOCSIFADDR, &ifr) < 0){
		perror("ioctl():");
		close(skfd);
		return -1;
	}
	close(skfd);

	return  0;
}
int main()
{
	unsigned char ip[4] = {0};
	
	ether_get_ip("eth0",ip,sizeof(ip));
	printf("begin change IP: %d.%d.%d.%d\n",ip[0],ip[1],ip[2],ip[3]);

	ether_set_ip("eth0", "192.168.119.11");
	ether_get_ip("eth0",ip,sizeof(ip));
	printf("after change IP: %d.%d.%d.%d\n",ip[0],ip[1],ip[2],ip[3]);
	
	return 0;
}
