
#ifdef WIN32
#include <windows.h>
#include <Iphlpapi.h>
#else
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <ifaddrs.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <linux/wireless.h>
#endif  //!WIN32
#include "libleiotpri.h"
#define MODULE_TAG   "LEIOT-NETWK"
#include "log.h"

#ifdef WIN32
#pragma comment(lib,"Iphlpapi.lib")
#pragma comment(lib,"ws2_32.lib")
#endif  //!WIN32

struct oct_env {
#ifndef WIN32
    char  eth[16];
#endif
    char  local_ip[16];
    UINT8 local_mac[6];
};

static struct oct_env env;

#ifdef WIN32
static int iotn_GetLocalIp(char *local_ip)
{
    WORD v = MAKEWORD(1, 1);
    WSADATA wsaData;
    WSAStartup(v, &wsaData);

    int i = 0;
    struct hostent *phostinfo = gethostbyname("");
    for (i = 0; NULL != phostinfo&& NULL != phostinfo->h_addr_list[i]; ++i)
    {
        char *pszAddr = inet_ntoa(*(struct in_addr *)phostinfo->h_addr_list[i]);
        if (octs_strlen(pszAddr) >= 7) {
            octs_strcpys(local_ip, SHORTSTRING, pszAddr);
            break;
        }
    }

    WSACleanup();
    return OCTOPUSHUB_E_SUCCESS;
}

static int iotn_GetLocalMac(char local_mac[6])
{
    PIP_ADAPTER_INFO pAdapterInfo;
    PIP_ADAPTER_INFO pAdapter = NULL;
    DWORD dwRetVal = 0;
    int retval = OCTOPUSHUB_E_FAILED;
    ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
    pAdapterInfo = (IP_ADAPTER_INFO *)OCTM_Malloc((UINT32)sizeof(IP_ADAPTER_INFO));
    if(pAdapterInfo == NULL) {
        leiot_error("malloc failed\n");
        return OCTOPUSHUB_E_FAILED;
    }
    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) != ERROR_SUCCESS)
    {
        OCTM_Free(pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO *)OCTM_Malloc((UINT32)ulOutBufLen);
        if(pAdapterInfo == NULL) {
            leiot_error("malloc failed\n");
            return OCTOPUSHUB_E_FAILED;
        }
    }
    if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR)
    {
        pAdapter = pAdapterInfo;
        while (pAdapter)
        {
            if (strcmp(pAdapter->IpAddressList.IpAddress.String, "0.0.0.0") != 0)
            {
                octs_memcpys(local_mac, MAXMACLENGTH, pAdapter->Address, pAdapter->AddressLength);
                retval = OCTOPUSHUB_E_SUCCESS;
                break;
            }
            pAdapter = pAdapter->Next;
        }
    }

    OCTM_Free(pAdapterInfo);
    return retval;
}

int IOTN_NetworkIsOk(void)
{
    return E_FAILED;
}

#else

int IOTN_GetLocalIp(char *eth, char *ipaddr)
{
    int sockfd;
    struct sockaddr_in *sin;
    struct ifreq ifr;
    int retval = E_FAILED;

    if(eth == NULL || ipaddr == NULL) {
        return E_FAILED;
    }

    if((sockfd=socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        leiot_error("socket create failed!\n");
        return E_FAILED;
    }
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, eth, sizeof(ifr.ifr_name) - 1);

    if(ioctl( sockfd, SIOCGIFADDR, &ifr) < 0 ) {
        leiot_error("%s ioctl ip failed!\n", eth);
        goto failed;
    }
    sin = (struct sockaddr_in *)&ifr.ifr_addr;
    iots_strcpy(ipaddr, inet_ntoa(sin->sin_addr));

    retval = E_SUCCESS;
failed:
    close(sockfd);
    return retval;
}

int IOTN_GetLocalMac(char *eth, UINT8 macaddr[6])
{
    int sockfd;
    struct ifreq ifr;
    int retval = E_FAILED;

    if(eth == NULL || macaddr == NULL) {
        return E_FAILED;
    }

    if((sockfd=socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        leiot_error("socket create failed!\n");
        return E_FAILED;
    }
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, eth, sizeof(ifr.ifr_name) - 1);

    if(macaddr != NULL) {
        if(ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0 ) {
            leiot_error("ioctl mac failed!\n");
            goto failed;
        }
        memcpy(macaddr, ifr.ifr_hwaddr.sa_data, 6);
    }
    retval = E_SUCCESS;
failed:
    close(sockfd);
    return retval;
}  

#if 0
int IOTN_NetworkIsOk(const char *iface)
{
    struct ethtool_value {
        __uint32_t      cmd;
        __uint32_t      data;
    }edata;
    int fd = -1, retval = E_FAILED;
    struct ifreq ifr;

    memset(&ifr, 0, sizeof(ifr));
    iots_strcpy(ifr.ifr_name, iface); 
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        leiot_error("Cannot creat socket, %s\n", strerror(errno));
        return -1;
    }

    edata.cmd = 0x0000000a;
    ifr.ifr_data = (caddr_t)&edata;
    retval = ioctl(fd, 0x8946, &ifr);
    if (retval == 0) {
        retval = edata.data?E_NETWORK_CONNECT:E_NETWORK_DISCONNECT;
    } else if (errno == 19) {
        retval = E_NO_DEVICE;
    } else {/*EOPNOTSUPP*/
        leiot_error("Cannot get network(%s) status, %s\n", iface, strerror(errno));
        retval = E_FAILED;
    }
    close(fd);
    return retval;
}
#else
int IOTN_NetworkIsOk(const char *if_name)
{
    int fd = -1;
    struct ifreq ifr;
    struct ifconf ifc;
    struct ifreq ifrs_buf[100];
    int if_number =0;
    int i;
    int retval = E_FAILED;

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        leiot_error("%s: socket error [%d] %s\n",if_name, errno, strerror(errno));
        return E_FAILED;
    }

    ifc.ifc_len = sizeof(ifrs_buf);
    ifc.ifc_buf = (caddr_t)ifrs_buf;
    if (ioctl(fd, SIOCGIFCONF, (char *)&ifc) <0) {
        leiot_error("%s: ioctl SIOCGIFCONF error [%d] %s\n",if_name, errno, strerror(errno));
        goto failed;
    }

    if_number = ifc.ifc_len / sizeof(struct ifreq);
    for(i=0; i< if_number; i++) {
        if(strcmp(if_name,ifrs_buf[i].ifr_name ) == 0) {
            break;
        }
    }

    if(i >= if_number) {
        leiot_debug2("%s DEVICE_NONE\n", if_name);
        retval = E_NO_DEVICE;
        goto failed;
    }

    bzero(&ifr, sizeof(ifr));
    strncpy(ifr.ifr_name, if_name, IFNAMSIZ-1);
    ifr.ifr_name[IFNAMSIZ-1] = 0;
    if (ioctl(fd, SIOCGIFFLAGS, (char *)&ifr) <0) {
        leiot_error("%s: ioctl SIOCGIFFLAGS error [%d] %s\n",if_name, errno, strerror(errno));
        goto failed;
    }  
#if 1     
    if(!(ifr.ifr_flags & IFF_UP))  {
        leiot_debug2("%s DEVICE_DOWN\n", if_name);
        retval = E_NETWORK_DISCONNECT;
        goto failed;
    }  

    if(!(ifr.ifr_flags & IFF_RUNNING)) {  
        leiot_debug2("%s DEVICE_UNPLUGGED\n", if_name);
        retval = E_NETWORK_DISCONNECT;
        goto failed;
    }  

    leiot_debug2("%s DEVICE_LINKED\n", if_name);
    retval = E_NETWORK_CONNECT;
failed:
    close(fd);
    return retval;  
#else  
{  
    struct ethtool_value edata;  
    if(!(ifr.ifr_flags & IFF_UP) || !(ifr.ifr_flags & IFF_RUNNING))  
    {  
        close(fd);
        fprintf(stderr, "%s: DOWN\r\n",if_name);
        return 1;  
    }  
    edata.cmd = ETHTOOL_GLINK;  
    edata.data = 0;  
    ifr.ifr_data = (char *) &edata;  
    if(ioctl( fd, SIOCETHTOOL, &ifr ) < 0)  
    {  
        fprintf(stderr, "%s: ioctl SIOCETHTOOL error [%d] %s\r\n",if_name, errno, strerror(errno));
        close(fd);
        return -1;   
    }  
  
    if(edata.data == 0)  
    {  
        fprintf(stderr, "DEVICE_UNPLUGGED\r\n");
        return 2;   
    }  
    else  
    {  
        fprintf(stderr, "DEVICE_LINKED\r\n");
        return 3;   
    }  
}  
#endif
}
#endif

#endif  //!WIN32

char *IOTN_LocalIP(void)
{
    log_assert(iots_strlen(env.eth) > 0);
    if(IOTN_GetLocalIp(env.eth, env.local_ip) != E_SUCCESS) {
        return NULL;
    }
    return env.local_ip;
}

UINT8 *IOTN_LocalMac(void)
{
    return env.local_mac;
}

int IOTN_Init(const char *eth)
{
    int retval = E_FAILED;
    memset(&env, 0, sizeof(env));
#ifdef WIN32
    if(IOTN_NetworkIsOk() != E_NETWORK_CONNECT) {
        leiot_error("Network is not connected\n");
        return E_FAILED;
    }

    if(Env_GetLocalIp(env.local_ip) != E_SUCCESS) {
        leiot_error("get local ip failed\n");
        goto failed;
    }

    if(Env_GetLocalMac(env.local_mac) != E_SUCCESS) {
        leiot_error("get local mac failed\n");
        goto failed;
    }
#else
    leiot_debug0("eth interface  = %s\n", eth);

    iots_strcpy(env.eth, eth);
    if(IOTN_NetworkIsOk(eth) != E_NETWORK_CONNECT) {
        leiot_error("Network is not connected. iface=%s\n", eth);
        return E_FAILED;
    }

    if(IOTN_GetLocalIp(env.eth, env.local_ip) != E_SUCCESS)
        goto failed;

    if(IOTN_GetLocalMac(env.eth, env.local_mac) != E_SUCCESS)
        goto failed;

#endif  //!WIN32

    leiot_debug0("local ip addr  = %s\n", env.local_ip);
    leiot_debug0("local mac addr = %02x:%02x:%02x:%02x:%02x:%02x\n",
           env.local_mac[0], env.local_mac[1], env.local_mac[2],
           env.local_mac[3], env.local_mac[4], env.local_mac[5]);

    retval = E_SUCCESS;
failed:
    return retval;
}

void IOTN_Exit(void)
{
}

void IOTN_Debug(char *ip, char *mac)
{
    if(ip != NULL) {
        iots_strcpy(env.local_ip, ip);
    }
    if(mac != NULL) {
        memcpy(env.local_mac, mac, 6);
    }
}

