#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <net/if.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/ethtool.h>
#include <linux/sockios.h>
#include <errno.h>

int ether_is_linked(char *ifname)
{
	int err;
	struct ethtool_value edata;
	struct ifreq ifr;
	static  int skt = -1;

	if(skt == -1)
		skt = socket(AF_INET, SOCK_DGRAM, 0);  
	edata.cmd = ETHTOOL_GLINK;
	edata.data = 0;
	ifr.ifr_data = (void*)&edata;
	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	err = ioctl(skt, SIOCETHTOOL, &ifr);
	if(err == 0) {
		if(edata.data)
			return 1;
		else	
			return 0;
	}
	else
		perror("ioctl");

	return  0;
}

int main()
{
	if( ether_is_linked("eth0") )
		printf("ether Linked \n");
	else
		printf("ether Unlinked \n");
	return 0;
}

