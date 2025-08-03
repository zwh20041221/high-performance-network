#ifndef __KVSTORE_H__
#define __KVSTORE_H__

#include<stdio.h>

#define MAX_FD_NUM 1024*128
#define BUF_LENGTH 1024
#define TOKEN_MAX_NUM 128
#define ARRAY_MAX_INDEX 1024
typedef void (*RCALLBACK)(int);//定义一个函数指针类型，相当于模版
struct fd_item{//在fd，buf,index的基础上添加事件触发后调用的回调函数
    int fd;
    char rbuf[BUF_LENGTH];//只负责处理读数据
    int rindex;
    char wbuf[BUF_LENGTH];//只负责处理写数据
    int windex;

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


struct array_item{
    char *key;
    char *value;
};
typedef struct array_s{
    struct array_item *array_table;
    int index;
}array_t;
extern array_t array;

int epoll_mode(void);
int kv_request(struct fd_item*,char *);
int kv_analysis_tokens(struct fd_item*,char **,int);

int array_create(array_t*);
int array_destroy(array_t*);
int array_set(char **);
int array_get(struct fd_item*,char **);















#endif