#include <string.h>
#include"kvstore.h"

char*commands[]={
    "SET","GET","MOD","COUNT"
};
enum{
    CMD_START=0,
    CMD_SET=CMD_START,
    CMD_GET,
    CMD_MOD,
    CMD_COUNT,
    CMD_END
};
int divide_tokens(char *msg,char**tokens){//返回分割的token数量
	if (msg == NULL || tokens == NULL) return -1;
    int count=0;
    char*token=strtok(msg," ");//注意，是双引号不是单引号，前者是字符串，后者是常量字符，是int型
    while(token!=NULL){
        tokens[count++]=token;
        token=strtok(NULL," ");
    }
    return count;
}
int kv_analysis_tokens(struct fd_item*connfd_ifm,char **tokens,int token_count){//负责对客户端发来的三个命令token进行解析
	if (token_count == 0 || tokens == NULL) return -1;
    char*buf=connfd_ifm->wbuf;
    int cmd=0;
    for(cmd=CMD_START;cmd<CMD_END;cmd++){
        if(strcmp(commands[cmd],tokens[0])==0){break;}
    }
    switch (cmd)
    {
    case CMD_SET:
        int set_ret=array_set(tokens);
        if(set_ret==0){snprintf(buf,BUF_LENGTH,"success set");}
        else{snprintf(buf,BUF_LENGTH,"fail set");}
        break;
    case CMD_GET:
        int get_ret=array_get(connfd_ifm,tokens);
        if(get_ret==-1){snprintf(buf,BUF_LENGTH,"fail get");}
        break;
    default:
        snprintf(buf,BUF_LENGTH,"cmd not qualify");
        break;
    }

}
int kv_request(struct fd_item* connfd_ifm,char *msg){
    char* tokens[TOKEN_MAX_NUM];
    char* buf=msg;
    int token_count=divide_tokens(buf,tokens);
    // for(int i=0;i<token_count;i++){
    //     printf("%s\n",tokens[i]);
    // }
    kv_analysis_tokens(connfd_ifm,tokens,token_count);
    return 0;
}

int main(){
    if(array_create(&array)==-1){return -1;}
    epoll_mode();
    array_destroy(&array);
    return 0;
}