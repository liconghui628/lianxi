#include <stdio.h>

int get_size(long long int num, int radix){
	int size = 0;

	if(radix <= 1)
		return 0;

	do{
		size++;
	}while(num /= radix);

	return size;
}

char* myftoa( double value, char* str, int slen, int decimals )
{
	int i = 0, j = 0, z = 0, mult = 1;
	long long int num; 
	char buf[10] = {'0','1','2','3','4','5','6','7','8','9'};
	char tmp ;
	int numsize;

	if( !str || slen <= 0 || decimals < 0 )
		return NULL;
	
	for(j = 0; j < decimals; j++)
		mult *= 10;

	num = (long long int)(value * mult);
	if(num < 0){
		str[i++] = '-';
		num = -num; 
	}
	
	numsize = get_size(num, 10);
	z = numsize;
	if(str[0] == '-')
		numsize += 1;
	do{
		if(i == numsize - decimals){
			str[i++] = '.';
			continue;
		}
		mult = 1;
		for(j = 1; j < z; j++)
			mult *= 10;
		z--;
		str[i++] = buf[num/mult];
		num %= mult;
	}while(i < slen-1 && num > 0);
	str[i] = 0;

	return str;
}

/*
int main()
{
	float f = 12.3456;
	char str[10];
	myftoa(f,str,4,4);
	printf("%s\n",str);
	return 0;
}*/
