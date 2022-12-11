/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

int main(void) {
	char *buf, *p;
	char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
	int n1 = 0, n2 = 0;

	/*  extract 2 arguments 
		getenv : calls arguments that client sent via url 
		QUERY_STRING : string where arguments are saved in 
	*/
	if ((buf = getenv("QUERY_STRING")) != NULL) {	//calls arguments & fail check 
		p = strchr(buf, '&');		//구분자 & index num 찾기 
		*p = '\0';					//구분자 NULL문자로 바꿈 
		
		strcpy(arg1, buf);			//첫번째 argument 저장
		strcpy(arg2, p + 1);		//구분자 p ptr 연산으로 다음 arg 위치 찾음 

		n1 = atoi(arg1);			// atoi : char to integer
		n2 = atoi(arg2);
	}

	/*  Make the response body 
		sprintf를 이용해 content에 여러 정보 저장해넣기 
	*/
	sprintf(content, "QUERY_STRING = %s", buf);
	sprintf(content, "Welcome to add.com: ");
	sprintf(content, "%sThe Internet addition portal.\r\n<p>", content);
	sprintf(content, "%sThe answer is : %d + %d = %d\r\n<p>",  
			content, n1, n2, n1 + n2);
	sprintf(content, "%sThanks for visiting!\r\n", content);

	/* Generate the HTTP response */
	printf("Connection : close\r\n");
	printf("Content-length: %d\r\n", (int)strlen(content));
	printf("Content-type : text/html\r\n\r\n");
	printf("%s", content);
	
	/* Clear output buffer & move buffered data to console */
	fflush(stdout);

	exit(0);
}
/* $end adder */
