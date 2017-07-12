#include <stdio.h>

#define MAX_SLEN	32
struct test
{
	double value;
	int slen;
};

extern char *myftoa(double value, char *str, int slen, int decimals);
int main(int argc, char *argv[])
{
	int i;
	char *pstr = NULL;
	char strval[MAX_SLEN];
	struct test stest[] = {
		{0.0, MAX_SLEN},
		{5.354, MAX_SLEN},
		{9.9999999999, MAX_SLEN},
		{555.555555555, MAX_SLEN},
		{5.355, MAX_SLEN},
		{-5.345, MAX_SLEN},
		{-5.999, MAX_SLEN},
	};

	printf("zbs marked, %lf %-5.20f %f\n", 9.9999999999, 555.555555555, 5.555554444);
	for(i = 0; i < sizeof(stest)/sizeof(stest[0]); i++) {
		pstr = myftoa(stest[i].value, strval, stest[i].slen, 6);
		printf("real value:%lf  test value:%s\n", stest[i].value, strval[0] ? strval : "NULL");
	}

	return 0;
}
