/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *         GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *     - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

/* FUNCTION DECLARATIONS */
void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                    char *longmsg);



/* 
 * FUNCTION : DOIT 
 * Purpose : for handling single HTTP transaction
 * 			 server loop will get one request at a time from client
 * 			 during loop, it will call DOIT() to handle that request
 * 
 * 1. read request line & headers 
 * 		- if sth other than GET, error (strcasecmp() : 문자열 비교)
 * 2. parse URI from GET request 
 * 		- call parse_uri()
 * 		- is_static : flag for static / dynamic
 * 3. fail check : does file exist?
 * 		- stat() : 파일 상태 및 정보를 얻는 함수 return int
 * 4. fail check : is this regular file? & do you have permission?
 * 5. call function serve_xxxxx() based on is_static flag 
 *  
 */
void doit(int fd) {
	int is_static; 			// flag for static/dynamic
	struct stat sbuf;		// file status buffer 
	char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
	char filename[MAXLINE], cgiargs[MAXLINE];
	rio_t rio;

	/* Read request line and headers */
	Rio_readinitb(&rio, fd);
	Rio_readlineb(&rio, buf, MAXLINE);

	printf("Request headers : \n");
	printf("%s", buf);

	/* method, uri, version 에 정보 저장 */
	sscanf(buf, "%s %s %s", method, uri, version);

	/* fail check : tiny server only handles GET requests */
	if (strcasecmp(method, "GET")) {	
		clienterror(fd, method, "501", "Not implemented", 
				"Tiny does not implement this method");
		return;
	}
	read_requesthdrs(&rio);			// reads and ignores request hdrs.


	/* Parse URI from GET request */
	is_static = parse_uri(uri, filename, cgiargs);

	/* fail check : does file exist? */
	if (stat(filename, &sbuf) < 0) {
		clienterror(fd, filename, "404", "Not Found", 
				"Tiny couldn't find this file");
		return ; 
	}

	/* based on is_static, serve content
	 * fail check : is this regular file? & do you have permission?
	 * call serve- function 
	 */
	if (is_static) {	// STATIC
		if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
			clienterror(fd, filename, "403", "Forbidden", 
					"Tiny couldn't read the file");
			return ; 
		}
		serve_static(fd, filename, sbuf.st_size);
	} else {			// DYNAMIC
		if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
			clienterror(fd, filename, "403", "Forbidden", 
					"Tiny couldn't read the file");
			return ; 
		}
		serve_dynamic(fd, filename, cgiargs);
	}

}



/* ###################### MAIN #########################*/
int main(int argc, char **argv) {
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    /* Check command line args */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

	/* open listening socket */
    listenfd = Open_listenfd(argv[1]);

	/*  Server Loop : waiting for requests 
		accept server request : Accept
		connfd : connection descriptor 
		doit : perform transaction  
	*/
    while (1) {
		printf("Waiting for client requests...\n");
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);    // line:netp:tiny:accept
        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        doit(connfd);     // line:netp:tiny:doit
        Close(connfd);    // line:netp:tiny:close
		printf("Closed this request\n");
    }
}
/* MAIN END */
