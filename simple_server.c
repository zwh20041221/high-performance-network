
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/poll.h>
#include <sys/epoll.h>
#define BUFF_SIZE 1024
void* cli_pthread(void*arg){
    int connfd=*(int *)arg;
    while(1){
        char buff[BUFF_SIZE];
        memset(buff,0,sizeof(buff));
        int count=recv(connfd,buff,BUFF_SIZE-1,0);
        if(count==0){
        printf("cli_close\n");
        break;
        }
        printf("connfd:%d,context:%s\n",connfd, buff);
    }
    close(connfd);
    
}
int main(){
//准备地址结构
    struct sockaddr_in ser;
    memset(&ser, 0, sizeof(ser));  // 清空结构体
    ser.sin_family=AF_INET;
    ser.sin_port=htons(8888);
    ser.sin_addr.s_addr=INADDR_ANY;
//创建套接字
    int sock=socket(AF_INET,SOCK_STREAM,0);
    if(sock==-1){
        perror("socket");
        return -1;
    }
//绑定套接字
    if(bind(sock,(struct sockaddr*)&ser,sizeof(ser))==-1){
        perror("bind");
        return -1;
    }
//监听套接字
    if(listen(sock,5)==-1){
        perror("listen");
        return -1;
    }
    while (1){
        //接受连接
    struct sockaddr_in cli;
    socklen_t len=sizeof(cli);
    memset(&cli, 0, sizeof(cli)); // 清空客户端地址结构
    int connfd=accept(sock,(struct sockaddr*)&cli,&len);
    if(connfd==-1){
        perror("accept");
        return -1;
    }
    pthread_t th_id;
    pthread_create(&th_id,NULL,cli_pthread,&connfd);
    }//多线程处理accept
    

    close(sock);
    return 0;
}