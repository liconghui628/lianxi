#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  
#define BUF_SIZE 	1024 
#define PATH_SIZE	128

int getPidByName(char* task_name)
{
	int pid = -1;
	DIR *dir;
	struct dirent *ptr;
	FILE *fp;
	char filepath[PATH_SIZE];
	char cur_task_name[PATH_SIZE];
	char buf[BUF_SIZE];

	dir = opendir("/proc");
	if (NULL != dir)
	{
		while ((ptr = readdir(dir)) != NULL)
		{
			if ((strcmp(ptr->d_name, ".") == 0) || (strcmp(ptr->d_name, "..") == 0))             
				continue;
			if (DT_DIR != ptr->d_type) 
				continue;
			pid = atoi(ptr->d_name);
			if(pid <= 0)
				continue;

			snprintf(filepath, PATH_SIZE, "/proc/%s/status", ptr->d_name);
			fp = fopen(filepath, "r");
			if (NULL != fp)
			{
				if( fgets(buf, BUF_SIZE-1, fp)== NULL ) {
					fclose(fp);
					continue;
				}
				sscanf(buf, "%*s %s", cur_task_name);
				cur_task_name[PATH_SIZE - 1] = '\0';

				if (!strncmp(task_name, cur_task_name, PATH_SIZE)) {
					fclose(fp);
					break;
				}
				fclose(fp);
			}
		}
		closedir(dir);
	}

	return pid;
} 

void getNameByPid(pid_t pid, char *task_name) 
{
	char proc_pid_path[BUF_SIZE];
	char buf[BUF_SIZE]; 

	sprintf(proc_pid_path, "/proc/%d/status", pid);
	FILE* fp = fopen(proc_pid_path, "r");
	if(NULL != fp){
		if( fgets(buf, BUF_SIZE-1, fp) )
			sscanf(buf, "%*s %s", task_name);
		fclose(fp);
	}
} 

