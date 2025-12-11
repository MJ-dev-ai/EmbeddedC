#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>

void error_handling(char* message);

int main(int argc, char* argv[]) {
	int serv_sock;
	int clnt_sock;

	struct sockaddr_in serv_addr;
	struct sockaddr_in clnt_addr;
	socklen_t clnt_addr_size;

	char message[30] = "Hello World!";
	if (argc != 2) {
		printf("Usage: %s <PORT>\n", argv[0]);
		exit(1);
	}

	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	if (serv_sock < 0)
		error_handling("socket() error");
	
	int opt = 1;
	setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(atoi(argv[1]));

	if (bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1)
		error_handling("bind() error");

	if (listen(serv_sock, 5) < 0)
		error_handling("listen() error");

	clnt_addr_size = sizeof(clnt_addr);
	do {

		clnt_sock = accept(serv_sock, (struct sockaddr*) &clnt_addr, &clnt_addr_size);
		printf("clnt_sock : %d, new client connected\n",clnt_sock);
		if (clnt_sock < 0)
			error_handling("accept() error");
	
		do {
			int len = read(clnt_sock, message, sizeof(message) - 1);
			if (len > 0) {
				message[len] = '\0';
				if (strcmp(message, "quit") == 0) {
					fputs("Client socket closed.\n", stdout);
					break;
				}
				printf("[%s:%d] Message from client: \"%s\"\n", inet_ntoa(clnt_addr.sin_addr), atoi(argv[1]), message);
				write(clnt_sock, message, len);
			}
			if (len < 0) {
				error_handling("read() error");
			}
			if (len == 0) 
				continue;
		} while (1);
		close(clnt_sock);
	} while (1);

	sleep(1);
	close(serv_sock);

	return 0;
}

void error_handling(char* message) {
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
