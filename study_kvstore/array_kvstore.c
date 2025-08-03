#include"kvstore.h"
#include<string.h>
#include <stdlib.h>
array_t array;
int array_create(array_t*arr){
    arr->array_table=(struct array_item*)malloc(sizeof(struct array_item)*ARRAY_MAX_INDEX);
    if(arr->array_table==NULL){return -1;}
    arr->index=0;
}
int array_destroy(array_t*arr){
    free(arr);
}
int array_set(char **tokens){
    /*先检查数组是否已经达到上限，然后检查当前最大索引前面的索引中
    是否有key==NULL，即被删除的kv，如果有，那么不用在末尾添加新元素，直接在NULL处添加即可。
    如果没有，那就在末尾添加新元素*/
    if(array.index==ARRAY_MAX_INDEX){return -1;}

    //struct array_item item=array.array_table[array.index++];
    /*这行代码是错误的，这将导致下面所有操作都操作到临时变量item中，
    并没有真正改变array中的内容，所以要用指针操作*/
    struct array_item *item=&(array.array_table[array.index++]);
    item->key=(char*)malloc(sizeof(char)*128);
    item->value=(char*)malloc(sizeof(char)*128);
    /*注意，如果初始化array时没有array_table分配内存
    那将成为野指针，此时访问它会报错*/
    strcpy(item->key,tokens[1]);
    strcpy(item->value,tokens[2]);
    /*注意，即使给array_table分配了内存，每次set必须还要给每一个key和value
    分配内存，否则他们也会是野指针。此时访问它们也会报错*/
    return 0;
}
int array_get(struct fd_item*connfd_ifm,char **tokens){
    char *buf=connfd_ifm->wbuf;
    int index=connfd_ifm->windex;
    memset(buf,0,BUF_LENGTH);
    for(int i=0;i<array.index;i++){
        if(strcmp(array.array_table[i].key,tokens[1])==0){
            strcpy(buf,array.array_table[i].value);
            index=strlen(buf);
            return 0;
        }
    }
    return -1;
}
