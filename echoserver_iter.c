#include "csapp.h"

void echo(int connfd);

int main(int argc, char **argv) {
    int listenfd, connfd;       // descriptors
    socklen_t clientlen;        // socket length
    struct sockaddr_storage clientaddr; // enough space for any address
    char client_hostname[MAXLINE], client_port[MAXLINE];

    if (argc != 2) {    // 전달된 정보는 두개여야한다 
        fprintf(stderr, "usage : %s <port>\n", argv[0]);
        exit(0);
    }

    /* making listening descriptor */
    listenfd = Open_listenfd(argv[1]);

    /* Loop for each iteration (one client at a time)
        0. wait for a connection request from client
        1. client connection here! 
        2. print domain name & port of connected client
        3. calls echo (for servicing client)
        4. echo returns after EOF status from client
        5. terminate used connected descriptor(connfd)
        client and server must both close descriptors(connfd & clientfd)
        6. server gets EOF from client and terminates. 
    */
    while(1) {
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);   // descriptor
        Getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAXLINE, 
                    client_port, MAXLINE, 0);
        printf("Connected to (%s, %s)\n", client_hostname, client_port);    
        echo(connfd);
        Close(connfd);
    }
    

}

void echo(int connfd) {
    size_t n; 
    char buf[MAXLINE];
    rio_t rio;

    Rio_readinitb(&rio, connfd);

    /* Loop 
        until Rio_readlineb gets EOF status

        read text line (Rio_readlineb)
        write line to connection descriptor
    */
    while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
        printf("server received %d bytes\n", (int)n);
        Rio_writen(connfd, buf, n);
    }
}