/************************************************* 
 * 这个程序实现了TFTP客户端，在Linux上编译通过并 *
 * 正常使用。使用socket编程，具体的TFTP协议由于 *
 * 篇幅太长，这里没有列出来，可以参考网址:         *
 * http://www.faqs.org/rfcs/rfc1350.html         *
 ************************************************/ 
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>

//下面定义的几个宏是TFTP协议的操作码，参考协议

#define OP_RRQ        1 //读请求

#define OP_WRQ        2 //写请求

#define OP_DATA        3 //数据包

#define OP_ACK        4 //确认包

#define OP_ERROR    5 //错误信息


//TFTP错误信息编码

static char *error_string[] = {
    "Not defined error!",                    //0

    "File not found!",                        //1

    "Access denied!",                        //2

    "Disk full or allocation exceeded!",    //3

    "Illegal TFTP operation!",                //4

    "Unknown transfer ID!",                    //5

    "File already exists!",                    //6

    "No such user!"                            //7

};

int main(int argc, char **argv)
{
    int socketFD; //socket描述符

    struct sockaddr_in tftpServer, tftpClient; //自身和服务器的socket信息

    FILE *fp; //用于保存本地文件

    char buffer[516]; //网络通信用的buffer

    int addrlen;
    int lastPacket, currentPacket; //包计数

    unsigned short *s_ptr; 
    char *c_ptr;
    int tmp;
    int ret;

    if (argc < 3)
    {
        //使用方式

        printf("Usage: %s <ipaddr> <filename>\n", argv[0]);
        return 0;
    }

    //创建socket

    socketFD = socket(AF_INET, SOCK_DGRAM, 0);
    if (-1 == socketFD)
    {
        printf("Can not create socket!\n");
        return 0;
    }

    //客户端自身信息

    bzero(&tftpClient, sizeof (tftpClient));
    tftpClient.sin_family = AF_INET;
    tftpClient.sin_addr.s_addr = INADDR_ANY;
    tftpClient.sin_port = htons(0);

    //绑定socket到本机IP地址, 可以不绑定，不绑定则由系统自己决定使用的接口和源端口

    tmp = bind(socketFD, (struct sockaddr *)&tftpClient, sizeof (tftpClient));
    if (0 != tmp)
    {
        printf("Can not bind socket!\n");
        goto error_1;
    }

    //服务器信息

    bzero(&tftpServer, sizeof (tftpServer));
    addrlen = sizeof (tftpServer);
    tftpServer.sin_family = AF_INET;
    tftpServer.sin_addr.s_addr = inet_addr(argv[1]);
    tftpServer.sin_port = htons(69); //TFTP服务端口


    s_ptr = (unsigned short *)&buffer[0];
    *s_ptr = htons(OP_RRQ); //操作码

    c_ptr = &buffer[2]; 

    strcpy(&buffer[2], argv[2]); //远程文件名

    c_ptr += strlen(argv[2]);
    *c_ptr++ = 0;
    strcpy(c_ptr, "netascii"); //模式

    c_ptr += 8;
    *c_ptr++ = 0;

    //创建一个本地文件

    fp = fopen(argv[2], "w");
    if (NULL == fp)
    {
        printf("Can not create locale file!\n");
        goto error_1;
    }

    lastPacket = 0;

    while (1)
    {
        //发送请求或者回应包到服务器

        tmp = sendto(socketFD, buffer, c_ptr - &buffer[0], 0, 
                    (struct sockaddr *)&tftpServer, sizeof (tftpServer));
        if (-1 == tmp)
        {
            printf("Can not send buffer!\n");
            goto error_2;
            return 0;
        }

        //从服务器获取数据包

        tmp = recvfrom(socketFD, buffer, 516, 0, (struct sockaddr *)&tftpServer, &addrlen);
        if (-1 == tmp)
        {
            printf("Receive data error!\n");
            goto error_2;
            return 0;
        }
        else
        {
            switch (ntohs(*s_ptr))
            {
                //数据包

                case OP_DATA:
                    currentPacket = ntohs(*(s_ptr+1));
                    //验证包的顺序

                    if ((lastPacket + 1) != currentPacket)
                    {
                        printf("ERROR: packet error!\n");
                        goto error_2;
                    }

                    lastPacket = currentPacket;

                    tmp -= 4;
                    //写入数据到本地文件

                    ret = fwrite(&buffer[4], 1, tmp, fp);
                    if (tmp != ret)
                    {
                        printf("ERROR: write to local file error!\n");
                        goto error_2;
                    }

                    //如果等于512则不是最后一个包

                    if (512 != tmp)
                    {
                        goto read_ok;
                    }

                    //准备返回信息

                    *s_ptr = htons(OP_ACK);
                    *(s_ptr + 1) = htons(currentPacket);
                    c_ptr = &buffer[4];

                    break;

                //服务器返回错误信息

                case OP_ERROR:
                    //根据错误码，打印错误信息

                    printf("ERROR: %s\n", error_string[ntohs(*(s_ptr+1))]);
                    goto error_2;
                    break;

                //发生其他未知错误

                default:
                    printf("ERROR: Unknow error!\n");
                    goto error_2;
                    break;
            }
        }
    }

read_ok:
    fclose(fp);
    close(socketFD);
    return 0;

error_2:
    fclose(fp);
    unlink(argv[2]);
error_1:
    close(socketFD);
    
    return 0;
}


