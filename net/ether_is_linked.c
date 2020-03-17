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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <ifaddrs.h>
#include <linux/wireless.h>

static int net_is_ok(const char *if_name)
{
    int fd = -1;
    struct ifreq ifr;
    struct ifconf ifc;
    struct ifreq ifrs_buf[100];
    int if_number =0;
    int i;
    int retval = 0;

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("%s: socket error [%d] %s\n",if_name, errno, strerror(errno));
        return 0;
    }

    ifc.ifc_len = sizeof(ifrs_buf);
    ifc.ifc_buf = (caddr_t)ifrs_buf;
    if (ioctl(fd, SIOCGIFCONF, (char *)&ifc) <0) {
        printf("%s: ioctl SIOCGIFCONF error [%d] %s\n",if_name, errno, strerror(errno));
        goto failed;
    }

    if_number = ifc.ifc_len / sizeof(struct ifreq);
    for(i=0; i< if_number; i++) {
        if(strcmp(if_name,ifrs_buf[i].ifr_name ) == 0) {
            break;
        }
    }

    if(i >= if_number) {
        printf("%s DEVICE_NONE\n", if_name);
        goto failed;
    }

    bzero(&ifr, sizeof(ifr));
    strncpy(ifr.ifr_name, if_name, IFNAMSIZ-1);
    ifr.ifr_name[IFNAMSIZ-1] = 0;
    if (ioctl(fd, SIOCGIFFLAGS, (char *)&ifr) <0) {
        printf("%s: ioctl SIOCGIFFLAGS error [%d] %s\n",if_name, errno, strerror(errno));
        goto failed;
    }  
    if(!(ifr.ifr_flags & IFF_UP))  {
        printf("%s DEVICE_DOWN\n", if_name);
        goto failed;
    }  

    if(!(ifr.ifr_flags & IFF_RUNNING)) {  
        printf("%s DEVICE_UNPLUGGED\n", if_name);
        goto failed;
    }  

    printf("%s DEVICE_LINKED\n", if_name);
    retval = 1;
failed:
    close(fd);
    return retval;  
}

int main()
{
	if(net_is_ok("ens33") )
		printf("ether Linked \n");
	else
		printf("ether Unlinked \n");
	return 0;
}

