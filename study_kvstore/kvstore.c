#include <string.h>
#include"kvstore.h"
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
void kv_request(char *msg){
    char* tokens[TOKEN_MAX_NUM];
    char* buf=msg;
    int token_count=divide_tokens(buf,tokens);
    for(int i=0;i<token_count;i++){
        printf("%s\n",tokens[i]);
    }

}

int main(){
    epoll_mode();
    return 0;
}