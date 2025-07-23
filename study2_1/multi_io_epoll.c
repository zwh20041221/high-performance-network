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
void add_connfd(int sock,int epoll_fd){
    struct sockaddr_in cli_addr;
    socklen_t len=sizeof(cli_addr);
    memset(&cli_addr,0,len);
    int connfd=accept(sock,(struct sockaddr*)&cli_addr,&len);//连接客户端
    printf("connect done\n");
    struct epoll_event interest_list;
    interest_list.data.fd=connfd;
    interest_list.events=EPOLLIN;//注册新的通信套接字
    if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,connfd,&interest_list)==-1){perror("epoll_ctl");return -1;}
}
void deal_cli(int epoll_fd,int connfd){
    char buf[buff_size];
    memset(buf, 0, sizeof(buf));
    int ret_recv=recv(connfd,buf,buff_size-1,0);
    if(ret_recv<0){perror("recv");return -1;}
    else if(ret_recv==0){
    printf("connfd:%dclose\n",connfd);
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
    //上面是常规套接字准备工作，下面进行epoll函数的使用
    int epoll_fd=epoll_create(5);//epoll_fd会占用一个描述符，如果监听描述符sock是3，那epoll+fd就是4，这是与select，poll的不同点之一，后续通信套接字从5开始
        struct epoll_event interest_list;
    interest_list.data.fd=sock;//把监听套接字注册上
    interest_list.events=EPOLLIN;
    if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,sock,&interest_list)==-1){perror("epoll_ctl");return -1;}
    struct epoll_event ready_list[MAX_FD];
    while(1){
        int ret_epoll=epoll_wait(epoll_fd,ready_list,MAX_FD,-1);
        if(ret_epoll==-1){perror("epoll_wait");return -1;}
    for(int i=0;i<ret_epoll;i++){
        if(ready_list[i].data.fd==sock){//监听套接字可读，连接新客户端
            add_connfd(sock,epoll_fd);
        }
        else{
            deal_cli(epoll_fd,ready_list[i].data.fd);
        }

    }
    }
    close(sock);
     return 0;
 }
