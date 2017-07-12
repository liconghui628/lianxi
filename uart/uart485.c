#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/select.h>
#include <string.h>
#include <errno.h>

#define UART_NAME	"/dev/ttyS1"

static int at_request(int fd, char *msg)
{
	int writecnt = 0;
	if(fd < 0 || !msg){
		printf("%s():param error!",__func__);
		return -1;
	}
	printf("[send]:%s\n",msg);
	writecnt = write(fd, msg, strlen(msg));
	//printf("writecnt = %d\n",writecnt);
	return writecnt;
}

static int at_response(int fd, char *buff, int size)
{
	int readcnt = 0, rc = 0;
	fd_set readfds;
	struct timeval tv;

	if(fd < 0 || size <= 0 || !buff){
		printf("%s():param error!",__func__);
		return -1;
	}

	memset(buff, 0, size);
	FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
	tv.tv_sec = 1;
    tv.tv_usec = 0;
	//printf("select()...\n");
	while( rc = select(fd+1, &readfds, NULL, NULL, &tv) && readcnt < size-1){
		readcnt += read(fd, &buff[readcnt], size-readcnt-1);
		//printf("while():rc = %d readcnt = %d\n",rc,readcnt);
		FD_ZERO(&readfds);
    	FD_SET(fd, &readfds);
	}
	//printf("rc = %d, readcnt = %d\n",rc, readcnt);
	printf("[recv]:%s\n",buff);
	return readcnt;
}

static int set_termio(int fd, int speed , int databits,int stopbits,int parity)
{
    unsigned i;
    struct termios options;
    int speed_arr[] = {B115200, B57600, B38400, B19200, B9600, B4800, B2400, B1200, B600, B300};
    int name_arr[] = {115200, 57600, 38400, 19200, 9600, 4800, 2400, 1200, 600, 300};

    //得到当前串口的参数
    if(fd <= 0 || tcgetattr(fd, &options) != 0) {
        perror("tcgetattr");
        return -1;
    }
	//memset(&options, 0, sizeof(struct termios));

    for(i= 0; i < sizeof(speed_arr)/sizeof(speed_arr[0]); i++) {
        if(speed == name_arr[i]) {
            tcflush(fd, TCIOFLUSH);
            if(cfsetispeed(&options, speed_arr[i]) < 0 ||cfsetospeed(&options, speed_arr[i]) < 0)
				return -1;
            break;
        }
    }
	printf("%d speed set\n",name_arr[i]);


    //设置字符大小
    options.c_cflag &= ~CSIZE;
    switch(databits)
    {
    case 7:
        options.c_cflag |= CS7;
        break;
    case 8:
		printf("8 bits data\n");
        options.c_cflag |= CS8;
        break;
    default:
        fprintf(stderr,"Unsupported data size\n");
        return -1;
    }

    //设置奇偶校验位
    switch(parity)
    {
	//无奇偶校验位
    case 'n':
    case 'N':
        //Clear parity enable
		printf("N parity\n");
        options.c_cflag &= ~PARENB;
        options.c_iflag &= ~INPCK;
        break;
    case 'o':
    case 'O':
        //设置为奇效验
        options.c_cflag |= (PARODD | PARENB);
        //INPCK:奇偶校验使能
        options.c_iflag |= INPCK;
        break;
    case 'e':
    case 'E':
        //Enable parity
        options.c_cflag |= PARENB;
        //转换为偶效验
        options.c_cflag &= ~PARODD;
        //Disnable parity checking
        options.c_iflag |= INPCK;
        break;
    case 'S':
    case 's':
        //Space 校验 as no parity
        options.c_cflag &= ~PARENB;
        options.c_cflag &= ~CSTOPB;
        options.c_iflag |= INPCK;
        break;
    default:
        fprintf(stderr,"Unsupported parity\n");
        return -1;
    }

    /*if (parity == 'N') {
        options.c_iflag &= ~INPCK;
    } else {
        options.c_iflag |= INPCK;
    }*/

    //设置停止位
    switch(stopbits)
    {
    case 1:
		printf("1 bit stop\n");
        options.c_cflag &= ~CSTOPB;
        //1个停止位
        break;
    case 2:
        options.c_cflag |= CSTOPB;
        //2个停止位
        break;
    default:
        fprintf(stderr,"Unsupported stop bits\n");
        return -1;
    }

    //使能接收并使能本地状态
    options.c_cflag |= (CLOCAL | CREAD);
    //原始数据输入
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    // 禁止软件流控 
    options.c_iflag &= ~(IXON | IXOFF | IXANY);
	//设置原始输出
    options.c_oflag &= ~(OPOST);
	options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 0;
    //处理未接收的字符
    tcflush(fd,TCIFLUSH);
    //激活新配置
    if(tcsetattr(fd, TCSANOW, &options) != 0) {
        perror("tcsetattr");
        return -1;
    }
    return 0;
}

int main()
{
	int fd;
	char buff[50] = {0};

	fd = open(UART_NAME, O_RDWR | O_NOCTTY | O_NDELAY | O_EXCL);
	if(fd < 0){
		perror("open");
		return -1;
	}
	
    if(set_termio(fd, 9600, 8, 1, 'N') < 0)
        return -1;
	
	while(1){
		at_request(fd, "hello");
		at_response(fd, buff,sizeof(buff));
	}
	close(fd);
	return 0;
}
