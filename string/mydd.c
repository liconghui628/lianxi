#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

struct ddstu{
	char *flag;
	char *param;
	int flagsz;
};

int mystrlen(const char *str)
{
	int i = 0;
	while(*str){
		i++;
		str++;
	}
	return i;
}

int mystrncmp(const char *str1, const char *str2, int count)
{
	int i;
	if(!str1 || !str2 || mystrlen(str1) < count || mystrlen(str2) < count)
		return -2;
	for(i = 0; i < count; i++){
		if(str1[i] < str2[i])
			return -1;
		else if(str1[i] > str2[i])
			return 1;
	}
	return 0;
}

char* mystrtok(const char *str, char delim)
{
	int i = 0;
	int len = mystrlen(str);
	char *retstr = calloc(1, len);
	while(*str){
		if(*str == delim){
			str++;
			break;
		}
		str++;
	}
	while(*str){
		retstr[i++] = *str;
		str++;
	}
	return retstr;
}

void decodeParam(char *argv[], struct ddstu *dd, int argvCnt, int ddCnt)
{
	int i, j;
	for(i = 1; i < argvCnt; i++){
		for(j = 0; j < ddCnt; j++){
			if(!mystrncmp(argv[i], dd[j].flag, dd[j].flagsz)){
				dd[j].param = mystrtok(argv[i],'=');
				printf("%s = %s  \n",dd[j].flag, dd[j].param);
			}
		}
	}
}

long long int mystrtol(char *str)
{
	int len, i, j, k;
	long long retvalue = 0, mutip = 1;
	if(!str)
			return -1;
	len = mystrlen(str);
	for( i = 0; i < len; i++){
		if(!(str[i]>= '0' && str[i] <= '9' 
				|| (str[i] == 'K' || str[i] == 'k' 
					|| str[i] == 'M' || str[i] == 'm' 
					|| str[i] == 'G' || str[i] == 'g')))
			return -1;
		if(str[i] == 'K' || str[i] == 'k' 
				|| str[i] == 'M' || str[i] == 'm' 
				|| str[i] == 'G' || str[i] == 'g'){
			i++;
			break;
		}
	}
	if(str[i] != 0)
		return -1;

	if(str[i-1] >= '0' && str[i-1] <= '9'){
		for(j = 0; j < i; j++){
			mutip = 1;
			for(k = j; k < i-1; k++)
				mutip*=10;
			retvalue += (str[j] - ('0'-0)) * mutip;
		}
	}
	else if(str[i-1] == 'K' || str[i-1] == 'k'){
		for(j = 0; j < i-1; j++){
			for(k = j; k < j; k++)
				mutip*=10;
			retvalue += (str[j] - ('0'-0)) * mutip;
		}
		retvalue *= 1024;
	}
	else if(str[i-1] == 'M' || str[i-1] == 'm'){
		for(j = 0; j < i-1; j++){
			for(k = j; k < i-1; k++)
				mutip*=10;
			retvalue += (str[j] - ('0'-0)) * mutip;
		}
		retvalue *= 1024 * 1024;
	}
	else if(str[i-1] == 'G' || str[i-1] == 'g'){
		for(j = 0; j < i-1; j++){
			for(k = j; k < i-1; k++)
				mutip*=10;
			retvalue += (str[j] - ('0'-0)) * mutip;
		}
		retvalue *= 1024 * 1024 *1024;
	}
	else{
		retvalue = -1;
	}
	return retvalue;
}

int main(int argc,char *argv[])
{
#define paramCnt	7 
	int sync = 0, notrunc = 0;
	int infd, outfd;
	long long int bs, count, rsize, wsize, ibs, obs;
	char *buff;
	struct stat statbuff;

	struct ddstu dd[paramCnt] = {
		{"if", NULL, 2},
		{"of", NULL, 2},
		{"bs", NULL, 2},
		{"count", NULL, 5},
		{"conv", NULL, 4},
		{"ibs", NULL, 3},
		{"obs", NULL, 3},
	};
	if(argc < 3){
		printf("param error\n");
		return -1;
	}

	//解析参数
	decodeParam(argv, dd, argc, paramCnt);

	// 得到文件处理方法
	if(!mystrncmp(dd[4].param,"sync",4)){
		sync = 1;
	}
	//输入文件
	if(dd[0].param){
		infd = open(dd[0].param,O_RDONLY);
		if(infd == -1){
			printf("open %s faild\n",dd[0].param);
			return -1;
		}
	}

	//得到输入文件信息
	fstat(infd,&statbuff);
	buff = calloc(1,statbuff.st_size);

	//输出文件
	if(dd[1].param){
		outfd = open(dd[1].param,O_RDWR|O_CREAT|O_TRUNC, statbuff.st_mode);
		if(outfd == -1){
			printf("open %s faild\n",dd[1].param);
			return -1;
		}
	}

	//得到读入和写入字节大小
	bs = mystrtol(dd[2].param);
	ibs = mystrtol(dd[5].param);
	obs = mystrtol(dd[6].param);
	count = mystrtol(dd[3].param);
	printf("bs=%lld, ibs=%lld, obs=%lld, count=%lld\n",bs,ibs,obs,count);
	if(bs != -1 && count != -1){
		rsize = (bs * count) < statbuff.st_size ? (bs * count) : statbuff.st_size;
		wsize = bs * count;
	}
	else if((ibs != -1 || obs != -1) && count != -1){
		if(ibs != -1)
			rsize = (ibs * count) < statbuff.st_size ? (ibs * count) : statbuff.st_size;
		else
			rsize = 0;

		if(obs != -1)
			wsize = (obs * count) ;
		else
			wsize = rsize;
	}
	else{
		printf("param error !\n");
		return -1;
	}

	printf("rsize = %lld, wsize = %lld\n",rsize,wsize);
	//读输入文件写到输出文件
	int rdcnt = read(infd, buff, rsize);
	if(!sync)
		wsize = wsize < rdcnt ? wsize : rdcnt;
	int wrcnt = write(outfd,buff,wsize);
	printf("read %s %d bytes; write %s %d bytes\n",dd[0].param,rdcnt,dd[1].param,wrcnt);
	return 0;
}

