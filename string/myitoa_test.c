#include <stdio.h>

#define MAX_SLEN	32
struct test
{
	int value;
	int radix, slen;
};

extern char *myitoa(int value, char *str, int slen, int radix);
int main(int argc, char *argv[])
{
	int i;
	char *pstr = NULL;
	char strval[MAX_SLEN];
	struct test stest[] = {
		{0, 10, MAX_SLEN},
		{520, 10, 2},
		{520, 10, 3},
		{-520, 10, 4},
		{-520, 10, 5},
		{-520, 16, MAX_SLEN},
		{0x10, 16, MAX_SLEN},
		{-520, 8, MAX_SLEN},
		{0666, 8, MAX_SLEN},
	};

	for(i = 0; i < sizeof(stest)/sizeof(stest[0]); i++) {
		pstr = myitoa(stest[i].value, strval, stest[i].slen, stest[i].radix);
		printf("real value:%d(hex:%x octet:%o) test value:%s(%d radix)\n",
				stest[i].value, stest[i].value, stest[i].value, strval[0] ? strval : "NULL", stest[i].radix);
	}

	return 0;
}
