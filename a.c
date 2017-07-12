#include <stdio.h>
int main() {
	char *a = "hello";
	char *b = "hello";
	printf("a:%p; b:%p %p\n",a,b,"hello");
	if(a == b)
		printf("YES\n");
	else
		printf("NO\n");
	return 0;
}
