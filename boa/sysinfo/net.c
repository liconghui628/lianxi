#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ip.h>
#include <netpacket/packet.h>

//get mac address
int ether_get_mac(char *ifname, char *mac, int size)
{
	int skfd = -1;
	struct ifreq ifr;
	
	if(size < 18)
		return -1;
	memset(&ifr, 0, sizeof(struct ifreq));
	skfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	if (skfd >= 0) {
		ifr.ifr_addr.sa_family = AF_INET;
		strcpy(ifr.ifr_name, ifname);
		if(ioctl(skfd, SIOCGIFHWADDR, &ifr) == 0){
			snprintf(mac,size,"%.2X:%.2X:%.2X:%.2X:%.2X:%.2X",(uint8_t)ifr.ifr_hwaddr.sa_data[0],(uint8_t)ifr.ifr_hwaddr.sa_data[1],
					(uint8_t)ifr.ifr_hwaddr.sa_data[2],(uint8_t)ifr.ifr_hwaddr.sa_data[3],(uint8_t)ifr.ifr_hwaddr.sa_data[4],(uint8_t)ifr.ifr_hwaddr.sa_data[5]);
		}
		close(skfd);
	}

	return 0;
}

//get ip address
int ether_get_ip(char *ifname, char *ip, int size)
{
	int skfd = -1;
	struct ifreq ifr;
	if(!ifname || !ip || size < 16)
		return -1;
	
	skfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
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
	strncpy(ip, (const char*)inet_ntoa(((struct sockaddr_in *)(&ifr.ifr_addr))->sin_addr), size);
	close(skfd);

	return  0;
}

void netinfo(char *mac_buff, char *ip_buff, int mac_size, int ip_size)
{
	ether_get_mac("eth0", mac_buff, mac_size);
	ether_get_ip("eth0", ip_buff, ip_size);
}
