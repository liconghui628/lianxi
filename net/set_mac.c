#include <stdio.h>  
#include <stdlib.h>  
#include <sys/ioctl.h>  
#include <string.h>  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <net/if.h>  
#include <netinet/in.h>  
  
int ifconfig_ethx_down_API(const char *interface_name)  
{  
    int     sock_fd;  
    struct 	ifreq ifr;  
    int     selector;   
       
    // check params
    if(interface_name == NULL)  
    {  
        fprintf(stdout, "%s:%d: args invalid!", __func__, __LINE__);  
        return -1;  
    }  
      
    // cant not close lo   
    if(strncmp((char *)interface_name, (char *)"lo", 2) == 0)  
    {  
       fprintf(stdout, "%s:%d: You can't pull down interface lo!",  __func__, __LINE__);  
       return 0;      
    }  
     
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);  
    if(sock_fd < 0)  
    {  
		perror("ioctl");
        fprintf(stdout, "%s:%d: socket failed!",  __func__, __LINE__);  
        return -2;  
    }  
  
    sprintf(ifr.ifr_name, "%s", interface_name);  
     
    if(ioctl(sock_fd, SIOCGIFFLAGS, &ifr) < 0)  
    {  
		perror("ioctl");
        fprintf(stdout, "%s:%d: ioctl failed 1!",  __func__, __LINE__);  
        return -3;  
    }  
      
    selector = IFF_UP;  
    ifr.ifr_flags &= ~selector;   
    if(ioctl(sock_fd, SIOCSIFFLAGS, &ifr) < 0)  
    {  
		perror("ioctl");
        fprintf(stdout, "%s:%d: ioctl failed 2!",  __func__, __LINE__);  
        return -4;  
    }  
  
    close( sock_fd );  
      
    return 0;  
}  
  
int ifconfig_ethx_up_API(const char *interface_name)  
{  
    int           sock_fd;  
    struct ifreq    ifr;  
    int             selector;   
      
    // check params  
    if(interface_name == NULL)  
    {  
        fprintf(stdout, "%s:%d: args invalid!",  __func__, __LINE__);  
        return -1;  
    }  
  
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);  
    if(sock_fd < 0)  
    {  
		perror("ioctl");
        fprintf(stdout, "%s:%d: create socket failed!",  __func__, __LINE__);  
        return -2;  
    }  
      
    sprintf(ifr.ifr_name, "%s", interface_name);  
    if(ioctl(sock_fd, SIOCGIFFLAGS, &ifr) < 0)  
    {  
		perror("ioctl");
        fprintf(stdout, "%s:%d: ioctl error 1",  __func__, __LINE__);  
        return -3;  
    }  
      
    selector = (IFF_UP | IFF_RUNNING);  
    ifr.ifr_flags |= selector;    
    if(ioctl(sock_fd, SIOCSIFFLAGS, &ifr) < 0)  
    {  
		perror("ioctl");
        fprintf(stdout, "%s:%d: ioctl error 2",  __func__, __LINE__);  
        return -4;  
    }  
  
    close( sock_fd );  
      
    return 0;     
}  

int SetLocalMACAddr_API(const char *interface_name, const unsigned char *str_macaddr)    
{  
    int             ret;    
    int             sock_fd;    
    struct ifreq    ifr;        
    unsigned int    mac2bit[6];  
      
    // check params  
    if(interface_name == NULL || str_macaddr == NULL)  
    {  
        fprintf(stdout, "%s:%d: args invalid!",  __func__, __LINE__);  
        return -1;  
    }  
      
    sscanf((char *)str_macaddr, "%02X:%02X:%02X:%02X:%02X:%02X", 
			&mac2bit[0], &mac2bit[1], &mac2bit[2], &mac2bit[3], &mac2bit[4], &mac2bit[5]);  
      
    sock_fd = socket(PF_INET, SOCK_DGRAM, 0);    
    if (sock_fd < 0)    
    {    
		perror("ioctl");
        perror("socket error");         
        return -2;    
    }    
      
    ret = ifconfig_ethx_down_API( interface_name );  
    if(ret < 0)  
    {  
        fprintf(stdout, "%s:%d: close eth0 error",  __func__, __LINE__);  
        return -3;  
    }  
    sleep(1); //  wait ether close OK
      
    sprintf(ifr.ifr_ifrn.ifrn_name, "%s", interface_name);    
    ifr.ifr_ifru.ifru_hwaddr.sa_family = 1;    
    ifr.ifr_ifru.ifru_hwaddr.sa_data[0] = mac2bit[0];  
    ifr.ifr_ifru.ifru_hwaddr.sa_data[1] = mac2bit[1];  
    ifr.ifr_ifru.ifru_hwaddr.sa_data[2] = mac2bit[2];  
    ifr.ifr_ifru.ifru_hwaddr.sa_data[3] = mac2bit[3];  
    ifr.ifr_ifru.ifru_hwaddr.sa_data[4] = mac2bit[4];  
    ifr.ifr_ifru.ifru_hwaddr.sa_data[5] = mac2bit[5];  
      
    ret = ioctl(sock_fd, SIOCSIFHWADDR, &ifr);    
    if (ret != 0)    
    {    
		perror("ioctl");
        perror("set mac address erorr");  
        return -4;    
    }    
      
    close( sock_fd );  
      
    ret = ifconfig_ethx_up_API( interface_name );  
    if(ret < 0)  
    {  
        fprintf(stdout, "%s:%d: open eth0 error!",  __func__, __LINE__);  
        return -5;  
    }  
      
    sleep(2); // wait ether open OK  
      
    return 0;    
}  
  
int GetLocalMACAddr_API(const unsigned char *interface_name, unsigned char *str_macaddr)    
{    
    int        sock_fd;    
    struct ifreq ifr_mac;    
      
    // check params  
    if(interface_name == NULL || str_macaddr == NULL)  
    {  
        fprintf(stdout, "%s:%d: args invalid!",  __func__, __LINE__);  
        return -1;  
    }  
      
    sock_fd = socket( AF_INET, SOCK_STREAM, 0 );    
    if( sock_fd == -1)    
    {    
		perror("ioctl");
        perror("create socket failed");    
        sprintf((char *)str_macaddr, "00:00:00:00:00:00");  
        return -2;    
    }    
        
    memset(&ifr_mac, 0, sizeof(ifr_mac));       
    sprintf(ifr_mac.ifr_name, "%s", interface_name);       
    
    if( (ioctl( sock_fd, SIOCGIFHWADDR, &ifr_mac)) < 0 )   
    {    
        perror("mac ioctl error");    
        sprintf((char *)str_macaddr, "00:00:00:00:00:00");  
        return -3;    
    }    
        
    close( sock_fd );    
      
    sprintf((char *)str_macaddr,"%02x:%02x:%02x:%02x:%02x:%02x",    
            (unsigned char)ifr_mac.ifr_hwaddr.sa_data[0],    
            (unsigned char)ifr_mac.ifr_hwaddr.sa_data[1],    
            (unsigned char)ifr_mac.ifr_hwaddr.sa_data[2],    
            (unsigned char)ifr_mac.ifr_hwaddr.sa_data[3],    
            (unsigned char)ifr_mac.ifr_hwaddr.sa_data[4],    
            (unsigned char)ifr_mac.ifr_hwaddr.sa_data[5]);    
    
    printf("local mac:<%s> \n", str_macaddr);        
      
    return 0;  
}  
  
  
  
int main(void)  
{  
    unsigned char str_macaddr[20];  
      
    memset(str_macaddr, 0, sizeof(str_macaddr));  
    GetLocalMACAddr_API("eth0", str_macaddr);  
    fprintf(stdout, "1 mac: %s\n", str_macaddr);  
      
    //ifconfig_ethx_down_API("eth0");  
          
    //system("ifconfig eth0 down");  
    //usleep(500000);  
    //system("ifconfig eth0 up");  
    //usleep(500000);  
    //sleep(1); //10ms  
    //ifconfig_ethx_down_API("eth0");  
    //sleep(1);  
      
    SetLocalMACAddr_API("eth0", "08:00:11:22:33:44");  
    //ifconfig_ethx_up_API("eth0");  
    //ifconfig_ethx_up_API("eth0");  
    //sleep(2);   
      
    memset(str_macaddr, 0, sizeof(str_macaddr));  
    GetLocalMACAddr_API("eth0", str_macaddr);  
    fprintf(stdout, "2 mac: %s\n", str_macaddr);  
      
    //usleep(50000);   
    //ifconfig_ethx_down_API("eth0");  
    //ifconfig_ethx_up_API("eth0");  
  
    //memset(str_macaddr, 0, sizeof(str_macaddr));  
    //GetLocalMACAddr_API("eth0", str_macaddr);  
    fprintf(stdout, "2 mac: %s\n", str_macaddr);  
          
    return 0;  
}  
