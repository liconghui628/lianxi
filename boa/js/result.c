#include <stdio.h>  
#include <stdlib.h>
#include <string.h>

#define RESULT_FILE		"/var/www/result"

int main(void){  
	FILE *fd;
	char result[10] = {0};
	printf("Content-type: text/html\n\n");  
	fd = fopen(RESULT_FILE, "r");
	if(fd == NULL)
		return -1;
	fgets(result, sizeof(result), fd);

	if(!strncmp(result, "success", strlen("success")))
		printf("success");
	else if(!strncmp(result, "failed", strlen("failed")))
		printf("failed");

	fclose(fd);
    return 0;  
}   
