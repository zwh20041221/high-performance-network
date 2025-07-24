#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/poll.h>
#include <sys/epoll.h>
#include <sys/time.h>
#define LISTEN_MAX_FD 10
#define MAX_FD_NUM 10
#define BUF_LENGTH 128
int epfd=0;
struct fd_item{
    int fd;
    char buf[BUF_LENGTH];//限制长度读取最大长度为BUF_LENGTH
    int index;//记录已读取的长度
};
struct fd_item buffer[MAX_FD_NUM]={0};
int accept_cb(int listenfd){//监听套接字EPOLLIN就绪时响应，连接新客户端，并将对应描述符注册到epoll上，在buffer数组新开一个元素负责该客户端的数据
    struct sockaddr_in cliaddr;
    memset(&cliaddr,0,sizeof(cliaddr));
    
    /*这里也埋下了一个雷，把结构体中的buf指针赋成了0，导致后续的recv：bad address*/
    
    socklen_t cliaddr_len=sizeof(cliaddr);
    int connfd=accept(listenfd,(struct sockaddr*)&cliaddr,&cliaddr_len);//连接
    printf("connect done connfd:%d\n",connfd);
    if(connfd==-1){perror("accept");return -1;}
    struct epoll_event ev;
    ev.data.fd=connfd;
    ev.events=EPOLLIN;
    epoll_ctl(epfd,EPOLL_CTL_ADD,connfd,&ev);//注册
    buffer[connfd].fd=connfd;//为其配备专属buffer元素
}
int recv_cb(int connfd){//通信套接字EPOLLIN就绪时响应，接收客户端发来的数据
    char*buf=buffer[connfd].buf;
    int index=buffer[connfd].index;
    int recv_count=recv(connfd,buf+index,BUF_LENGTH-index,0);
    if(recv_count==0){
        printf("connfd:%d disconnect\n",connfd);
    }
    else if(recv_count<0){perror("recv");return -1;}
    else{
    buffer[connfd].index+=recv_count;
    printf("connfd:%d context:%s\n",connfd,buf);
    }
}
//int send_cb();
int main(){
    struct sockaddr_in listenaddr;
    listenaddr.sin_family=AF_INET;
    listenaddr.sin_port=htons(8888);
    listenaddr.sin_addr.s_addr=INADDR_ANY;
    int listenfd=socket(AF_INET,SOCK_STREAM,0);
    if(listenfd==-1){perror("socket");return -1;}
    if(bind(listenfd,(struct sockaddr*)&listenaddr,sizeof(listenaddr))==-1){perror("bind");return -1;}
    if(listen(listenfd,LISTEN_MAX_FD)==-1){perror("listen");return -1;}
    epfd=epoll_create(5);
    struct epoll_event ev;
    struct epoll_event ready_list[MAX_FD_NUM];
    ev.data.fd=listenfd;
    ev.events=EPOLLIN;
    epoll_ctl(epfd,EPOLL_CTL_ADD,listenfd,&ev);//首先把监听套接字注册上去
    buffer[listenfd].fd=listenfd;
    while(1){
    int ret_epoll=epoll_wait(epfd,ready_list,MAX_FD_NUM,-1);
    if(ret_epoll==-1){perror("epoll_wait");return -1;}
    for(int i=0;i<ret_epoll;i++){
        if(ready_list[i].data.fd==listenfd){accept_cb(listenfd);}
        else if(ready_list[i].events&EPOLLIN){recv_cb(ready_list[i].data.fd);}
        //else if(ready_list[i].events&EPOLLOUT){send_cb;}
    }
}
close(listenfd);
close(epfd);
    return 0;
}