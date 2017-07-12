#include <stdio.h>  
#include <stdlib.h>
#include <string.h>

#define FILE_NAME		"/home/app/ddc.db"
#define RESULT_FILE		"/var/www/result"


int getfile_start_and_end(char *filebuff, int strsz, int *start, int *end)
{
	int i = 0;
	while(1){
		if(i + strlen("Content-Type: ") >= strsz)	
			return -1;
		if(!strncmp(&filebuff[i], "Content-Type: ", strlen("Content-Type: "))){
			i += strlen("Content-Type: ");
			//printf("====%d,filebuff[%d]=%c ",i,i,filebuff[i]);
			while(filebuff[i] != '\n')
				i++;
			while(filebuff[i] == '\n' || filebuff[i] == '\r')
				i++;
			//printf("===%d,filebuff[%d]=%c ",i,i,filebuff[i]);
			*start = i;
			while(1){
				if(i + strlen("--------") >= strsz)
					return -1;
				if(!strncmp(&filebuff[i], "--------", strlen("--------"))){
					//printf("___filebuff[%d]=%hhd ",i-4,filebuff[i-4]);
					//printf("___filebuff[%d]=%hhd ",i-3,filebuff[i-3]);
					//printf("___filebuff[%d]=%hhd ",i-2,filebuff[i-2]);
					//printf("___filebuff[%d]=%hhd ",i-1,filebuff[i-1]);
					//printf("___filebuff[%d]=%c ",i,filebuff[i]);
					if(filebuff[i-1] == '\n' || filebuff[i-1] == '\r')
						i--;
					if(filebuff[i-1] == '\n' || filebuff[i-1] == '\r')
						i--;
					*end = i;
					break;
				}
				i++;
			}
			break;
		}
		i++;
	}
	if(*end <= *start)
		return -1;
	return 0;
}

int write_to_file(char *filebuff, int start, int end)
{
	int ret;
	FILE *fd;
	int size = end -start;
	fd = fopen(FILE_NAME, "wb");
	if(fd == NULL)
		return -1;
	ret = fwrite(&filebuff[start], 1, size, fd);
	if(ret < size){
		fclose(fd);
		return -1;
	}
	return 0;
}

int set_upfile_result(char *result)
{
	int ret;
	FILE *fd;
	fd = fopen(RESULT_FILE, "wb");
	if(fd == NULL)
		return -1;
	return fputs(result, fd);
}

int main(void){  
    int len, strsz, start, end, i, ret;  
    char *lenstr, *content_type;  
    char m[10],n[10];
	char *filebuff = calloc(1, 1024 * 1024);
	printf("Content-type: text/html\n\n");  
	if(!filebuff){
		//printf("<p align=%s><span style=%s>%s</span>&nbsp;</p>","center","font-size:20px;color:red","Update File failed !");
		return 0;
	}
	//printf("%s()",__func__);
    lenstr=getenv("CONTENT_LENGTH");
    if(lenstr == NULL)  
        printf("lenstr == NULL");  
    else{  
        len=atoi(lenstr);  
		//printf("%d",len);
		
        strsz = fread(filebuff,1,len,stdin);
		//printf("strsize=%d",strsz);
		//for( i = 0; i< strsz; i++)
		//	printf("%c",filebuff[i]);

		ret = getfile_start_and_end(filebuff, strsz, &start, &end);
		//printf("ret = %d ",ret);
		if(ret < 0){
			//printf("<p align=%s><span style=%s>%s</span>&nbsp;</p>","center","font-size:20px;color:red","Update File failed !");
			set_upfile_result("failed");
			free(filebuff);
			return -1;
		}
		//printf("start=%d,end=%d",start,end);
		//for( i = start; i< end; i++)
		//	printf("%c",filebuff[i]);

		ret = write_to_file(filebuff, start, end);
		if(ret < 0){
			set_upfile_result("failed");
			//printf("<p align=%s><span style=%s>%s</span>&nbsp;</p>","center","font-size:20px;color:red","Update File failed !");
			free(filebuff);
			return -1;
		}
    }  
	set_upfile_result("success");
	//printf("<p align=%s><span style=%s>%s</span>&nbsp;</p>","center","font-size:20px;color:green","Update File success !");
    fflush(stdout);  
	free(filebuff);
    return 0;  
}   
