#include "csapp.h"

int main(void){
    char *buf, *p;
    char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
    int n1 = 0, n2 = 0;
    
    if((buf = getenv("QUERY_STRING")) != NULL){
        p = strchr(buf, '&');
        *p = '\0';
        sscanf(buf, "first=%d", &n1);
        sscanf(p+1, "second=%d", &n2);
    }

    // 요청 바디 만들기
    sprintf(content, "Welcome to add.com: ");
    sprintf(content, "%sTHE Internet addition portal.\r\n<p>", content);
    sprintf(content, "%sThe answer is: %d + %d = %d\r\n<p>", content, n1, n2, n1 + n2);
    sprintf(content, "%sThanks for visiting!\r\n", content);

    // HTTP 요청 만들기
    printf("Connection: close\r\n");
    printf("Content-length: %d\r\n", (int)strlen(content));
    printf("Content-type: text/html\r\n\r\n");
    printf("%s", content);
    fflush(stdout);

    exit(0);
}