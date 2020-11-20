#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include "FileTree.h"

#define BUFSIZE 512

struct client_info { //클라이언트 정보 담음
    int ns; //소켓번호
    char ip[BUFSIZE]; //ip
};

pthread_mutex_t m_lock; //한번에 한개의 스레드만 실행하기위해 lock 걸때 사용
char * root; //service directory
int log_file; // log.txt에 대한 파일 기술자
struct Node * node; //트라이 루트

int sendFile(int ns, int fd, const char * realPath) { //realPath에 있는 파일 데이터를 클라이언트 소켓으로 전송함.

    int byte_n = 0; //총 데이터 양 저장.
    int byte_temp; 
    char buf[BUFSIZE]; // 송수신 할때 사용하는 버퍼
    while ((byte_temp = read(fd, buf, BUFSIZE)) > 0) { //해당 파일로부터 BUFSIZE 만큼 받아옴
        byte_n += byte_temp; 
        if(byte_temp < BUFSIZE) //읽어온 바이트 길이가 버퍼사이즈보다 작을때
            buf[byte_temp] = 0;
        // printf("%s",buf);
        if(send(ns, buf, byte_temp, 0) == -1) { //읽어온 바이트수만큼 클라이언트 소켓에 보냄
            perror("send");
            exit(1); //전송 실패시 프로그램 종료
        }
    }
    close(fd); //해당 파일 close
    return byte_n; //총 전송 데이터 량 반환
}

long long getSum(char * parameter) { //parameter로부터 합을 얻음

    long long from = 0, to = 0; 
    sscanf(parameter, "from=%lld&to=%lld", &from, &to); //parameter 문자열로부터 from과 to 추출

    return ((to * (to + 1)) / 2) - (((from - 1) * from) / 2); //sum(1 ~ n) = n * (n + 1) / 2
}

void* sender(void * data) {
    
    char realPath[BUFSIZE]; //실제 경로를 저장
    char header[BUFSIZE]; //헤더를 저장
    char type[BUFSIZE]; // 파일의 타입을 저장.
    char buf[BUFSIZE]; // 송수신 할때 사용하는 버퍼
    char parameter[BUFSIZE]; //from=NNN&to=MMM
    long long rs = 0; //total.cgi일때 result 저장
    int ns;  // 소켓번호 가져옴
    char * ip;  //클라이언트 ip 저장 
    int messageSize = 0; //받아온 메세지 크기를 담는 변수
    char path[BUFSIZE]; // path 저장
    char requestPath[BUFSIZE]; //실제 클라이언트가 요청한 urlrequest 저장
    int fd; //파일 기술자 저장
    char log[BUFSIZE]; //log를 담는 버퍼
    int search_rs; //검색결과 담음
    struct client_info ci = *((struct client_info *) data); //클라이언트 정보 얻음
    ns = ci.ns;
    ip = ci.ip;

    if((messageSize = recv(ns, buf, BUFSIZE - 1, 0)) == -1) { //소켓번호 ns로 부터 BUFSIZE만큼 데이터 얻어옴,
                                                                //messagesize로 socket close됫는지 check 
        perror("recv"); // recv 실패시
        exit(1);
    }

    buf[messageSize] = 0;
    
    int x,y;
    int parameter_index = 0; //parameter 문자열 길이 저장

    // printf("%d\n",messageSize);
    if(messageSize > 0) { //보낸 데이터가 있을시
        search_rs = search_n(node, requestPath, type, path, parameter, &parameter_index, buf + 4, messageSize);
        //트라이에 클라이언트가 보낸 요청을 탐색하게하여, type, path, parameter을 얻게함.
        // printf("output type : %s path : %s\n", type, path);
    }

    sprintf(header, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\n\r\n", type); // --> 헤더 저장 
    
    if(send(ns, header, strlen(header), 0) == -1) { //헤더 정보 보냄.
        perror("send");
        exit(1);
    }

    if(parameter_index > 0) //파라미터 존재시
        rs = getSum(parameter); //파라미터로부터 결과값을 얻음.

    int byte_n = 0;

    if(strstr(path, "/total.cgi") != NULL) { //클라이언트가 요청한 path가 total.cgi라면
        sprintf(buf, "<html><head></head> <body><h1>%lld</h1></body></html>", rs); 
        byte_n = strlen(buf); //전송할 데이터 크기 저장
        if((send(ns, buf, byte_n, 0)) == -1) { //데이터 전송
            perror("send");
            exit(1); //전송 실패시 프로그램 종료
        }
    }
    else {
        sprintf(realPath, "%s%s", root, path); //realpath = service_directory + path
        fd = open(realPath, O_RDONLY, 0444); //file open

        if(fd > 0) { //realpath에 파일이 존재시
            byte_n = sendFile(ns, fd, realPath); //파일을 보냄
        }
        else {  // not found
            sprintf(buf, "<html><head></head> <body><h1>%s</h1></body></html>", "Not found");
            byte_n = strlen(buf); //전송할 데이터 크기 저장
            if((send(ns, buf, byte_n, 0)) == -1) { //데이터 전송
                perror("send");
                exit(1); // 전송 실패시 프로그램 종료
            }
        }
    }
    
    if(search_rs == 1) //원하는 데이터가 트라이에 있는경우 -> 그냥 트라이에 있는 정보를 path로
        sprintf(log, "%s %s %d\n", ip, path, byte_n); //ip, path, send_size
    else //원하는 데이터가 트라이에 없는경우 -> 실제 요청한 url을 path로
        sprintf(log, "%s %s %d\n", ip, requestPath, byte_n); //ip, path, send_size
    // printf("%s",log);
    pthread_mutex_lock(&m_lock); //임계영역 log.txt를 보호 
    if(write(log_file, log, strlen(log)) != strlen(log)) // log.txt에 log 기록
        perror("Write");
    pthread_mutex_unlock(&m_lock); //unlock
    
    free(data);   //스레드로 넘어온 인자 free
    close(ns); //사용자가 요청한 자원 넘겨주고 스레드 close
}

void initailize() { // 고정된 자원으로 트라이를 초기화 시킴 

    //html에 있는 자원들을 모두 트라이에 삽입함.
    node = create_Node(); //트라이 루트노드 생성
    //html 부분
    char* str = "/index.html";
	insert_n(node, str, "text/html", strlen(str));
    str = "/total.cgi";
	insert_n(node, str, "text/html", strlen(str));
	str = "/contact.htm";
	insert_n(node, str, "text/html", strlen(str));
    str = "/home.htm";
	insert_n(node, str, "text/html", strlen(str));
    str = "/icontact.htm";
	insert_n(node, str, "text/html", strlen(str));
    str = "/ilinks.htm";
	insert_n(node, str, "text/html", strlen(str));
    str = "/ishop.htm";
	insert_n(node, str, "text/html", strlen(str));
    str = "/links.htm";
	insert_n(node, str, "text/html", strlen(str));
    str = "/main.htm";
	insert_n(node, str, "text/html", strlen(str));
    str = "/shop.htm";
	insert_n(node, str, "text/html", strlen(str));
    //image
    str = "/banner1.jpg";
	insert_n(node, str, "image/jpg", strlen(str));
    str = "/banner2.jpg";
	insert_n(node, str, "image/jpg", strlen(str));
    str = "/banner3.jpg";
	insert_n(node, str, "image/jpg", strlen(str));
    str = "/bullet1.jpg";
	insert_n(node, str, "image/jpg", strlen(str));
    str = "/bullet2.jpg";
	insert_n(node, str, "image/jpg", strlen(str));
    str = "/bullet3.jpg";
	insert_n(node, str, "image/jpg", strlen(str));
	str = "/images/05_01.gif";
	insert_n(node, str, "image/gif",strlen(str));
    str = "/images/05_02.gif";
	insert_n(node, str, "image/gif",strlen(str));
    str = "/images/05_03.gif";
	insert_n(node, str, "image/gif",strlen(str));
    str = "/images/05_04-05_07_over.gif";
	insert_n(node, str, "image/gif",strlen(str));
    str = "/images/05_04-over.gif";
	insert_n(node, str, "image/gif",strlen(str));
    str = "/images/05_04.gif";
	insert_n(node, str, "image/gif",strlen(str));
    str = "/images/05_05.gif";
	insert_n(node, str, "image/gif",strlen(str));
    str = "/images/05_06-over.gif";
	insert_n(node, str, "image/gif",strlen(str));
    str = "/images/05_06.gif";
	insert_n(node, str, "image/gif",strlen(str));
    str = "/images/05_07-over.gif";
	insert_n(node, str, "image/gif",strlen(str));
    str = "/images/05_07.gif";
	insert_n(node, str, "image/gif",strlen(str));
    str = "/images/05_08-over.gif";
	insert_n(node, str, "image/gif",strlen(str));
    str = "/images/05_08.gif";
	insert_n(node, str, "image/gif",strlen(str));
    str = "/images/05_09.gif";
	insert_n(node, str, "image/gif",strlen(str));
    str = "/images/05_10.gif";
	insert_n(node, str, "image/gif",strlen(str));
    str = "/images/bac_04.jpg";
	insert_n(node, str, "image/jpg",strlen(str));
    //favicon
    str = "/favicon.ico";
	insert_n(node, str, "image/x-icon", strlen(str));
}
int main (int argc, char **argv)
{
    if(argc != 3) { //program, path, port
        printf ("Use%s <path> <port>\n", argv[0]);
		exit(1); //인자의 갯수가 맞지않으면 프로그램 종료.
    }

    initailize(); //현재 html에 있는 자원들로 트라이 초기화
    signal(SIGPIPE, SIG_IGN); //broken pipe signal 무시
    struct client_info * ci; //클라이언트 정보 담는 변수
    root = argv[1]; //service directory 

    struct sockaddr_in sin, cli; //서버 소켓 주소와 클라이언트 소켓주소를 담을 변수 선언 

    int sd,clientlen = sizeof(cli); 
    int ns; 
    pthread_t tid; //thread 생성 시 사용.
    log_file = open("log.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666); //log.txt open

    if((sd = socket(AF_INET, SOCK_STREAM, 0)) == - 1) { //인터 네트워크, TCP, 프로토콜
        //sd에 서버 소켓 description 번호 저장
        perror("socket"); //socket 생성 실패시
        exit(1);
    }

    int optvalue = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optvalue, sizeof(optvalue)); //포트 재사용 허가
    
    memset((char *) &sin, '\0', sizeof(sin)); // '\0'으로 초기화
    sin.sin_family = AF_INET; // 인터 네트워크
    sin.sin_port = htons(atoi(argv[2])); //host to network --> byte ordering 
    sin.sin_addr.s_addr = inet_addr("0.0.0.0"); // string -> ip, open to anyway

    if(bind(sd, (struct sockaddr *)&sin, sizeof(sin))) { //socket에 port와 addr를 bind 시킴

        perror("bind"); //bind 실패시
        exit(1);
    } 
    
    if(listen(sd,100)) { //client 100명까지만 받음.

        perror("listen"); //listen 실패시
        exit(1);
    }
    if (pthread_mutex_init(&m_lock, NULL) != 0) //thread간 동기화할때 사용되는 mutex 초기화
    {
        perror("Mutex Init failure");
        return 1;
    }

    while(1) {

        ci = (struct client_info *) malloc(sizeof(struct client_info)); //클라이언트 정보 담을 구조체 메모리 할당 
        if((ns = accept(sd,(struct sockaddr *) &cli, &clientlen)) == -1) { //client가 들어올때 accept return 됨, client connect시 진입 시점
           //ns에 client socket description 번호 저장.
            perror("accept"); //accept 실패시
            exit(1);
        } //client accpet가 끝나면 반복
        ci->ns = ns; 
        strcpy(ci->ip, inet_ntoa(cli.sin_addr)); //ip 구조체를 문자열로 변환
        // client 정보에 소켓번호, ip 저장
        if(ci != NULL)
            pthread_create(&tid, NULL, sender, (void *)ci); // 스레드에 클라이언트 정보 인자로 넘김
    }

    close(log_file); //log.txt close
    return 0;
}
