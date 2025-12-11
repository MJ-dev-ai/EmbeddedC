#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

void error_handling(char *message);

int main(int argc, char* argv[])
{
	int sock;
	struct sockaddr_in serv_addr;
	char message[10]="12345";
	int str_len;

	if(argc!=3){
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}

	sock=socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(sock < 0)
		error_handling("socket() error");

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
	serv_addr.sin_port=htons(atoi(argv[2]));

	if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) 
		error_handling("connect() error!");

	fputs("문자열을 입력하세요(quit:종료) : ",stdout);
	do {	
		fgets(message,sizeof(message),stdin);
//		message[strlen(message)-1] = '\0'; //'\n'제거 
		char *ip = strstr(message,"\n");
		if(ip != NULL)
				*ip = '\0';
//		printf("message : %s\n",message);
		if(!strcmp(message,"quit")) {
			break;
		}
		if(!strlen(message))
			continue;
		str_len=write(sock, message, strlen(message));
		str_len=read(sock, message, sizeof(message)-1);
		printf("str_len:%d\n",str_len);
		if(str_len < 0)
			error_handling("read() error!");
	
		message[str_len] = '\0';
		printf("Message from server: %s \n", message);  
	} while(1);

	close(sock);
	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
