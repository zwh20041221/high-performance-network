//没有打印是因为没有加\n,具体去UC笔记中看，有记载关于printf的知识
//deal_cli 函数中判断是否断开连接时写成else if(ret_recv=0)这个经典错误
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include<sys/select.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include<arpa/inet.h>
#include <pthread.h>
#include <sys/poll.h>
#include <sys/epoll.h>
#define conn_num 3
#define buff_size 1024
#define MAX_FD 10
void add_connfd(struct pollfd*fds,int sock){
    struct sockaddr_in cli_addr;
    socklen_t len=sizeof(cli_addr);
    memset(&cli_addr,0,len);
    int connfd=accept(sock,(struct sockaddr*)&cli_addr,&len);
    printf("connect done\n");
    fds[connfd].fd=connfd;
    fds[connfd].events=POLLIN;
}
void deal_cli(int connfd ,struct pollfd*fds){
    char buf[buff_size];
    memset(buf, 0, sizeof(buf));
    int ret_recv=recv(connfd,buf,buff_size-1,0);
    if(ret_recv<0){perror("recv");return -1;}
    else if(ret_recv==0){
        printf("connfd:%dclose\n",connfd);
        fds[connfd].fd=-1;
        close(connfd);
    }
    else{
        printf("connfd:%d,context:%s\n",connfd,buf);
    }
}
int main(){
    //准备地址结构
    struct sockaddr_in ser_addr;
    ser_addr.sin_family=AF_INET;
    ser_addr.sin_port=htons(8888);
    ser_addr.sin_addr.s_addr=INADDR_ANY;
    //创建套接字
    int sock=socket(AF_INET,SOCK_STREAM,0);
    if(sock==-1){perror("socket");return -1;}
    //实例化套接字
    if(bind(sock,(struct sockaddr*)&ser_addr,sizeof(ser_addr))==-1){perror("bind");return -1;}
    //监听
    if(listen(sock,5)==-1){perror("listen");return -1;}
    //等待连接
    //上面是常规套接字准备工作，下面进行poll函数的使用
    struct pollfd fds[MAX_FD];
    memset(fds,0,sizeof(fds));
    for(int i=0;i<MAX_FD;i++){
        fds[i].fd=-1;
    }
    fds[sock].fd=sock;
    fds[sock].events=POLLIN;
    while(1){
    int ret_poll=poll(&fds,MAX_FD,-1);
    if(ret_poll==-1){perror("poll");return -1;}
    for(int i=0;i<MAX_FD;i++){
        if((fds[i].revents==POLLIN)&&(i==sock)){//监测监听套接字
            add_connfd(fds,sock);
        }
        else if((fds[i].revents==POLLIN)&&(i!=sock)){//监听通信套接字
            deal_cli(i,fds);
        }
    }
    }
    close(sock);
     return 0;
 }
