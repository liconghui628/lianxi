#include <stdio.h>
#include <stdlib.h>

char *mystrcpy(char *dest, const char *src){
	while(*src)
		*dest++ = *src++;
	*dest = 0;
	return dest;
}

int mystrlen(const char *str)
{
	int i = 0;
	while(*str){
		i++;
		str++;
	}
	return i;
}

char *mystrtok(char *str, const char *delim)
{
	int i, j, l;
	int firstflag = 1;
	char *retstr;
	static int len;
	static char *strbuf, *strbuf1;

	if(str){
		len = mystrlen(str);
		strbuf1 = calloc(1, mystrlen(str) + 1);
		mystrcpy(strbuf1, str);
		strbuf = strbuf1;
	}
	else
		firstflag = 0;

	if(len == 0){
		free(strbuf1);
		return NULL;
	}

	for(i = 0; i < len; i++){
		for(j = 0; j < mystrlen(delim); j++){
			if(strbuf[i] == delim[j])
				strbuf[i] = 0;
		}
	}
	//去掉字符串前的‘\0’字符
	i = 0;
	if(!strbuf[0] && firstflag){
		while(!strbuf[i]){
			len -= 1;
			if(len == 0){
				free(strbuf1);
				return NULL;
			}
			i++;
		}
	}
		
	retstr = &strbuf[i];
	l = mystrlen(retstr);
	while(!strbuf[l+i]){
		if(l == len)
			break;
		l++;
	}
	strbuf = &strbuf[l+i];
	len -= l;
	return retstr;
}

