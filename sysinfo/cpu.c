#include <stdio.h>
#include <stdlib.h>

void cpuinfo(char *cpu)
{
	FILE *fd = NULL;
	//static unsigned long user1, nice1, system1, idle1; 
	unsigned long user1, nice1, system1, idle1; 
	unsigned long user2, nice2, system2, idle2;
	unsigned long used1, used2, total1, total2;
	char buff[100] = {0};
	char *ret = NULL;

	fd = fopen("/proc/stat","r");
	if(fd == NULL){
		//printf("fopen() error\n");
		return ;
	}
	ret = fgets(buff, sizeof(buff), fd);
	if(ret == NULL){
		//printf("fgets() error\n");
		return ;
	}
	fclose(fd);
	sscanf(buff,"%*s%lu%lu%lu%lu",&user1,&nice1,&system1,&idle1);
	//printf("%lu  %lu  %lu  %lu\n",user1,nice1,system1,idle1);

	sleep(1);

	fd = fopen("/proc/stat","r");
	if(fd == NULL){
		//printf("fopen() error\n");
		return ;
	}
	ret = fgets(buff, sizeof(buff), fd);
	if(ret == NULL){
		//printf("fgets() error\n");
		return ;
	}
	sscanf(buff,"%*s%lu%lu%lu%lu",&user2,&nice2,&system2,&idle2);
	//printf("%lu  %lu  %lu  %lu\n",user2,nice2,system2,idle2);
	used1 = user1 + nice1 + system1;
	used2 = user2 + nice2 + system2;
	total1 = used1 + idle1;
	total2 = used2 + idle2;
	if(total2 - total1 != 0){
		//printf("%u%%\n",(unsigned int)((used2 - used1) * 100 / (total2 - total1)));
		snprintf(cpu, 7, "%3.1lf%%",((double)(used2 - used1) * 100 / (total2 - total1)));
	}
	//user1 = user2; nice1 = nice2; system1 = system2; idle1 = idle2;
	fclose(fd);
}
