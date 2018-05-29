#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

int get_broadcast_addr(char *buff, int buff_size)
{
    int fd;
    int i;
    struct ifreq *ifreq;
    struct ifreq ifr;
    struct ifconf ifc;
    unsigned char buf[512];
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        printf("create socket error\n");
        return -1;
    }
    
    ifc.ifc_len = 512; 
    ifc.ifc_buf = buf;
    if (ioctl(fd, SIOCGIFCONF, (char *)&ifc) < 0){
        printf("ioctl error:%s\n", strerror(errno));
        return -1;
    }
    ifreq = (struct ifreq*)buf;
    printf("%d\n", ifc.ifc_len/sizeof(struct ifreq));
    for(i = 0; i < ifc.ifc_len/sizeof(struct ifreq); i++){
        printf("%s\n", ifreq->ifr_name);
        if (strcmp(ifreq->ifr_name, "lo") != 0 && strcmp(ifreq->ifr_name, "usb0") != 0) {
            printf("find it\n");
            break;
        }
        ifreq ++;
    }
    if (i > ifc.ifc_len/sizeof(struct ifreq)) {
        printf("did not find valuable eth\n");
        return -1;
    }
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, ifreq->ifr_name);  
    if (ioctl(fd, SIOCGIFBRDADDR, &ifr) < 0) {
        printf("ioctl error:%s\n", strerror(errno));
        return -1;
    }
    printf("broadcast addr:%s\n", inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_broadaddr))->sin_addr));
    strncpy(buff, inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_broadaddr))->sin_addr), buff_size);
    close(fd);
}

int main()
{
    char addr[20];
    memset(addr, 0, sizeof(addr));
    get_broadcast_addr(addr, sizeof(addr));
    printf("broadcast addr:%s\n", addr);
    return 0;
}
