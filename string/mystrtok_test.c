#include <stdio.h>
extern char *mystrtok(char *str, const char *delim);
int main(int argc, char *argv[])
{
	int i, j, tcnt;
	char *pstr = NULL;
	char strtest1[][64] = {
		{"=aaa=bbb=ccc==="},{"="},
		{"=aaa=bbb=ccc==="},{"=="},
		{"xx:yyyyy=zzzz"},{"z"},
		{"zzz"},{"zz"},
		{"zz"},{"z"},
		{"z"},{"zz"},
		{"z"},{"z"},
		{"zz"},{"zz"},
		{"0"},{"."},
		{"00"},{".."},
		{"0"},{".."},
		{"000"},{".."},
		{"192.168.1.100"},{"."},
		{"xxyyzz"},{"x"},
		{"xx yy zz"},{" "},
		{"xx yy zz"},{"  "},
		{"aaa=vv====aaaadvv-dd===="},{"==="},
	};
	char strtest2[][64] = {
		{"xx=aa:bb:cc"},{"="},{":"},
		{"conv=notrunc,sync"},{"="},{","},
		{"conv  notrunc,sync"},{"  "},{","},
	};

	if(argc < 3) {
		printf("*************************test1***************************\n");
		tcnt = sizeof(strtest1)/sizeof(strtest1[0])/2;
		for(i = 0; i < tcnt; i++) {
			printf("test string:[%s] delim:[%s]\n", strtest1[i*2], strtest1[i*2+1]);
			pstr = mystrtok(strtest1[i*2], strtest1[i*2+1]);
			if(pstr)
				printf("result0:[%s]\n", pstr);
			for(j = 1; pstr; j++) {
				pstr = mystrtok(NULL, strtest1[i*2+1]);
				if(pstr)
					printf("result%d:[%s]\n", j, pstr);
				if(j >= 9) {
					printf("something maybe wrong, force quit!\n");
					break;
				}
			}
			if((i + 1) != tcnt) printf("\n");
		}

		printf("\n\n*************************test1***************************\n");
		tcnt = sizeof(strtest2)/sizeof(strtest2[0])/3;
		for(i = 0; i < tcnt; i++) {
			printf("test string:[%s] delim1:[%s] delim2:[%s]\n", strtest2[i*3], strtest2[i*3+1], strtest2[i*3+2]);
			pstr = mystrtok(strtest2[i*3], strtest2[i*3+1]);
			if(pstr)
				printf("result0:[%s]\n", pstr);
			for(j = 1; pstr; j++) {
				pstr = mystrtok(NULL, strtest2[i*3+2]);
				if(pstr)
					printf("result%d:[%s]\n", j, pstr);
				if(j >= 9) {
					printf("something maybe wrong, force quit!\n");
					break;
				}
			}
			if((i + 1) != tcnt) printf("\n");
		}
	} else {

	}

	return 0;
}
