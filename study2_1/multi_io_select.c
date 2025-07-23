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
#define MAX_FD 6
void add_connfd(fd_set *read_fds,fd_set *read_tmp,int sock){
    struct sockaddr_in cli_addr;
    socklen_t len=sizeof(cli_addr);
    memset(&cli_addr,0,len);
    int connfd=accept(sock,(struct sockaddr*)&cli_addr,&len);
    printf("connect done\n");
    FD_SET(connfd,read_tmp);

}
void deal_cli(int connfd ,fd_set *read_fds,fd_set *read_tmp){
    char buf[buff_size];
    memset(buf, 0, sizeof(buf));
    int ret_recv=recv(connfd,buf,buff_size-1,0);
    if(ret_recv<0){perror("recv");return -1;}
    else if(ret_recv==0){
        printf("connfd:%dclose\n",connfd);
        FD_CLR(connfd,read_tmp);
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
    //上面是常规套接字准备工作，下面进行select函数的使用
    fd_set read_fds;
    fd_set read_tmp;
    FD_ZERO(&read_fds);
    FD_ZERO(&read_tmp);
    FD_SET(sock,&read_fds);
    FD_SET(sock,&read_tmp);
    while(1){
        read_fds=read_tmp;
        int ret_select=select(MAX_FD+1,&read_fds,NULL,NULL,NULL);
        if(ret_select<0){perror("select");return -1;}
        for(int i=0;i<MAX_FD;i++){
            if(FD_ISSET(i,&read_fds)&&i==sock){//如果可读的是监听套接字，则添加客户端
                add_connfd(&read_fds,&read_tmp,sock);

            }
            else if(FD_ISSET(i,&read_fds)&&i!=sock){
                deal_cli(i,&read_fds,&read_tmp);
            }
        }

    
    }
    close(sock);




//     struct sockaddr_in cli_addr;
//     socklen_t len=sizeof(cli_addr);
//     memset(&cli_addr,0,len);
//     int fds[conn_num];
//     int max_fd=0;
//     memset(fds,0,sizeof(fds));
//     for(int i=0;i<conn_num;i++){
//         fds[i]=accept(sock,(struct sockaddr*)&cli_addr,&len);
//         if(fds[i]<0){perror("accept");return -1;}
//         if(fds[i]>max_fd){max_fd=fds[i];}
//     }//待改，目前必须连接够conn_num个客户端才往下走
//     //select
//     fd_set read_fds;
//     FD_ZERO(&read_fds);
//     char buf[buff_size];
//     while(1){
//         FD_ZERO(&read_fds);
//         for(int i=0;i<conn_num;i++){
//             FD_SET(fds[i], &read_fds);
//         }
//         memset(buf, 0, sizeof(buf));//下面的select函数的第一个参数必须加1，否则取不到最大的fd
//         if(select(max_fd+1,&read_fds,NULL,NULL,NULL)==-1){perror("select");return -1;}
//         for(int fd=0;fd<max_fd+1;fd++){
//             if(FD_ISSET(fd,&read_fds)){
//                 if(recv(fd,&buf,buff_size-1,0)==-1){perror("recv");}
//                 printf("ip:%s  port:%d  fd:%d  context:%s\n",inet_ntoa(cli_addr.sin_addr),ntohs(cli_addr.sin_port),fd,buf);
//             }
//         }//有bug，当客户端断开连接后无限循环打印
//     }
     return 0;
 }
