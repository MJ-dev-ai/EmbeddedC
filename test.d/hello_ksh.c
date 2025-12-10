#include <stdio.h>
#include <stdbool.h>

#define MAX_SIZE 1000

void strcpy_usr(char* str1, char* str2) {
	int idx = 0;
	while (true) {
		if (str2[idx] == '\0') {
			str1[idx] = str2[idx];
			break;
		}
		else if (idx >= 1000) break;
		else {
			str1[idx] = str2[idx];
			idx += 1;
		}
	}
}

int main(int argc, char** argv) {
#if 0
	char ch = 'A'; // 8bit : -128 ~ 127
	short sh = 1234;
	int in = 1234567;
	long lo = 1;
	long long ll = 2;
	float fl = 3.14;
	double db = 1.234;
	
	printf("&ch: %p\n", &ch);
	printf("&sh: %p\n", &sh);
	printf("&in: %p\n", &in);
	printf("&lo: %p\n", &lo);
	printf("&ll: %p\n", &ll);
	printf("&fl: %p\n", &fl);
	printf("&db: %p\n", &db);
	printf("%p\n", "Hello World!\n");
	printf("%p\n", "Hello World!\n");
	printf("%c\n", "Hello World!\n"[1]);

#endif
	char str[14] = "Hello World!\n";
	printf("str: %s", str);
	char str1[20];
	char str2[10];
	str2[0] = 'H';
	str2[1] = 'i';
	str2[2] = 'v';
	str2[3] = 'e';
	str2[4] = '\0';
	strcpy_usr(str1, str2);
	printf("%s\n", str1);
	return 0;
}
