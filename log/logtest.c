#include <stdio.h>
#include <sys/types.h>
#include "bl_log.h"
#include "bl_sem.h"
#include "bl_shm.h"

#define TAG	"TEST"

extern char *shmaddr;
extern int semid,shmid;

int main()
{
	int i = 0;
	bl_log_init();
	while(1){
		bl_log("AAA", LOGW_PRIORITY_TRACE, "This is a test log %d !!! aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", ++i);
		usleep(10);
		bl_log("BBB", LOG_PRIORITY_DEBUG, "This is a test log %d !!! bbbbbbbbbbbbb", ++i);
		usleep(10);
		bl_log("CCC", LOGW_PRIORITY_ERROR, "This is a test log %d !!! ccccccccccccccccccccccccccccccccccccccccccccc", ++i);
		usleep(10);
		bl_log("AAA", LOGW_PRIORITY_INFO, "This is a test log %d !!! dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd", ++i);
		//printf("This is a test log %d !!!\n", ++i);
		usleep(10);
		bl_log("BBB", LOGW_PRIORITY_TRACE, "This is a test log %d !!! aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", ++i);
		usleep(10);
		bl_log("CCC", LOG_PRIORITY_DEBUG, "This is a test log %d !!! bbbbbbbbbbbbb", ++i);
		usleep(10);
		bl_log("AAA", LOGW_PRIORITY_ERROR, "This is a test log %d !!! ccccccccccccccccccccccccccccccccccccccccccccc", ++i);
		usleep(10);
		bl_log("BBB", LOGW_PRIORITY_INFO, "This is a test log %d !!! dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd", ++i);
		usleep(10);
		bl_log("CCC", LOGW_PRIORITY_INFO, "This is a test log %d !!! dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd", ++i);
		usleep(10);
	}
	bl_log_fini();
	return 0;
}
