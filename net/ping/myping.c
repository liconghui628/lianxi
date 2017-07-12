#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <setjmp.h>
#include <errno.h>
#define PACKET_SIZE     4096
#define MAX_WAIT_TIME   5
int MAX_NO_PACKETS = 5;
char sendpacket[PACKET_SIZE];
char recvpacket[PACKET_SIZE];
int sockfd,datalen=56;
int nsend=0,nreceived=0;
struct sockaddr_in dest_addr;
pid_t pid;
struct sockaddr_in from;
struct timeval tvrecv;
unsigned short cal_chksum(unsigned short *addr,int len);
int pack(int pack_no);
void send_packet(void);
void recv_packet(void);
int unpack(char *buf,int len);
void tv_sub(struct timeval *out,struct timeval *in);

/*  */
unsigned short cal_chksum(unsigned short *addr,int len)
{       int nleft=len;
        int sum=0;
        unsigned short *w=addr;
        unsigned short answer=0;
		
        while(nleft>1)
        {       sum+=*w++;
                nleft-=2;
        }
        if( nleft==1)
        {       *(unsigned char *)(&answer)=*(unsigned char *)w;
                sum+=answer;
        }
        sum=(sum>>16)+(sum&0xffff);
        sum+=(sum>>16);
        answer=~sum;
        return answer;
}
/*  */
int pack(int pack_no)
{       int i,packsize;
        struct icmp *icmp;
        struct timeval *tval;
        icmp=(struct icmp*)sendpacket;
        icmp->icmp_type=ICMP_ECHO;
        icmp->icmp_code=0;
        icmp->icmp_cksum=0;
        icmp->icmp_seq=pack_no;
        icmp->icmp_id=pid;
        packsize=8+datalen;
        tval= (struct timeval *)icmp->icmp_data;
        gettimeofday(tval,NULL);    
        icmp->icmp_cksum=cal_chksum( (unsigned short *)icmp,packsize); 
        return packsize;
}
/*  */
void send_packet()
{       int packetsize;
        nsend++;
        packetsize=pack(nsend); 
        if( sendto(sockfd,sendpacket,packetsize,0,(struct sockaddr *)&dest_addr,sizeof(dest_addr) )<0  )
               perror("sendto error");
}
/*  */
void recv_packet()
{       int n,fromlen;
        extern int errno;
        fromlen=sizeof(from);
        if( (n=recvfrom(sockfd,recvpacket,sizeof(recvpacket),0,(struct sockaddr *)&from,&fromlen)) <= 0)
        {   
			printf("No msg recived !\n");
		}
        gettimeofday(&tvrecv,NULL);  
        unpack(recvpacket,n);
}
/*  */
int unpack(char *buf,int len)
{       int i,iphdrlen;
        struct ip *ip;
        struct icmp *icmp;
        struct timeval *tvsend;
        double rtt;
        ip=(struct ip *)buf;
        iphdrlen=ip->ip_hl<<2;    
        icmp=(struct icmp *)(buf+iphdrlen);  
        len-=iphdrlen;            
        if( len<8)                
        {       printf("ICMP packets\'s length is less than 8\n");
                return -1;
        }
        /*  */
		//printf("icmp_type = %d, id = %d),pid = %d\n",icmp->icmp_type,icmp->icmp_id,pid);
        if(icmp->icmp_type==ICMP_DEST_UNREACH)
			printf("Dest unreach !\n");
        else if( (icmp->icmp_type==ICMP_ECHOREPLY) && (icmp->icmp_id==pid) )
        {       
			tvsend=(struct timeval *)icmp->icmp_data;
            tv_sub(&tvrecv,tvsend);  
            rtt=(double)tvrecv.tv_sec*1000+(double)tvrecv.tv_usec/1000;  
            /*  */
            printf("%d byte from %s: icmp_seq=%u ttl=%d rtt=%.3f ms\n",
                    len,inet_ntoa(from.sin_addr),icmp->icmp_seq,ip->ip_ttl,rtt);
        }
        else    
			return -1;
}

main(int argc,char *argv[])
{       struct hostent *host;
        struct protoent *protocol;
		struct timeval tv;
        unsigned long inaddr=0l;
        int waittime=MAX_WAIT_TIME;
        int size=50*1024;
        if(argc<2)
        {       printf("usage:%s hostname/IP address\n",argv[0]);
                exit(1);
        }
        if( (protocol=getprotobyname("icmp") )==NULL)
        {       perror("getprotobyname");
                exit(1);
        }
        /*  */
        if( (sockfd=socket(AF_INET,SOCK_RAW,protocol->p_proto) )<0)
        {       perror("socket error");
                exit(1);
        }
        /* */
		tv.tv_sec = 5;
		tv.tv_usec = 0;
        if(setuid(getuid()))
			printf("setuid %d error\n",getuid());
        setsockopt(sockfd,SOL_SOCKET,SO_RCVBUF,&size,sizeof(size) );
		setsockopt(sockfd,SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof(struct timeval));
        setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(struct timeval) );
        bzero(&dest_addr,sizeof(dest_addr));
        dest_addr.sin_family=AF_INET;
        /*  */
		
       	dest_addr.sin_addr.s_addr = inet_addr(argv[1]); 

        pid=getpid();
        printf("\nPING %s(%s): %d bytes data in ICMP packets.\n",argv[1],
                        inet_ntoa(dest_addr.sin_addr),datalen);
		while( MAX_NO_PACKETS-- )
		{
        	send_packet(); 
       		recv_packet();  
			sleep(1);
		}
        return 0;
}
/*  */
void tv_sub(struct timeval *out,struct timeval *in)
{  
	//printf("tv_usec = %d, tv_usec2 = %d,tv_sec = %d,tv_sec2 = %d\n",out->tv_usec,in->tv_usec,out->tv_sec,in->tv_sec);
	if( (out->tv_usec-=in->tv_usec)<0)
    {       
		--out->tv_sec;
        out->tv_usec+=1000000;
    }
    out->tv_sec-=in->tv_sec;
}
