#include <stdio.h>  
#include <stdlib.h>
#include <string.h>

#define RESULT_FILE		"/var/www/result"

void upfile_result(void){  
	FILE *fd;
	char result[10] = {0};
	fd = fopen(RESULT_FILE, "r");
	if(fd == NULL){
		printf(" ");
		return;
	}
	fgets(result, sizeof(result), fd);

	if(!strncmp(result, "success", strlen("success")))	// up file success
		printf("更新成功！");
	else if(!strncmp(result, "failed", strlen("failed")))//up file failed
		printf("更新失败，请重试！");							
	else
		printf(" ");									// file have not updated
	fclose(fd);

	//truncate the file
	fd = fopen(RESULT_FILE, "w");
	if(fd)
		close(fd);
}   
