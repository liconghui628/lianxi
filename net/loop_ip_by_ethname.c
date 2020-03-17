static void get_local_ip(char *buff, int buff_size)
{
    int fd, i;
    struct ifreq buf[16];
    struct ifconf ifc;

    memset(buf, 0, sizeof(buf));
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = (caddr_t)buf;

    if((fd = socket(AF_INET, SOCK_DGRAM, 0)) != -1){
        if (ioctl(fd, SIOCGIFCONF, (char *)&ifc) != -1){
            for(i = ifc.ifc_len/sizeof(struct ifreq); i > 0; i--){
                if (ioctl(fd, SIOCGIFADDR, (char *)&buf[i]) != -1) {
                    char *eth = buf[i].ifr_name;
                    char *ip = (char *)inet_ntoa(((struct sockaddr_in *)&(buf[i].ifr_addr))->sin_addr);
	                log_debug0("F:%s, f:%s, L:%d, eth_name:%s, eth_ip:%s\n", __FILE__, __func__, __LINE__, eth, ip);
                    if (strcmp(ip, "127.0.0.1") != 0) {
                        iots_strcpys(buff, buff_size, (char *)inet_ntoa(((struct sockaddr_in *)&(buf[i].ifr_addr))->sin_addr));
                        break;
                    }
                }
            }       
        }            
    }
}

