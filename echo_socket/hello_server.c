#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

void error_handling(char *message);

int main(int argc, char *argv[])
{
	int str_len;
	int serv_sock;
	int clnt_sock;

	struct sockaddr_in serv_addr;
	struct sockaddr_in clnt_addr;
	socklen_t clnt_addr_size;

	char message[30]="Hello World!";

	if(argc!=2){
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
		//return 1;
	}

	serv_sock=socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(serv_sock < 0)
		error_handling("socket() error");

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_addr.sin_port=htons(atoi(argv[1]));

	if(bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr) )< 0 )
		error_handling("bind() error"); 

	if(listen(serv_sock, 5) < 0)
		error_handling("listen() error");

	clnt_addr_size=sizeof(clnt_addr);  

	do {
		clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_addr,&clnt_addr_size);
		printf("clnt_sock : %d, new client connected(%s)\n",clnt_sock,inet_ntoa(clnt_addr.sin_addr));
		if(clnt_sock < 0)
			error_handling("accept() error");  
		do {
			str_len = read(clnt_sock, message, sizeof(message)-1);
			if(str_len > 0)
			{
				message[str_len] = '\0'; //0,0x00
				printf("message : %s\n",message);
	//			write(clnt_sock, message, strlen(message));
				write(clnt_sock, message, str_len);
			}
			else if(str_len <= 0)
			{
				close(clnt_sock);	
				break;
			}
		} while(1);
		printf("clnt_sock : %d, client disconnected(%s)\n",clnt_sock,inet_ntoa(clnt_addr.sin_addr));
	} while(1);
	close(serv_sock);
	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
