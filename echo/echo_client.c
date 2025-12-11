#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

void error_handling(char* message);

int main(int argc, char* argv[]) {
	int sock;
	struct sockaddr_in serv_addr;
	char message[30];
	int str_len;

	if (argc != 3) {
		printf("Usage: %s <IP> <PORT>\n", argv[0]);
		exit(1);
	}
	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock < 0)
		error_handling("socket() error");

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));

	if (connect(sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1) {
		error_handling("connect() error!");
	}

	do {
		fputs("문자열을 입력하세요 (quit: 종료): ", stdout);
		fgets(message, sizeof(message), stdin);
		if (strcmp(message, "\n") == 0) fgets(message, sizeof(message), stdin);
		int l = strlen(message);
		if (message[l - 1] == '\n')
			message[l - 1] = '\0';
		l = strlen(message);
		printf("strlen: %d\n", l);
		if (l > 0)
			write(sock, message, l);
			str_len = read(sock, message, sizeof(message));
			if (strcmp(message, "quit") == 0) break;
			printf("[%s:%d] Message from server: \"%s\"\n", argv[1], atoi(argv[2]), message);
		if (l == 0) continue;
	} while (1);

	close(sock);
	return 0;
}

void error_handling(char* message) {
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
