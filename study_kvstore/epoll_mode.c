#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/poll.h>
#include <sys/epoll.h>
#include<stdlib.h>
#include <sys/time.h>
#include"kvstore.h"
#define LISTEN_MAX_FD 1024
int epfd=0;
static int total_connections = 0;//连接计数
struct fd_item fd_infor_list[MAX_FD_NUM]={0};
typedef struct fd_item connect_t;
void http_response(connect_t*);
void recv_cb(int);
void send_cb(int);
void errExit(const char*);
void errExit(const char *msg) {// 自定义错误退出函数
    perror(msg);      // 打印错误信息 + errno 对应的描述
    exit(1); // 终止程序，返回失败状态码（通常为1）
}
void http_response(connect_t* conn){
conn->windex = sprintf(conn->wbuf,
    "HTTP/1.1 200 OK\r\n"
    "Accept-Ranges: bytes\r\n"
    "Content-Length: 78\r\n"
    "Content-Type: text/html\r\n"
    "Date: Sat, 06 Aug 2025 15:57:46 GMT\r\n\r\n"
    "<html><head><title>zwh.king</title></head><body><h1>zwh</h1></body></html>\r\n\r\n");
}
void add_interest_event(int connfd,int event){//专门进行epoll_ctl的add添加事件工作
    struct epoll_event ev;
    ev.data.fd=connfd;
    ev.events=event;
    epoll_ctl(epfd,EPOLL_CTL_ADD,connfd,&ev);//注册
}
void mod_interest_event(int connfd,int event){//专门进行epoll_ctl的mod修改事件工作
    struct epoll_event ev;
    ev.data.fd=connfd;
    ev.events=event;
    epoll_ctl(epfd,EPOLL_CTL_MOD,connfd,&ev);//修改感兴趣的事件
}
void accept_cb(int listenfd){//监听套接字EPOLLIN就绪时响应，连接新客户端，并将对应描述符注册到epoll上，在fd_infor_list数组新开一个元素负责该客户端的数据
    struct sockaddr_in cliaddr;
    memset(&cliaddr,0,sizeof(cliaddr));
    socklen_t cliaddr_len=sizeof(cliaddr);
    int connfd=accept(listenfd,(struct sockaddr*)&cliaddr,&cliaddr_len);//连接
    //printf("connect done connfd:%d\n",connfd);
    total_connections++;
    printf("New connection: %d, Total: %d\n", connfd, total_connections);
    if(connfd==-1){errExit("accept");}
    add_interest_event(connfd,EPOLLIN);
    fd_infor_list[connfd].fd=connfd;//为其配备专属fd_infor_list元素,绑定上回调函数
    memset(fd_infor_list[connfd].rbuf,0,BUF_LENGTH);
    memset(fd_infor_list[connfd].wbuf,0,BUF_LENGTH);
    fd_infor_list[connfd].rindex=0;
    fd_infor_list[connfd].windex=0;
    fd_infor_list[connfd].recv_type.recv_callback=recv_cb;
    fd_infor_list[connfd].send_callback=send_cb;
}
void recv_cb(int connfd){//通信套接字EPOLLIN就绪时响应，接收客户端发来的数据
    char*buf=fd_infor_list[connfd].rbuf;
    int index=fd_infor_list[connfd].rindex;
    int recv_count=recv(connfd,buf+index,BUF_LENGTH-index,0);
    if(recv_count==0){
        printf("connfd:%d disconnect\n",connfd);
        close(connfd);//老生常谈，这里必须手动close掉，否则每次epoll执行时都会直接响应，导致无限循环打印disconnect
    }
    else if(recv_count<0){errExit("recv");}
    else{
        kv_request(&fd_infor_list[connfd],buf+index);
        mod_interest_event(connfd,EPOLLOUT);
    }

}
void send_cb(int connfd){
    //http_response(&fd_infor_list[connfd]);

    char*buf=fd_infor_list[connfd].wbuf;
    int index=fd_infor_list[connfd].windex;
    int count=send(connfd,buf,BUF_LENGTH,0);
    printf("send to:%d context:%s current wbuf:%s\n",connfd,buf,buf);;
    if(count==-1){errExit("send");}
    mod_interest_event(connfd,EPOLLIN);

    /*发送给客户端必须将监控可写事件修改为监控可读事件，
    否则epoll会一直监控到可写从而一直触发
    */
}

int init_server(int port){
    struct sockaddr_in listenaddr;
    listenaddr.sin_family=AF_INET;
    listenaddr.sin_port=htons(port);
    listenaddr.sin_addr.s_addr=INADDR_ANY;
    int listenfd=socket(AF_INET,SOCK_STREAM,0);
    if(listenfd==-1){errExit("socket");}
    if(bind(listenfd,(struct sockaddr*)&listenaddr,sizeof(listenaddr))==-1){errExit("bind");}
    if(listen(listenfd,LISTEN_MAX_FD)==-1){errExit("listen");}
    return listenfd;
}

int epoll_mode(){
        int port_count=20;
    unsigned short port=8888;
    epfd=epoll_create(5);
    for(int i=0;i<port_count;i++){
        int listenfd=init_server(port+i);//创建20个监听socket，每一个都注册到epoll兴趣表中，都添加到管理监听套接字信息的数组中
        add_interest_event(listenfd,EPOLLIN);
        fd_infor_list[listenfd].fd=listenfd;
        fd_infor_list[listenfd].recv_type.accept_callback=accept_cb;//将负责管理监听套接字信息的数组元素绑定上回调函数，只要监听套接字的事件一触发，就调用
    }
    struct epoll_event ready_list[MAX_FD_NUM]={0};
    while(1){
    int ret_epoll=epoll_wait(epfd,ready_list,MAX_FD_NUM,-1);
    if(ret_epoll==-1){errExit("epoll_wait");}
    for(int i=0;i<ret_epoll;i++){
        int connfd=ready_list[i].data.fd;
        /*抛弃对监听套接字单列判断，转而以事件作为分类标准，
        就是因为fd_item中有每个fd注册事件触发后的回调函数，
        不同类型的fd在不同事件触发时调用不同的回调函数，
        这些回调函数在fd生成时就进行绑定初始化*/
        if(ready_list[i].events&EPOLLIN){fd_infor_list[connfd].recv_type.recv_callback(connfd);}
        else if(ready_list[i].events&EPOLLOUT){fd_infor_list[connfd].send_callback(connfd);}
    }
}
}
