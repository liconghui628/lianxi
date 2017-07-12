#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFSIZE             128
#define NUM_PER_TASK        2 
#define MAX_WAIT_QUEUE      10
#define PERROR(arg) do { perror(arg); exit(-1); } while(0)
#define LEN (sizeof(MSG) - sizeof(long))

int deal_recv(int connfd, char buf[])
{
    assert(buf != NULL);
    printf("deal_recv connfd=%d\n", connfd);
    send(connfd, buf, BUFSIZE, 0);

    return 0;
}

void handler_zombie(int signum)
{
    while(waitpid(-1, NULL, WNOHANG) != 0);
}

int deal_client(fd_set bakfs, int maxfd);
fd_set parent_process(int sockfd, int *pmaxfd);
int parent_fork(int sockfd, fd_set bakfs, int maxfd);
int main(int argc, char *argv[])
{
    fd_set bakfs;
    int sockfd, maxfd;
    struct sockaddr_in myaddr;

    printf("server started...\n");
    if((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) PERROR("socket");
    bzero(&myaddr, sizeof(myaddr));
    myaddr.sin_family = PF_INET;
    myaddr.sin_port = htons(atoi(argv[2]));
    myaddr.sin_addr.s_addr = inet_addr(argv[1]);
    if(bind(sockfd, (struct sockaddr *)&myaddr, sizeof(myaddr)) == -1) PERROR("bind");
    if(listen(sockfd, 10) == -1) PERROR("listen");

    bakfs = parent_process(sockfd, &maxfd);
    FD_CLR(sockfd, &bakfs);
    parent_fork(sockfd, bakfs, maxfd);

    return 0;
}

fd_set parent_process(int sockfd, int *pmaxfd)
{
    char buf[BUFSIZE];
    socklen_t addrlen;
    fd_set rdfs, bakfs;
    struct sockaddr_in peeraddr;
    int i, maxfd, connfd, num = 0, cnt = 0;

    maxfd = sockfd;
    FD_ZERO(&bakfs);
    FD_SET(sockfd, &bakfs);
    bzero(&peeraddr, sizeof(peeraddr));
    addrlen = sizeof(peeraddr);

    signal(SIGCHLD, handler_zombie);
    while(1) {
        rdfs = bakfs;
        if(select(maxfd + 1, &rdfs, NULL, NULL, NULL) == -1) PERROR("select");
        printf("one cycle\n");
        for(i = 0; i <= maxfd; i++) {
            if(FD_ISSET(i, &rdfs)) {
                if(i == sockfd) {
                    if((connfd = accept(sockfd, (struct sockaddr *)&peeraddr, &addrlen)) == -1) PERROR("accept");
                    if(connfd > maxfd) maxfd = connfd;
                    FD_SET(connfd, &bakfs);
                    printf("connfd=%d pid=%d\n", connfd, getpid());
                    if(++num >= NUM_PER_TASK) goto exit;
                } else {
                    cnt = recv(i, buf, BUFSIZE, 0);
                    if(cnt <= 0) FD_CLR(i, &bakfs);

                    printf("connfd=%d pid=%d\n", i, getpid());
                    deal_recv(i, buf);
                }
            }
        }
    }
exit:
    *pmaxfd = maxfd;
    printf("exit parent_process\n");

    return bakfs;
}

int parent_fork(int sockfd, fd_set bakfs, int maxfd)
{
    pid_t pid;
    int tmpfd;

    if((pid = fork()) == -1) PERROR("fork");
    if(pid == 0) {
        bakfs = parent_process(sockfd, &tmpfd);
        FD_CLR(sockfd, &bakfs);
        parent_fork(sockfd, bakfs, tmpfd);        
    } else {
        close(sockfd);
        deal_client(bakfs, maxfd);
    }

    return 0;
}

int deal_client(fd_set bakfs, int maxfd)
{
    fd_set rdfs;
    char buf[BUFSIZE];
    int i, num = NUM_PER_TASK, cnt = 0;

    while(1) {
        rdfs = bakfs;
        if(select(maxfd + 1, &rdfs, NULL, NULL, NULL) == -1) PERROR("select");
        for(i = 0; i <= maxfd; i++) {
            if(FD_ISSET(i, &rdfs)) {
                cnt = recv(i, buf, BUFSIZE, 0);
                if(cnt <= 0) {
                    FD_CLR(i, &bakfs);
                    if(--num <= 0) exit(0);
                }

                deal_recv(i, buf);
            }
        }
    }

    return 0;
}
