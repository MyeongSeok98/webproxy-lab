#include "csapp.h"

void echo(int connfd);

int main(int argc, char **argv)
{
    int listenfd, connfd;       // 클라이언트와의 연결 요청을 기다리는데 사용할 리스닝 소켓과 실제로 연결되서 데이터를 주고받는데 사용할 연결 소켓의 fd
    socklen_t clientlen;        // 클라이언트 주소 구조체의 크기를 저장할 변수
    struct sockaddr_storage clientaddr; // 연결될 클라이언트의 주소 정보를 저장할 구조체
    char client_hostname[MAXLINE], client_port[MAXLINE];    // 클라이언트의 호스트 이름과 포트번호 문자열을 저장할 버퍼

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    listenfd = Open_listenfd(argv[1]);      // 주어진 포트번호로 들어오는 연결을 받아들여서 리스닝 소켓 fd를 반환한다.
    while (1) {
        clientlen = sizeof(struct sockaddr_storage);    // 실제 클라이언트 주소 구조체의 크기를 기록한다.
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);   //클라이언트 연결 요청 대기 및 수락을 한다.
        Getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAXLINE,     // accept를 통해 얻은 클라이언트 주소 정보를 사람이 읽을 수 있는 호스트 이름과 포트 번호 문자열로 변경한다.
                    client_port, MAXLINE, 0);
        printf("Connected to (%s, %s)\n", client_hostname, client_port);

        echo(connfd);           // 연결된 소켓을 인자로 넘겨줘서 echo 함수를 처리한다.
        Close(connfd);          // 연결된 소켓을 닫는다.
    }
    exit(0);
}

void echo(int connfd)       // 소켓 디스크립터를 인자로 받는다.
{
    size_t n;
    char buf[MAXLINE];
    rio_t rio;

    Rio_readinitb(&rio, connfd);    // RIO 구조체를 초기화해 connfd 소켓으로부터 데이터를 읽을 준비를 한다.
    while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {      // 클라이언트로부터 받은 데이터가 0이면 루프 종료
        printf("server received %d bytes\n", (int)n);           // 몇바이트 수신했는지 출력
        Rio_writen(connfd, buf, n);                             // 클라이언트로부터 읽은 데이터를 읽은 만클의 크기 그대로 다시 클라이언트에게 전송한다.
    }
}