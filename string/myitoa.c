#include <stdio.h>
 
int get_size(unsigned int num, int radix){
	int size = 0;

	if(radix <= 1)
		return 0;

	do{
		size++;
	}while(num /= radix);

	return size;
}

char* myitoa( int num, char* str, int slen, int radix )
{
	int i = 0, j = 0, numsize = 0, temp = 0, mult = 1;
	unsigned int unum = 0;
	char buf[36] = {'0','1','2','3','4','5','6','7','8','9',
		'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z'};

	if( !str || slen <= 0 || radix <= 1 || radix > 32)
		return NULL;


	if( num < 0 && radix == 10){
		str[i++] = '-';
		unum = 0 - num;
	}
	else
		unum = (unsigned int)num;
	
	numsize = get_size(unum, radix);

	do{
		mult = 1;
		for(j = 1; j < numsize; j++)
			mult *= radix;
		numsize--;
		temp = unum / mult;
		unum %= mult;
		str[i++] = buf[temp];
		//printf("numsize = %d,str[%d] = %c\n",numsize,i-1,str[i-1]);
	}while(i < slen-1 && numsize >= 1);

	str[i] = 0;
	return str;
}

/*
int main()
{
	int num = -125;
	char str[10] = {0};
	char *str1;
	printf("整数：%d\n",num);
	str1 = myitoa(num, str, 10, 2);
	printf("二进制：%s\n",str);
	str1 = myitoa(num, str, 10, 8);
	printf("八进制：%s\n",str);
	str1 = myitoa(num, str, 10, 10);
	printf("十进制：%s\n",str);
	str1 = myitoa(num, str, 10, 16);
	printf("十六进制：%s\n",str);
	return 0;
}
*/
