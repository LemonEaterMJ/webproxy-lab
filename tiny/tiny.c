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


	/* Parse URI from GET request 
	 * filename will filled with xxx.html for appropriate request 
	 * gets arguments for cgi and fills it to cgiargs
	 */
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



/*
 *  FUNCTION : clienterror 
 * 	called when other functions check error 
 *  used for displaying ERROR
 * 	1. build HTTP response : with sprintf (html 형식)
 *  2. print HTTP response : with rio_writen
 * 
 */
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                    char *longmsg) {
	char buf[MAXLINE], body[MAXBUF];

	/* Build HTTP response body */
	sprintf(body, "<html><title>Tiny Error</title>");
	sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
	sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
	sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
	sprintf(body, "%s<hr><em>The Tiny Web Server</em>\r\n", body);

	/* print HTTP Response */
	sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-type: text/html\r\n");
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
	Rio_writen(fd, buf, strlen(buf));
	Rio_writen(fd, body, strlen(body));
}



/*
 * FUNCTION : read_requesthdrs 
 * in this version, tiny reads but ignores headers 
 * empty text line will terminate request headers 
 * 		check empty text line with \r\n
 */
void read_requesthdrs(rio_t *rp) {
	char buf[MAXLINE];

	Rio_readlineb(rp, buf, MAXLINE);	//always read first request line

	/* check for emtpy line(ternimation) \r\n 
		if not empty, just print 
	*/ 
	while(strcmp(buf, "\r\n")) {		
		Rio_readlineb(rp, buf, MAXLINE);
		printf("%s", buf);
	}
	return ;
}






/* 
 * FUNCTION : parse_uri
 * parse uri string for cgi-bin -> RETURN flag for static(1)/dynamic(0)
 * 		if cgi-bin : dynamic content
 * 
 * 1. parse URI into filename and CGI argument string 
 * depending on static/dynamic, 
 * 2. if static, clear argument string 
 * 3. convert URI into pathname xxxx.html
 * 4. if end with / : call for home, URI home.html
 * 5. if dynamic, pointer cgiargs points to args 
 * 6. convert remaining URI inro pathname xxxx.html
 */
int parse_uri(char *uri, char *filename, char *cgiargs) {
	char *ptr;

	// 1. find cgi-bin
	if (!strstr(uri, "cgi-bin")) {	// STATIC 
		strcpy(cgiargs, "");		// clear arguments
		strcpy(filename, ".");	
		strcat(filename, uri);		// convert URI to pathname
		
		if (uri[strlen(uri) - 1] == '/') {	// if ends with /
			// convert URI to default home.html
			strcat(filename, "home.html");
		}
		if (!strcmp(uri, "/adder")) {
			strcpy(filename, "adder.html");
		}
		return 1;
	} else {						// DYNAMIC
		// search for arg separator ?
		ptr = index(uri, '?');
		if (ptr) {
			strcpy(cgiargs, ptr + 1);
			*ptr = '\0';
		} else {	// no ? separator 
			strcpy(cgiargs, "");
		}

		// convert URI to pathname 
		strcpy(filename, ".");
		strcat(filename, uri);
		return 0;
	}

	return 0;
}



/*
 * FUNCTION : serve_static 
 * called from doit() 
 * serves static content 
 * 1. parse filename and filetype with get_filetype()
 * 2. send response headers to client(end with empty line)
 * 3. open 'filename' and get descriptor 
 * 4. malloc file to virtual memory
 * 5. close file(since malloced, not needed)
 * 6. write to fd(the one we send to client)
 * 7. free malloced source file pointer
 * 
 */
void serve_static(int fd, char *filename, int filesize) {
	int srcfd;		// source file descriptor 
	// source pointer
	char *srcp, filetype[MAXLINE], buf[MAXBUF];

	/* parse filename and filetype */
	get_filetype(filename, filetype);

	/* prepare response hdrs in buffer */
	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
	sprintf(buf, "%sConnection: close\r\n", buf);
	sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
	sprintf(buf, "%sContent-Type: %s\r\n\r\n", buf, filetype);

	// write buf to fd (file descriptor)
	Rio_writen(fd, buf, strlen(buf));

	/* print response hdrs */
	printf("Response headers: \n");
	printf("%s", buf);

	/* Send response body to client */
	// open 'filename' and get descriptor source file descriptor
	srcfd = Open(filename, O_RDONLY, 0);

	/* malloc file to virtual memory(source ptr) */
	// srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
	srcp = (char *)malloc(filesize *sizeof(char));
	Rio_readn(srcfd, srcp, filesize);
	Close(srcfd);						// Close file 
	Rio_writen(fd, srcp, filesize);		// write srcp to fd
	// Munmap(srcp, filesize);				// free
	free(srcp);
}




/* 
 * FUNCTION : get_filetype()
 * parse filename and filetype 
 * this version of tiny supports 6 filetypes : 
 * 		html : text/html	
 * 		image : image/png, image/jpeg
 * 		gif : image/gif
 * 		text : text/plain
 * 		mpg : video/mpg
 */
void get_filetype(char *filename, char *filetype) {
	if (strstr(filename, ".html")) {
		strcpy(filetype, "text/html");
	} else if (strstr(filename, ".gif")) {
		strcpy(filetype, "image/gif");
	} else if (strstr(filename, ".png")) {
		strcpy(filetype, "image/png");
	} else if (strstr(filename, ".jpg")) {
		strcpy(filetype, "image/jpeg");
	} else if (strstr(filename, ".mp4")) {
		strcpy(filetype, "video/mp4");
	} else {	// plain text
		strcpy(filetype, "text/plain");
	}
}




/* 
 * FUNCTION : serve_dynamic
 * called from doit()
 * serves dynamic content by forking child process
 * 
 * 1. Send success response line & server header to client  
 * 		other HTTP responses will be sent by cgi
 * 		notice there is no print process at this stage
 * 2. fork a new child process
 * 3. initialize QUERY_STRING with cgi arg (from request uri)
 * 4. redirect child stdout to connected file descriptor 
 * 5. load & run cgi program(uses QUERY_STRING)
 * 		child process outputs go directly to client 
 * 6. parent waiting for child to finish(and then kills it)
 */
void serve_dynamic(int fd, char *filename, char *cgiargs) {
	// connected file descriptor fd
	char buf[MAXLINE], *emptylist[] = {NULL};

	/* Prepare first part of HTTP response */
	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	Rio_writen(fd, buf, strlen(buf));	// write buf to fd 
	sprintf(buf, "Server: Tiny Web Server\r\n");
	Rio_writen(fd, buf, strlen(buf));

	/* fork new child process */
	// if successful, return 0
	if (Fork() == 0) {
		/* Real server would set all CGI vars here*/
		setenv("QUERY_STRING", cgiargs, 1);		

		/* Redirect stdout to client*/
		Dup2(fd, STDOUT_FILENO);

		/* Run cgi program */
		Execve(filename, emptylist, environ);
	}
	Wait(NULL);		
	//parent waiting for child to finish(and then kills it)
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
