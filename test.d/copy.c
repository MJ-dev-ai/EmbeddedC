#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

int main(int argc, char** argv) {
	int n, in, out;
	const int bufsiz = 100;
	char buf[bufsiz];

	/* 명령행 인수로 복사할 파일명이 없는 경우에 에러를 출력하고 종료한다. */
	/*for (int idx = 0; idx < argc; idx++) {
		printf("idx: %d, argv: %p, argv[%d]: %s\n", idx, (argv + idx), idx, argv[idx]);
		for (int j = 0; j < strlen(argv[idx]); j++) {
			printf("\ti: %d, j: %d, argv: %p, argv[%d][%d]:%c\n", idx, j, (argv + idx), idx, j, argv[idx][j]);
		}
	}
	int idx = 0;
	while (*(argv + idx) != NULL) {
		printf("idx: %d, argv: %s\n", idx++, *(argv + idx));
	}*/

	if (argc < 3) {
		//write(2, "Usage : copy file1 file2\n", 25);
		fputs("Usage : copy file1 file2\n", stdout);
		return 1;
	}
	
	/* 복사의 원본이 되는 파일을 읽기 모드로 연다. */
	if ((in = open(argv[1], O_RDONLY)) < 0) {
		perror(argv[1]);
		return 2;
	}

	/* 복사할 결과 파일을 쓰기 모드 (새로운 파일 생성 | 기존에 파일 내용 지움)로 연다. */
	if ((out = open(argv[2], O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR)) < 0) {
		perror(argv[2]);
	}

	/* 원본 파일의 내용을 읽어서 복사할 파일에 쓴다. */
	/*while ((n = read(in, buf, sizeof(buf))) > 0)
		write(out, buf, n);*/
	do {
		n = read(in, buf, sizeof(buf));
		printf("%d\n", n);
		if (n < 0) {
			perror("read()");
			break;
		}
		else if (n == 0) {
			fputs("Done ... \n", stdout);
			break;
		}
		else write(out, buf, n);
	} while (1);
	
	/* 열린 파일들을 닫는다. */
	close(out);
	close(in);
	
	/* 프로그램의 정상 종료 시 0을 반환한다. */
	return 0;
}
