#include "csapp.h"

int main(int argc, char **argv)
{
    int clientfd;                       // 소켓 파일 디스크립터를 저장할 변수
    char *host, *port, buf[MAXLINE];    // 서버의 호스트 이름과 포트번호를 저장할 변수
    rio_t rio;                          // 요청한 바이트 수보다 적게 읽히는 현상을 처리하는 구조체

    if (argc != 3) {                    // 프로그램 시작할때 host, port 인자를 주라는 뜻뜻
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }

    host = argv[1];         // host 인자 입력
    port = argv[2];         // port 인자 입력

    clientfd = Open_clientfd(host, port);       // 주어진 호스트와 포트로 TCP 연결 시도, 성공하면 소켓 파일디스크립터를 받음
    Rio_readinitb(&rio, clientfd);              // rio_t 구조체를 초기화해서 소켓과 연결한다.

    while (Fgets(buf, MAXLINE, stdin) != NULL) {    // 사용자의 표준 입력으로부터 한줄을 읽어 buf에 저장
        Rio_writen(clientfd, buf, strlen(buf));     // 사용자가 입력한 문자열을 clientfd 소켓을 통해 서버로 전송한다.
        Rio_readlineb(&rio, buf, MAXLINE);          // clientfd 소켓으로부터 buf를 한줄 읽어온다.
        Fputs(buf, stdout);                         // 서버로 받은 데이터 출력
    }

    Close(clientfd);                                // 사용자가 입력을 멈추면 서버와의 연결을 닫는다.
    exit(0);
}
