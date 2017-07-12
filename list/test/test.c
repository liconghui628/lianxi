#include <stdio.h>
#include <stdlib.h>
#include "list.h"

#define SIZE	6
#define MAX_BUFFSIZE	1024

static char stabuf[6][10] = {"a","b","c","d","e","f"};

char *mystrncpy(char *dest, const char *src, int len)
{
	int i = 0;
	while(*src && i < len){
		dest[i] = src[i];
		i++;
	}
	dest[i] = 0;
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

int main()
{
	int i, ret;
	NODE *list, *node;
	char *buf[6];
	char *olddata;

	for(i = 0; i<6; i++){
		buf[i] = calloc(1,MAX_BUFFSIZE);
		if(!buf[i])
			return -1;
		printf("buf[%d]:%p\n",i,buf[i]);
		mystrncpy(buf[i], stabuf[i], mystrlen(stabuf[i]));		
	}

	list = list_create();
	if(!list){
		printf("list_create() error;\n");
		return -1;
	}
	printf("list_create() success\n");

	for(i = 0; i < SIZE; i++){
		ret = list_insert(list, buf[i], i);
		printf("list_insert %d success, data:%s\n",i,(char*)list_getdata(list,i));
		if(ret < 0){
			printf("list_insert() error\n");
			return -1;
		}
		ret = list_size(list);
		printf("list size = %d\n",ret);
	}

	for(i = 0; i < SIZE - 3; i++){
		olddata = list_remove(list, 0);
		if(ret < 0){
			printf("list_remove() %d  error\n",i);
			return -1;
		}
		printf("list_remove %d %s success\n",i,olddata);
		ret = list_size(list);
		printf("list size = %d\n",ret);
	}

	char *chstr = calloc(1,10);
	mystrncpy(chstr,"123456",mystrlen("123456"));
	for(i = 0; i < SIZE-3; i++){
		olddata = list_modif(list, chstr,i);
		if(olddata == NULL){
			printf("list_modif %d error\n",i);
			return -1;
		}
		printf("list_modif node[%d] %s -> %s success\n",i, olddata, (char*)list_getdata(list,i));
		free(olddata);
	}

	ret = list_destroy(list);
	if(ret < 0){
		printf("list_destroy() error\n");
		return -1;
	}
	printf("list_destroy() success\n");
	return 0;
}
