#include "csapp.h"

void doit(int fd);          // HTTP 클라이언트 요청을 처리하는 메인 루틴
void read_requesthdrs(rio_t *rp);   // HTTP 요청 헤더를 읽고 처리한다.
int parse_uri(char *uri, char *filename, char *cgiargs);    // 클라이언트가 요청한 URI를 분석해서 정적 콘텐츠인지, 동적 콘텐츠인지 확인하고 파일 이름과 CGI 인자를 추출한다.
void serve_static(int fd, char *filename, int filesize);    // 정적 파일을 클라이언트에게 전송한다.
void get_filetype(char *filename, char *filetype);          // 파일 이름 확장자를 보고 MIME 타입을 결정한다. (EX. html -> text/html) 
void serve_dynamic(int fd, char *filename, char *cgiargs);  // CGI 프로그램을 실행해서 동적 콘텐츠를 생성하고 클라이언트에게 전송한다.
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,   // 클라이언트에게 HTTP 오류 메세지를 전송한다.
                 char *longmsg);
void sigchild_handler(int sig);
void echo(int connfd);

int main(int argc, char **argv)
{
    int listenfd, connfd;       // 리스닝 소켓 디스크립터와 연결된 소켓 디스크립터.
    char hostname[MAXLINE], port[MAXLINE];    // 호스트이름과 포트 저장
    socklen_t clientlen;        // 구조체의 크기 저장
    struct sockaddr_storage clientaddr; // 클라이언트 주소 정보 저장

    /* Check command-line args */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    /* 11.8 문제 답안 */
    if(Signal(SIGCHLD, sigchild_handler) == SIG_ERR) // 어떤 자식 프로세스든 종료되면 main 프로세스에게 SIGCHLD 시그널을 보내고 그러면 signal_handler함수가 실행된다.
      unix_error("signal child handler error");      // 에러 메세지 출력하고 프로그램 종료시킴
    /* 11.8 문제 답안 */

    listenfd = Open_listenfd(argv[1]);    // 포트 번호를 이용해 리스닝 소켓을 생성하고 listening 상태로 만든다.
    while (1) {
        clientlen = sizeof(struct sockaddr_storage);    // 구조체의 크기를 초기화한다.
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // 연결 요청이 들어오면 주소 정보를 clientaddr에 저장하고 소켓은 connfd로 반환한다.
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE,   // 주소 정보를 사람이 읽을 수 있는 호스트 이름과 포트 번호 문자열로 변환한다.
                    port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        doit(connfd);                                             // doit 함수를 호출해 현재 연결된 클라이언트의 HTTP 요청을 처음부터 끝까지 처리한다. 
        Close(connfd);                                            // 다시 Accept 함수 호출 지점으로 돌아간다.
    }
}

/* 11.6 문제 처리용 */
void echo(int connfd){
   size_t n;
   char buf[MAXLINE];
   rio_t rio;

   Rio_readinitb(&rio, connfd);
   while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0){
    if (strcmp(buf, "\r\n") == 0)
      break;
    Rio_writen(connfd, buf, n);
   }
}
/* 11.6 문제 처리용 */

/* 11.8 문제 처리용 */
void sigchild_handler(int sig){
  int old_errno = errno;
  int status;
  pid_t pid;
  while((pid = waitpid(-1, &status, WNOHANG)) > 0){
  }
  errno = old_errno;
}
/* 11.8 문제 처리용 */

void doit(int fd){
  int is_static;      // 요청이 정적 컨텐츠(1)인지 동적컨텐츠(0)인지 나타내는 변수 
  struct stat sbuf;   // 파일의 상태 정보(크기, 권한 등)을 저장하는 구조체
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE]; // 요청 라인을 읽을 버퍼(buf), 파싱된 메소드, URI, HTTP 버전을 저장할 문자 배열
  char filename[MAXLINE], cgiargs[MAXLINE];   // 요청한 파일 경로(filename)와 CGI 프로그램에 전달될 인자(cgiargs)를 저장할 문자 배열
  rio_t rio;        // Robust I/O 구조체 변수, 소켓으로부터 안정적인 읽기를 위해 사용

  // 요청라인과 헤더를 읽는 부분
  Rio_readinitb(&rio, fd);    // rio 구조체를 초기화해서 fd 소켓에서 읽을 준비를 한다.
  if(!(Rio_readlineb(&rio, buf, MAXLINE))){
    return;
  }    // fd 소켓으로부터 HTTP 요청의 첫번째 라인(요청 라인)을 읽어 buf에 저장한다.
  printf("Request headers:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version);    //buf에 저장된 요청 라인을 공백을 기준으로 파싱해 method, uri, version 변수에 각각 저장

  // 요청 메소드가 GET이 아닌 경우 strcasecmp는 대소문자 무시하고 비교, 0이 아니면 다름
  if(strcasecmp(method, "GET")){
    // clienterror 함수를 호출해 501 Not implement 오류를 보냄
    clienterror(fd, method, "501","Not implemented", "Tiny does not implement this method");
    return;
  }
  // 나머지 요청 헤더들을 읽고 무시한다.
  read_requesthdrs(&rio);

  // parse_uri 함수를 호출해 uri를 분석
  // 정적 컨텐츠면 1, 동적 컨텐츠면 0
  // filename은 파일 경로, cgiargs는 CGI인자
  is_static = parse_uri(uri, filename, cgiargs);

  // stat함수를 이용해 filename경로의 파일 정보를 sbuf 구조체에 가져온다.
  // 파일이 없거나 오류가 발생하면 오류 발생
  if(stat(filename, &sbuf) < 0){
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
    return;
  }

  // 정적 콘텐츠면
  if(is_static){
    // 파일이 일반파일이 아니거나 읽기 권한이 없는 경우
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)){
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program.");      // 403 오류 응답을 보냄
      return;
    }
    serve_static(fd, filename, sbuf.st_size);     // serve_static 함수를 호출해 정적 파일(filename)의 내용을 클라이언트에게 보낸다. 파일 크기도 보낸다.
  }
  // 동적 콘텐츠면
  else{
    // 파일이 일반파일이 아니거나 실행행 권한이 없는 경우
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)){
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program.");      // 403 오류 응답을 보냄
      return;
    }
    serve_dynamic(fd, filename, cgiargs);       // serve_dynamic 함수를 호출해 CGI 프로그램을 실행하고, 그 결과를 클라이언트에게 보낸다.  CGI 인자도 보낸다.
  }
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg){
  char buf[MAXLINE], body[MAXBUF];  // HTTP 응답 헤더를 만들 buf와 응답 본문(HTML)을 만들 body 버퍼 선언

  // HTTP 응답의 body부분 작성
  // body 버퍼에 HTML 시작 부분과 에러 메세지 타이틀을 작성한다.
  sprintf(body, "<html><title>TinyError</title>");
  // 기존 body 내용 뒤에 HTML body 태그와 배경색 추가 (%s는 이전 바디 내용)
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
  // 기존 바디 내용 뒤에 오류 번호와 메세지 추가
  sprintf(body, "%s%s:%s\r\n", body, errnum, shortmsg);
  // 기존 바디 내용 뒤에 긴 메세지와 오류 원인을 문단으로 추가
  sprintf(body, "%s<p>%s:%s\r\n",body, longmsg, cause);
  // 기존 바디 내용 뒤에 수평선과 서버 정보 텍스트를 추가
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);
  
  // HTTP 응답의 헤더 부분 작성
  // buf에 버전 정보 및 오류 정보, 짧은 메세지 추가
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  // buf의 현재 내용을 fd를 통해 클라이언트에게 보냄. 
  Rio_writen(fd, buf, strlen(buf));
  // content-type 헤더를 작성
  sprintf(buf, "Content-type: text/html\r\n");
  // content-type 헤더를 클라이언트에게 보냄
  Rio_writen(fd, buf, strlen(buf));
  // content-type 헤더를 작성하고 헤더의 끝을 알리는 빈줄도 추가
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    // buf에 작성된 Content-length 헤더와 빈 줄을 클라이언트에게 전송
  Rio_writen(fd,buf,strlen(buf));
  // 최종적으로 헤더 이후 body 내용까지 클라이언트에게 전송
  Rio_writen(fd, body, strlen(body));
}

void read_requesthdrs(rio_t *rp){

  char buf[MAXLINE]; // 한 줄의 헤더를 임시로 저장할 문자 배열

  Rio_readlineb(rp, buf, MAXLINE);    // 첫번째 헤더라인을 읽는다. 이때 이미 doit 함수에서 요청라인은 읽은 상태이다.
  while(strcmp(buf, "\r\n")){         // 헤더부분의 끝은 빈줄이기 때문에 헤더 부분이 빈줄이 아닐동안 루프를 실행함
    Rio_readlineb(rp, buf, MAXLINE);  // 다음 헤더 라인을 읽어서 buf에 덮어씀
    printf("%s", buf);        // 방금 읽은 버퍼를 출력력
  }
  return;
}

int parse_uri(char *uri, char *filename, char *cgiargs){
  char *ptr;  // 문자열 내 특정 위치를 가리키기 위한 포인터

  // filename을 구성 : 현재 디렉토리(`.`) + URI 경로
  if(!strstr(uri, "cgi-bin")){    // URI에 "cgi-bin" 문자열이 포함되어 있는지 확인
                                  // strstr(uri, "cgi-bin")은  uri에서 `cgi-bin`이 문자열이 있으면 반환, 없으면 NULL
    strcpy(cgiargs, "");          // CGI 인자가 없으므로 cgiargs를 빈 문자열로 설정
    //sprintf(filename, ".%s", uri); 로 개선여지 있음
    strcpy(filename, ".");        // filename에 "."을 복사
    strcat(filename, uri);        // filename 뒤에 uri를 이어붙임
    
    // 만약 디렉토리가 `/`로 끝난다면
    // 기본 파일인 `home.html`을 filename뒤에 추가
    if(uri[strlen(uri)-1] == '/')
      strcat(filename, "index.html");
    return 1;
  }
  // URI에 "cgi-bin" 문자열이 포함된 경우 -> CGI 요청으로 간주
  else{
    // URI에서 `?` 문자의 위치를 찾음. `?`는 CGI 스크립트 경로와 인자를 구분
    ptr = index(uri, '?');
    // 만약 `?` 문자가 있으면
    if(ptr){
      // `?` 다음 문자부터 문자열 끝까지를 cgiargs에 복사 (ptr+1)이 인자의 시작
      strcpy(cgiargs, ptr+1);
      *ptr = '\0';    // uri를 NULL로 변경, "/cgi-bin/adder?a=1&b=2" 이면 "/cgi-bin/adder"로 변경
    }
    // `?`문자가 URI에 없다면
    else 
      strcpy(cgiargs, "");  // cgiargs를 빈 문자열로 설정
    //sprintf(filename, ".%s", uri); 로 개선여지 있음
    strcpy(filename, ".");  // filename에 `.`을 복사
    strcat(filename, uri);  // filename뒤에 잘려나간 uri를 이어붙임
    return 0;   // 0 은 CGI 요청임을 밝힘
  }
}

void serve_static(int fd, char *filename, int filesize){
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXLINE];
  get_filetype(filename, filetype);

  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf,"%sServer:Tiny Web Server\r\n",buf);
  sprintf(buf,"%sConnection:close\r\n",buf);
  sprintf(buf,"%sContent-length:%d\r\n",buf, filesize);
  sprintf(buf,"%sContent-type:%s\r\n\r\n",buf, filetype);

  Rio_writen(fd, buf, strlen(buf));

  printf("Response headers:\n");
  printf("%s", buf);

  srcfd = Open(filename, O_RDONLY, 0);

  srcp = (char*)malloc(filesize);
  Rio_readn(srcfd, srcp, filesize);

  Close(srcfd);
  Rio_writen(fd, srcp, filesize);
  free(srcp);
}

void get_filetype(char *filename, char *filetype){
  if(strstr(filename,".html"))
    strcpy(filetype,"text/html");
  else if(strstr(filename,".gif"))
    strcpy(filetype,"image/gif");
  else if(strstr(filename,".png"))
    strcpy(filetype,"image/png");
  else if(strstr(filename,".jpg"))
    strcpy(filetype,"image/jpeg");
  else if(strstr(filename, ".mp4"))
    strcpy(filetype,"video/mp4");
  else
    strcpy(filetype,"text/plain");
}

void serve_dynamic(int fd, char *filename, char *cgiargs){
  char buf[MAXLINE], *emptylist[] = { NULL };   // buf : HTTP 응답 헤더 일부 구성용, 
  // emptylist : execve에 사용할 빈 인자 목록

  // 기본적인 HTTP 응답 헤더의 일부를 클라이언트에게 먼저 전송한다.
  // CGI 프로그램이 나머지 헤더와 본문을 생성하여 전송한다.
  sprintf(buf, "HTTP/1.0 200 OK\r\n");    // HTTP 상태 라인
  Rio_writen(fd, buf, strlen(buf));       // 클라이언트에게 전송
  sprintf(buf,"Server: Tiny Web Server\r\n");   // 서버 정보
  Rio_writen(fd, buf, strlen(buf));       // 클라이언트에게 전송

  // 자식 프로세스를 생성한다.
  if(Fork() == 0){
    // 자식 프로세스가 CGI 프로그램의 QUERY_STRING 환경변수에 cgiargs 값을 설정한다.
    // CGI 프로그램은 이 환경 변수를 통해 클라이언트가 보낸 인자를 받는다.
    setenv("QUERY_STRING",cgiargs,1);

    // 표준 출력을 클라이언트 소켓으로 재지정
    // 자식프로세스의 표준 출력을 클라이언트 소켓 파일 디스크럽터로 복제한다.
    Dup2(fd,STDOUT_FILENO);
    // CGI 프로그램을 실행한다.
    // 자식 프로세스의 이미지를 filename에 지정된 프로그램으로 교체한다.
    Execve(filename, emptylist, environ);
  }
  // 자식 프로세스가 종료될 때까지 기다린다.
  Wait(NULL);
}