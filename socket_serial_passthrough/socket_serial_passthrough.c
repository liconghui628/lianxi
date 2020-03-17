#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <errno.h>
#include <error.h>
#include <net/route.h>
#include <string.h>
#include <sys/time.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <pthread.h>

#define MEM_SIZE ((1024)*(10))
#define PORT_REMOTE_DEBUG (8881)

static char *g_read_buff = NULL;
static int conn_fd = -1;
static int g_run = 1;

//处理信号
static void remotedebug_signal_handle(int signo)
{
    printf("F:%s, f:%s, L:%d, catched signal%d\n", __FILE__, __func__, __LINE__,signo);
    switch(signo) {
        case SIGINT:
        case SIGTERM:
            g_run = 0;
            exit(1);
            break;
        default:
            break;
    }
}

//socket和serial间透传数据 sock_type: 1-TCP, 2-UDP
static int socket_serial_passthrough(int sock_fd, int serial_fd, int sock_type)
{
    int ret = -1;
    int epoll_fd = -1;
    int bytes = 0;
    int time_out = 5*60*1000; // 5 minutes epoll_wait() time out
    struct epoll_event events[2]; 
    struct sockaddr_in *src_addr = NULL;
    socklen_t addr_len = sizeof(struct sockaddr_in);

    if (sock_fd < 0 || serial_fd < 0 || (sock_type != 1 && sock_type != 2))
    {
        printf("F:%s, f:%s, L:%d, param error!\n", __FILE__, __func__, __LINE__);
        return -1;
    }
    printf("F:%s, f:%s, L:%d, sock_fd=%d, serial_fd=%d, sock_type=%d\n", __FILE__, __func__, __LINE__,sock_fd, serial_fd, sock_type);

    //fcntl O_NONBLOCK
    int flags;
    flags = fcntl(sock_fd, F_GETFL, NULL);
    if (flags < 0) 
        printf("F:%s, f:%s, L:%d, fcntl sock_fd F_GETFL failed\n", __FILE__, __func__, __LINE__);
    else
    {
        if (fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK) < 0)
            printf("F:%s, f:%s, L:%d, fcntl sock_fd F_SETFL O_NONBLOCK failed\n", __FILE__, __func__, __LINE__);
        else
            printf("F:%s, f:%s, L:%d, fcntl sock_fd F_SETFL O_NONBLOCK success\n", __FILE__, __func__, __LINE__);
    }
    flags = fcntl(serial_fd, F_GETFL, NULL);
    if (flags < 0) 
        printf("F:%s, f:%s, L:%d, fcntl serial_fd F_GETFL failed\n", __FILE__, __func__, __LINE__);
    else
    {
        if (fcntl(serial_fd, F_SETFL, flags | O_NONBLOCK) < 0)
            printf("F:%s, f:%s, L:%d, fcntl serial_fd F_SETFL O_NONBLOCK failed\n", __FILE__, __func__, __LINE__);
        else
            printf("F:%s, f:%s, L:%d, fcntl serial_fd F_SETFL O_NONBLOCK success\n", __FILE__, __func__, __LINE__);
    }

    //epoll create
    epoll_fd = epoll_create(2); 
    if(epoll_fd < 0)
    {
        printf("F:%s, f:%s, L:%d, epoll_create() failed: %s\n", __FILE__, __func__, __LINE__,strerror(errno));
        return -1;
    }

    //epoll add
    memset(&events[0], 0, sizeof(events[0]));
    events[0].events = EPOLLIN;
    events[0].data.fd = sock_fd;
    ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd, &events[0]);
    if (ret < 0)
    {
        printf("F:%s, f:%s, L:%d, epoll_ctl() sock_fd failed: %s\n", __FILE__, __func__, __LINE__,strerror(errno));
        goto failed;
    }
    memset(&events[1], 0, sizeof(events[1]));
    events[1].events = EPOLLIN;
    events[1].data.fd = serial_fd;
    ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, serial_fd, &events[1]);
    if (ret < 0)
    {
        printf("F:%s, f:%s, L:%d, epoll_ctl() serial_fd failed: %s\n", __FILE__, __func__, __LINE__,strerror(errno));
        goto failed;
    }

    while(1)
    {
        //epoll wait
        ret = epoll_wait(epoll_fd, events, 2, time_out);
        printf("F:%s, f:%s, L:%d, epoll_wait() return %d\n", __FILE__, __func__, __LINE__, ret);
        if(ret < 0) // error
        {
            printf("F:%s, f:%s, L:%d, epoll_wait() error： %s\n", __FILE__, __func__, __LINE__,strerror(errno));
            continue;
        }
        else if (ret == 0) //time out
        {
            printf("F:%s, f:%s, L:%d, epoll_wait() time out\n", __FILE__, __func__, __LINE__);
            goto failed;
        }
        else //data come
        {
            for(int i = 0; i < ret; i++) 
            {
                printf("F:%s, f:%s, L:%d, event.data.fd=%d\n", __FILE__, __func__, __LINE__, events[i].data.fd);
                memset(g_read_buff, 0, MEM_SIZE);
                if (events[i].data.fd == sock_fd)
                {
                    if(sock_type == 1) //tcp
                    {
                        bytes = read(sock_fd, g_read_buff, MEM_SIZE);
                        printf("F:%s, f:%s, L:%d, read from tcp sock_fd, read bytes=%d\n", __FILE__, __func__, __LINE__, bytes);
                    }
                    else if(sock_type == 2) //udp
                    {
                        if (src_addr == NULL)
                        {
                            //如果为UDP通讯必须保存源地址信息，如果源地址信息为NULL,则serial不能转发到UDP
                            src_addr = malloc(sizeof(struct sockaddr_in));
                            if(src_addr == NULL)
                            {
                                printf("F:%s, f:%s, L:%d, malloc struct sockaddr_in failed\n", __FILE__, __func__, __LINE__);
                                goto failed;
                            }
                            memset(src_addr, 0, sizeof(struct sockaddr_in));
                        }
                        bytes = recvfrom(sock_fd, g_read_buff, MEM_SIZE, 0, (struct sockaddr*)src_addr, &addr_len);
                        printf("F:%s, f:%s, L:%d, read from udp sock_fd, read bytes=%d\n", __FILE__, __func__, __LINE__, bytes);
                    }
                    //write to serial_fd
                    if(bytes <= 0)
                    {
                        goto failed;
                    }
                    else
                    {
                        bytes = write(serial_fd, g_read_buff, bytes);
                        printf("F:%s, f:%s, L:%d, write to serial_fd bytes=%d\n", __FILE__, __func__, __LINE__, bytes);
                    }

                }
                else if(events[i].data.fd == serial_fd) //serial
                {
                    bytes = read(serial_fd, g_read_buff, MEM_SIZE);
                    printf("F:%s, f:%s, L:%d, read from serial_fd, read bytes=%d\n", __FILE__, __func__, __LINE__, bytes);
                    //write to sock_fd
                    if(bytes <= 0)
                    {
                        goto failed;
                    }
                    else
                    {
                        if(sock_type == 1)
                        {
                            bytes = write(sock_fd, g_read_buff, bytes);
                            printf("F:%s, f:%s, L:%d, write to tcp sock_fd bytes=%d\n", __FILE__, __func__, __LINE__, bytes);
                        }
                        else if(sock_fd == 2)
                        {
                            //如果serial数据转发到UDP,则必须知道目的地址
                            if(src_addr == NULL)
                            {
                                printf("F:%s, f:%s, L:%d, UDP src_addr is NULL! ignore this msg\n", __FILE__, __func__, __LINE__);
                            }
                            else
                            {
                                bytes = sendto(sock_fd, g_read_buff, bytes, 0, (struct sockaddr*)src_addr, addr_len);
                                printf("F:%s, f:%s, L:%d, write to udp sock_fd bytes=%d\n", __FILE__, __func__, __LINE__, bytes);
                            }
                        }
                    }
                }
            }
        }
    }

failed:
    if (epoll_fd > 0)
        close(epoll_fd);
    return ret;
}

static int serial_open_with_params(char *device_name, int baud, int data_bits, int stop_bits, char parity)
{
    int ret = -1;
    int serial_fd = -1;
    unsigned int speed = B9600;
    struct termios tios;

    if (device_name == NULL)
    {
        printf("F:%s, f:%s, L:%d, device_name is NULL\n", __FILE__, __func__, __LINE__);
        return ret;
    }
    printf("F:%s, f:%s, L:%d, device_name=%s, baud=%u,data_bits=%d,stop_bits=%d, parity=%c\n", 
            __FILE__, __func__, __LINE__, device_name, baud, data_bits, stop_bits, parity);

    serial_fd = open(device_name, O_RDWR | O_NOCTTY | O_NDELAY | O_EXCL);
    if(serial_fd < 0)
    {
        printf("F:%s, f:%s, L:%d, open %s failed: %s\n", __FILE__, __func__, __LINE__,device_name, strerror(errno));
        return ret;
    }

    memset(&tios, 0, sizeof(struct termios));

    //baud
    switch (baud) {
    case 110:
        speed = B110;
        break;
    case 300:
        speed = B300;
        break;
    case 600:
        speed = B600;
        break;
    case 1200:
        speed = B1200;
        break;
    case 2400:
        speed = B2400;
        break;
    case 4800:
        speed = B4800;
        break;
    case 9600:
        speed = B9600;
        break;
    case 19200:
        speed = B19200;
        break;
    case 38400:
        speed = B38400;
        break;
    case 57600:
        speed = B57600;
        break;
    case 115200:
        speed = B115200;
        break;
    default:
        speed = B9600;
        printf("F:%s, f:%s, L:%d, unsupported baud(%d) use default(9600)\n", __FILE__, __func__, __LINE__,baud);
    }
    if ((cfsetispeed(&tios, speed) < 0) || (cfsetospeed(&tios, speed) < 0)) 
    {
        printf("F:%s, f:%s, L:%d, cfsetispeed() failed\n", __FILE__, __func__, __LINE__);
        return ret;
    }
    /* C_CFLAG      Control options
       CLOCAL       Local line - do not change "owner" of port
       CREAD        Enable receiver
    */
    tios.c_cflag |= (CREAD | CLOCAL);
    /* CSIZE, HUPCL, CRTSCTS (hardware flow control) */

    /* Set data bits (5, 6, 7, 8 bits)
       CSIZE        Bit mask for data bits
    */
    tios.c_cflag &= ~CSIZE;
    switch (data_bits) {
    case 5:
        tios.c_cflag |= CS5;
        break;
    case 6:
        tios.c_cflag |= CS6;
        break;
    case 7:
        tios.c_cflag |= CS7;
        break;
    case 8:
    default:
        tios.c_cflag |= CS8;
        break;
    }

    /* Stop bit (1 or 2) */
    if (stop_bits == 1)
        tios.c_cflag &= ~CSTOPB;
    else /* 2 */
        tios.c_cflag |= CSTOPB;

    /* PARENB       Enable parity bit
       PARODD       Use odd parity instead of even */
    if (parity == 'N' || parity == 'n') {
        /* None */
        tios.c_cflag &= ~PARENB;
    } else if (parity == 'E' || parity == 'e') {
        /* Even */
        tios.c_cflag |= PARENB;
        tios.c_cflag &= ~PARODD;
    } else {
        /* Odd */
        tios.c_cflag |= PARENB;
        tios.c_cflag |= PARODD;
    }

    /* Read the man page of termios if you need more information. */

    /* This field isn't used on POSIX systems
       tios.c_line = 0;
    */

    /* C_LFLAG      Line options

       ISIG Enable SIGINTR, SIGSUSP, SIGDSUSP, and SIGQUIT signals
       ICANON       Enable canonical input (else raw)
       XCASE        Map uppercase \lowercase (obsolete)
       ECHO Enable echoing of input characters
       ECHOE        Echo erase character as BS-SP-BS
       ECHOK        Echo NL after kill character
       ECHONL       Echo NL
       NOFLSH       Disable flushing of input buffers after
       interrupt or quit characters
       IEXTEN       Enable extended functions
       ECHOCTL      Echo control characters as ^char and delete as ~?
       ECHOPRT      Echo erased character as character erased
       ECHOKE       BS-SP-BS entire line on line kill
       FLUSHO       Output being flushed
       PENDIN       Retype pending input at next read or input char
       TOSTOP       Send SIGTTOU for background output

       Canonical input is line-oriented. Input characters are put
       into a buffer which can be edited interactively by the user
       until a CR (carriage return) or LF (line feed) character is
       received.

       Raw input is unprocessed. Input characters are passed
       through exactly as they are received, when they are
       received. Generally you'll deselect the ICANON, ECHO,
       ECHOE, and ISIG options when using raw input
    */

    /* Raw input */
    tios.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    /* C_IFLAG      Input options

       Constant     Description
       INPCK        Enable parity check
       IGNPAR       Ignore parity errors
       PARMRK       Mark parity errors
       ISTRIP       Strip parity bits
       IXON Enable software flow control (outgoing)
       IXOFF        Enable software flow control (incoming)
       IXANY        Allow any character to start flow again
       IGNBRK       Ignore break condition
       BRKINT       Send a SIGINT when a break condition is detected
       INLCR        Map NL to CR
       IGNCR        Ignore CR
       ICRNL        Map CR to NL
       IUCLC        Map uppercase to lowercase
       IMAXBEL      Echo BEL on input line too long
    */
    if (parity == 'N'|| parity == 'n') {
        /* None */
        tios.c_iflag &= ~INPCK;
    } else {
        tios.c_iflag |= INPCK;
    }

    /* Software flow control is disabled */
    tios.c_iflag &= ~(IXON | IXOFF | IXANY);

    /* C_OFLAG      Output options
       OPOST        Postprocess output (not set = raw output)
       ONLCR        Map NL to CR-NL

       ONCLR ant others needs OPOST to be enabled
    */

    /* Raw ouput */
    tios.c_oflag &= ~OPOST;

    /* C_CC         Control characters
       VMIN         Minimum number of characters to read
       VTIME        Time to wait for data (tenths of seconds)

       UNIX serial interface drivers provide the ability to
       specify character and packet timeouts. Two elements of the
       c_cc array are used for timeouts: VMIN and VTIME. Timeouts
       are ignored in canonical input mode or when the NDELAY
       option is set on the file via open or fcntl.

       VMIN specifies the minimum number of characters to read. If
       it is set to 0, then the VTIME value specifies the time to
       wait for every character read. Note that this does not mean
       that a read call for N bytes will wait for N characters to
       come in. Rather, the timeout will apply to the first
       character and the read call will return the number of
       characters immediately available (up to the number you
       request).

       If VMIN is non-zero, VTIME specifies the time to wait for
       the first character read. If a character is read within the
       time given, any read will block (wait) until all VMIN
       characters are read. That is, once the first character is
       read, the serial interface driver expects to receive an
       entire packet of characters (VMIN bytes total). If no
       character is read within the time allowed, then the call to
       read returns 0. This method allows you to tell the serial
       driver you need exactly N bytes and any read call will
       return 0 or N bytes. However, the timeout only applies to
       the first character read, so if for some reason the driver
       misses one character inside the N byte packet then the read
       call could block forever waiting for additional input
       characters.

       VTIME specifies the amount of time to wait for incoming
       characters in tenths of seconds. If VTIME is set to 0 (the
       default), reads will block (wait) indefinitely unless the
       NDELAY option is set on the port with open or fcntl.
    */
    /* Unused because we use open with the NDELAY option */
    tios.c_cc[VMIN] = 0;
    tios.c_cc[VTIME] = 0;

    if (tcsetattr(serial_fd, TCSANOW, &tios) < 0) {
        printf("F:%s, f:%s, L:%d, tcsetattr() failed\n", __FILE__, __func__, __LINE__);
        return ret;
    }

    ret = serial_fd;
    return ret;
}

//远程调试线程
static void*  thread_remote_debug(void *arg)
{
    printf("F:%s, f:%s, L:%d, start\n", __FILE__, __func__, __LINE__);

    int ret = -1;
    int sock_fd = -1;
    int serial_fd = -1;
    int optval = 1;
    struct sockaddr_in serv_addr;
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size;

    //create socket
    sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock_fd < 0)
    {
        printf("F:%s, f:%s, L:%d, socket() failed:%s\n", __FILE__, __func__, __LINE__,strerror(errno));
        goto failed;
    }
    ret = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));
    if (ret < 0)
    {
        printf("F:%s, f:%s, L:%d, setsockopt() failed:%s\n", __FILE__, __func__, __LINE__,strerror(errno));
        goto failed;
    }
    //bind
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORT_REMOTE_DEBUG);
    ret = bind(sock_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if (ret < 0)
    {
        printf("F:%s, f:%s, L:%d, bind() failed:%s\n", __FILE__, __func__, __LINE__,strerror(errno));
        goto failed;
    }
    //listen
    ret = listen(sock_fd, 1);
    if (ret < 0)
    {
        printf("F:%s, f:%s, L:%d, listen() failed:%s\n", __FILE__, __func__, __LINE__,strerror(errno));
        goto failed;
    }
    conn_fd = -1;
    while(g_run)
    {
        //accept
        printf("F:%s, f:%s, L:%d, Wait for client connect ... \n", __FILE__, __func__, __LINE__);
        clnt_addr_size = sizeof(clnt_addr);
        if(conn_fd < 0)
        {
            conn_fd = accept(sock_fd, (struct sockaddr*)&clnt_addr, &clnt_addr_size); 
            if (conn_fd < 0)
            {
                printf("F:%s, f:%s, L:%d, accept() failed:%s\n", __FILE__, __func__, __LINE__,strerror(errno));
                continue;
            }
        }
        printf("F:%s, f:%s, L:%d, accept() success; IP=%s \n", __FILE__, __func__, __LINE__,inet_ntoa(clnt_addr.sin_addr));
        //串口参数信息（端口、波特率、数据位、停止位、校验位）
        memset(g_read_buff, 0, MEM_SIZE);
        if (read(conn_fd, g_read_buff, MEM_SIZE) <= 0)
        {
            printf("F:%s, f:%s, L:%d, read failed\n", __FILE__, __func__, __LINE__);
            close(conn_fd);
            conn_fd = -1;
            continue;
        }
        printf("F:%s, f:%s, L:%d, READ serial params:%s\n", __FILE__, __func__, __LINE__,g_read_buff);
        char parity;
        //char *device_name = "/dev/ttyO2";
        char *device_name = "/dev/ttyUSB0";
        int baud, data_bits, stop_bits;
        sscanf(g_read_buff, "%d;%d;%d;%c", &baud, &data_bits,&stop_bits,&parity);
        printf("F:%s, f:%s, L:%d, device_name=%s, baud=%d,data_bits=%d,stop_bits=%d, parity=%c\n", 
                __FILE__, __func__, __LINE__, device_name, baud, data_bits, stop_bits, parity);

        //打开串口
        serial_fd = serial_open_with_params(device_name, baud, data_bits, stop_bits, parity);
        if (serial_fd < 0)
        {
            printf("F:%s, f:%s, L:%d, serial_open_with_params() failed\n", __FILE__, __func__, __LINE__);
            continue;
        }
        else 
        {
            printf("F:%s, f:%s, L:%d, open serial %s success\n", __FILE__, __func__, __LINE__,g_read_buff);
        }

        //进入透传模式
        printf("F:%s, f:%s, L:%d, Enter passthrough mode!\n", __FILE__, __func__, __LINE__);
        socket_serial_passthrough(conn_fd, serial_fd, 1);
        printf("F:%s, f:%s, L:%d, Exit passthrough mode!\n", __FILE__, __func__, __LINE__);
        if(sock_fd > 0)
            close(sock_fd);
        if(serial_fd > 0)
            close(serial_fd);
    }

failed:
    if (sock_fd > 0)
        close(sock_fd);
    return NULL;
}

int main(int argc, char *argv[])
{
    int rc;
    pthread_t t1;
    int run_in_front = 0;

    // set signal handle
    signal(SIGTERM, remotedebug_signal_handle);
    signal(SIGINT,  remotedebug_signal_handle);

    //申请一块缓冲区
    g_read_buff = malloc(MEM_SIZE);
    if(g_read_buff == NULL)
    {
        printf("F:%s, f:%s, L:%d, malloc error\n", __FILE__, __func__, __LINE__);
        exit(-1);
    }

    //本地指令socket线程
	if (pthread_create(&t1, NULL, thread_remote_debug, NULL)!=0){
        printf("F:%s, f:%s, L:%d,create thread thread_remote_debug fail\n", __FILE__, __func__, __LINE__);
		exit(-1);
	}

    pthread_join(t1, NULL);

	return 0;
}
