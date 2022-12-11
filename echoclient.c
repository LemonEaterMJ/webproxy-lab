#include "csapp.h"

int main(int argc, char **argv) {
    int clientfd;   //client descriptor 
    char *host, *port, buf[MAXLINE];
    rio_t rio;      //reliable input-output

    /*  
        argc : main에 전달되는 정보 개수
        argv[] : main에 실제로 전달된 정보
    */
    if (argc != 3) {    
        fprintf(stderr, "usage : %s <host> <port>\n", argv[0]);
        exit(0);
    }

    host = argv[1];
    port = argv[2];

    clientfd = Open_clientfd(host, port);   //return client descriptor integer
    Rio_readinitb(&rio, clientfd);          
    //associate descriptor clientfd with read buffer and reset buffer  

    /* 
        루프를 돌며 
        1. standard input에서 text line을 받는다 
        2. send text line to server
        3. read echo line from server 
        4. print result (to standard output)
        Termination condition : ctrl + D or EOF status from stdin
    */
    while (Fgets(buf, MAXLINE, stdin) != NULL) {    // 1
        Rio_writen(clientfd, buf, strlen(buf));     // 2
        Rio_readlineb(&rio, buf, MAXLINE);          // 3
        Fputs(buf, stdout);                         // 4
    }
    Close(clientfd);        // close descriptor : sends EOF to server
    exit(0);
}