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
typedef void (*RCALLBACK)(int);//定义一个函数指针类型，相当于模版
int epfd=0;
struct fd_item{//在fd，buf,index的基础上添加事件触发后调用的回调函数
    int fd;
    char buf[BUF_LENGTH];//限制长度读取最大长度为BUF_LENGTH
    int index;//记录已读取的长度

        /*就是因为fd_item中有每个fd注册事件触发后的回调函数，
        不同类型的fd在不同事件触发时调用不同的回调函数，
        这些回调函数在fd生成时就进行绑定初始化*/

    union{
    RCALLBACK accept_callback;//监听套接字绑定
    RCALLBACK recv_callback;//通信套接字绑定
    }recv_type;//epollin触发后的回调
    RCALLBACK send_callback;//epollout触发后的回调
    //对于每一个来连接的新客户端，在accept后都及时给他一个专属的回调函数，只要事件一触发就调用回调
};
struct fd_item fd_infor_list[MAX_FD_NUM]={0};
void recv_cb(int);
void send_cb(int);
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
    printf("connect done connfd:%d\n",connfd);
    if(connfd==-1){perror("accept");return -1;}
    add_interest_event(connfd,EPOLLIN);
    fd_infor_list[connfd].fd=connfd;//为其配备专属fd_infor_list元素,绑定上回调函数
    fd_infor_list[connfd].recv_type.recv_callback=recv_cb;
    fd_infor_list[connfd].send_callback=send_cb;
}
void recv_cb(int connfd){//通信套接字EPOLLIN就绪时响应，接收客户端发来的数据
    char*buf=fd_infor_list[connfd].buf;
    int index=fd_infor_list[connfd].index;
    int recv_count=recv(connfd,buf+index,BUF_LENGTH-index,0);
    if(recv_count==0){
        printf("connfd:%d disconnect\n",connfd);
        close(connfd);//老生常谈，这里必须手动close掉，否则每次epoll执行时都会直接响应，导致无限循环打印disconnect
    }
    else if(recv_count<0){perror("recv");return -1;}
    else{
    fd_infor_list[connfd].index+=recv_count;
    printf("connfd:%d byte:%d context:%s\n",connfd,recv_count, buf);
    }
    mod_interest_event(connfd,EPOLLOUT);//收到客户端来信后再发给客户端，所以recv后监测可写
}
void send_cb(int connfd){
    char*buf=fd_infor_list[connfd].buf;
    int index=fd_infor_list[connfd].index;
    int conut=send(connfd,buf,index,0);
    if(conut==-1){perror("send");return -1;}
    mod_interest_event(connfd,EPOLLIN);

    /*发送给客户端必须将监控可写事件修改为监控可读事件，
    否则epoll会一直监控到可写从而一直触发
    */
}
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
    add_interest_event(listenfd,EPOLLIN);
    struct epoll_event ready_list[MAX_FD_NUM];
    fd_infor_list[listenfd].fd=listenfd;
    fd_infor_list[listenfd].recv_type.accept_callback=accept_cb;//将负责管理监听套接字信息的数组元素绑定上回调函数，只要监听套接字的事件一触发，就调用
    while(1){
    int ret_epoll=epoll_wait(epfd,ready_list,MAX_FD_NUM,-1);
    if(ret_epoll==-1){perror("epoll_wait");return -1;}
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
close(listenfd);
close(epfd);
    return 0;
}