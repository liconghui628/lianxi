#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <netinet/ether.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netpacket/packet.h>

#define    FAILURE   -1  
#define    SUCCESS    0

unsigned char src_ip[4] = {0};
unsigned char src_mac[6] = {0};
unsigned char dest_ip[4] = {0};
unsigned char dest_mac[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};//arp broadcast
char gstrmac[18] = {0};

typedef struct _tagARP_PACKET{    
    struct ether_header  eh;    
    struct ether_arp arp;    
}ARP_PACKET_OBJ, *ARP_PACKET_HANDLE;

//get mac address
int ether_get_mac(char *ifname, uint8_t *mac, int size)
{
	int skfd = -1;
	struct ifreq ifr;
	
	if(size < 6)
		return -1;
	memset(&ifr, 0, sizeof(struct ifreq));
	skfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	if (skfd >= 0) {
		ifr.ifr_addr.sa_family = AF_INET;
		strcpy(ifr.ifr_name, ifname);
		if(ioctl(skfd, SIOCGIFHWADDR, &ifr) == 0){
			memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);
			printf("src_mac: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",
				mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

		}
		close(skfd);
	}

	return 0;
}

//get ip address
int ether_get_ip(char *ifname, uint8_t *ip, int size)
{
	int skfd = -1;
	struct ifreq ifr;
	if(!ifname || !ip || size < 4)
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

// get ether mode ; 10M or other
int ether_get_mode(char *ifname)
{
	int skfd = -1;
	struct ifreq ifr;
	
	if(!ifname)
		return -1;
	
	skfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	if(skfd < 0){
		perror("socket():");
		return -1;
	}
	
	memset(&ifr, 0, sizeof(struct ifreq));
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, ifname,sizeof(ifr.ifr_name) - 1);
	if(ioctl(skfd, SIOCGIFINDEX, &ifr) < 0){
		perror("ioctl():");
		close(skfd);
		return -1;
	}
	close(skfd);

	return  ifr.ifr_ifindex;
}


int send_arp(int sockfd, struct sockaddr_ll *peer_addr)  
{  
    int rtval, i;  
    ARP_PACKET_OBJ frame;  
    memset(&frame, 0x00, sizeof(ARP_PACKET_OBJ));  
      
    memcpy(frame.eh.ether_dhost, dest_mac, 6);   
    memcpy(frame.eh.ether_shost, src_mac, 6);    
    frame.eh.ether_type = htons(ETH_P_ARP);       
  
    frame.arp.ea_hdr.ar_hrd = htons(ARPHRD_ETHER);      
	frame.arp.ea_hdr.ar_pro = htons(ETHERTYPE_IP);     
	frame.arp.ea_hdr.ar_hln = 6;                 
	frame.arp.ea_hdr.ar_pln = 4;                  
	frame.arp.ea_hdr.ar_op = htons(ARPOP_REQUEST);      
	memcpy(frame.arp.arp_sha, src_mac, 6);     
	memcpy(frame.arp.arp_spa, src_ip, 4);      
	memcpy(frame.arp.arp_tha, dest_mac, 6);      
	memcpy(frame.arp.arp_tpa, dest_ip, 4);      
	for(i = 0; i < sizeof(ARP_PACKET_OBJ); i++)
		printf("%.2X ",((uint8_t*)&frame)[i]);
	printf("\n");

	rtval = sendto(sockfd, &frame, sizeof(ARP_PACKET_OBJ), 0,   
			(struct sockaddr*)peer_addr, sizeof(struct sockaddr_ll));    
    if (rtval < 0)  
    {  
		perror("sendto():");
        return FAILURE;  
    }  
    return SUCCESS;  
}


int recv_arp(int sockfd, struct sockaddr_ll *peer_addr)  
{  
    int rtval = 0, i = 0, j = 0;  
    ARP_PACKET_OBJ frame;  
      
    memset(&frame, 0, sizeof(ARP_PACKET_OBJ));  
	for(i = 0; i < 3; i++){
    	rtval = recvfrom(sockfd, &frame, sizeof(frame), 0, NULL, NULL);  
	    if (htons(ARPOP_REPLY) == frame.arp.ea_hdr.ar_op && rtval > 0)  
   		{ 
				for( j = 0; j < rtval; j++)
					printf("%.2X ",((uint8_t*)&frame)[j]);
				printf("\n");
				snprintf(gstrmac, sizeof(gstrmac), "%02X:%02X:%02X:%02X:%02X:%02X",frame.arp.arp_sha[0],frame.arp.arp_sha[1],
					frame.arp.arp_sha[2],frame.arp.arp_sha[3],frame.arp.arp_sha[4],frame.arp.arp_sha[5]);
            	return SUCCESS;  
    	}  
	}
    return FAILURE;  
}

int main(int argc, char *argv[])
{
	int sockfd, rtval;
	struct sockaddr_ll peer_addr;
	struct ifreq req;
	struct timeval tv;
	in_addr_t ipaddr;

	if(argc < 2){
		printf("use: myarp destMacAddr \n");
		return -1;
	}

	ipaddr = inet_addr(argv[1]);
	memcpy(dest_ip, &ipaddr, sizeof(dest_ip));
	if(ether_get_mac("ens33", src_mac, sizeof(src_mac)) < 0){
		printf("get src_mac error\n");
		return -1;
	}
	printf("src_mac: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",
			src_mac[0],src_mac[1],src_mac[2],src_mac[3],src_mac[4],src_mac[5]);

	// get src_ip
	if(ether_get_ip("ens33", src_ip, sizeof(src_ip)) < 0){
		printf("get src_ip error\n");
		return -1;
	}
	printf("src_ip: %d.%d.%d.%d\n",src_ip[0],src_ip[1],src_ip[2],src_ip[3]);

	memset(&peer_addr, 0, sizeof(peer_addr));
	peer_addr.sll_family = AF_PACKET;
	peer_addr.sll_protocol = htons(ETH_P_ARP);
	//peer_addr.sll_ifindex = 2;	//ether Mode IF_PORT_10BASET
	peer_addr.sll_ifindex = ether_get_mode("ens33");	//ether Mode
	if(peer_addr.sll_ifindex < 0){
		printf("get ether mode error");
		return -1;
	}
	//printf("ether mode:%d\n", peer_addr.sll_ifindex); 
	
	sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
	if(sockfd == -1){
		perror("socket():");
		return -1;
	}
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	setsockopt(sockfd,SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof(struct timeval));
	setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(struct timeval));

	rtval = send_arp(sockfd, &peer_addr);
	if(rtval == FAILURE)
		printf("send_arp error\n");

	rtval = recv_arp(sockfd, &peer_addr);
	if(rtval == FAILURE)
		printf("recv_arp error\n");

	printf("Dest MacAddr:%s\n",gstrmac);

	return 0;
}
