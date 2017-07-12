#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <sys/ipc.h>
#include <stdarg.h>
#include <stdbool.h>
#include <errno.h>
#include "bl_log.h"
#include "bl_sem.h"
#include "bl_shm.h"

char *shmaddr;
int semid, shmid;
int iscreat = 1;
static char rowbuff[LOG_SZE];
static char writebuf[LOG_SZE];
static int semcnt;

static char levelarray[][8] = {
	{"fatal"}, {"alert"}, {"crit"}, {"error"}, {"warn"}, 
	{"notice"}, {"info"}, {"debug"}, {"trace"}, {"notset"}, {"unknown"}
};

int bl_log_init(void)
{
	LogInfo *loghead;
	key_t key = 0;
	int inval[] = {0, 1, 1};

    key = ftok("/", 82);
	semid = bl_sem_init(key, sizeof(inval)/sizeof(inval[0]), inval, &iscreat);
	shmid = bl_shm_init(key, SHM_SZE);

	//printf("%s():semid = %d,shmid = %d\n",__func__,semid,shmid);
	shmaddr = bl_shm_ataddr(shmid);
	if(semid < 0 || shmid < 0 || !shmaddr)
		return -1;
	//printf("%s():shmaddr:%p\n",__func__,shmaddr);
	if(iscreat){
		memset(shmaddr, 0, sizeof(LogInfo));
		loghead = (LogInfo*)shmaddr;
		loghead->readcnt = 0;
		loghead->loglimit = LOG_DEFAULT_LEVEL;	// warn
		loghead->firstline = 1;
		loghead->currentline = 1;
		loghead->totalline = SHM_SZE/LINE_SZE ;
		loghead->isfull = false;
	}
	return 0;
}

void bl_log_fini(void)
{
	if(shmid >= 0 && shmaddr)
		bl_shm_exit(shmid, shmaddr);
	if(semid >= 0)
		bl_sem_exit(semid);
}

int log_record(const char *tag, int level, const char *format, va_list ap)
{
	int wrcnt = -1, vsnprintcnt = -1; 
	time_t logtime;
	struct tm *local;
	LogInfo *loghead;
	LogData *logdata, *logfirst, *logcurrent;
	if(bl_sem_pv(semid, 1, -1) == -1){	//获取写信号量
		printf("bl_sem_pv faild\n");
		return -1;
	}
	loghead = (LogInfo*)shmaddr;
	if(loghead->readcnt == 0)
		loghead->loglimit = LOG_DEFAULT_LEVEL;
	if(loghead->loglimit < ((level/100) & 0x0f)){
		bl_sem_pv(semid, 1, 1);
		return 0;
	}
	logdata = (LogData*)rowbuff;
	logfirst = (LogData*)(shmaddr + loghead->firstline * LINE_SZE) ;
	logcurrent = (LogData*)(shmaddr + loghead->currentline * LINE_SZE);
	logdata->magicnum = MAGIC_NUM;
	strncpy((char *)logdata->logtag, tag, TAG_SZE);
	logdata->loglevel = (level / 100) & 0x0F;
	logtime = time(NULL);
	local = localtime(&logtime);
	snprintf(logdata->logtime, sizeof(logdata->logtime),"%d-%02d-%02d %02d:%02d:%02d",
			local->tm_year+1900,local->tm_mon+1,local->tm_mday,local->tm_hour,local->tm_min,local->tm_sec);
	vsnprintcnt = vsnprintf((char *)logdata->logmsg,sizeof(rowbuff) - sizeof(LogData) - 1 , format, ap);
	if(vsnprintcnt >= sizeof(rowbuff) - sizeof(LogData) - 1 ){
		((char *)(logdata->logmsg))[sizeof(rowbuff) - sizeof(LogData) - 3] = '\r';
		((char *)(logdata->logmsg))[sizeof(rowbuff) - sizeof(LogData) - 2] = '\n';
		((char *)(logdata->logmsg))[sizeof(rowbuff) - sizeof(LogData) - 1] = '\0';
	}
	else if(vsnprintcnt > 0 && vsnprintcnt < sizeof(rowbuff) - sizeof(LogData) - 1){
		((char *)(logdata->logmsg))[vsnprintcnt - 1] = '\r';
		((char *)(logdata->logmsg))[vsnprintcnt] = '\n';
		((char *)(logdata->logmsg))[vsnprintcnt + 1] = '\0';
	}

	wrcnt = strlen((char*)logdata->logmsg) + 1 + sizeof(LogData);
	if(wrcnt % LINE_SZE)
		logdata->loglines = wrcnt/LINE_SZE + 1;
	else
		logdata->loglines = wrcnt/LINE_SZE;

	if(loghead->currentline + logdata->loglines <= loghead->totalline){
		memcpy(logcurrent, rowbuff, wrcnt);
		loghead->currentline += logdata->loglines;
		if(loghead->readcnt > 0){	
			loghead->firstline = loghead->currentline;
		}
	}
	else {
		loghead->isfull = true;
		memset(logcurrent, 0, (loghead->totalline - loghead->currentline) * LINE_SZE);
		loghead->currentline = 1;
		logcurrent = (LogData*)(shmaddr + loghead->currentline * LINE_SZE);
		memcpy(logcurrent, rowbuff, logdata->loglines * LINE_SZE);
		loghead->currentline += logdata->loglines;
		if(loghead->readcnt > 0)
			loghead->firstline = loghead->currentline;
	}
	bl_sem_pv(semid, 1, 1);	//释放写信号量
	if(loghead->readcnt > 0)
		bl_sem_pv(semid, 0, loghead->readcnt);
	printf("[%s %s/%d]:%s\n",logdata->logtime,logdata->logtag,logdata->loglevel,logdata->logmsg);
	return 0;
}

int log_write(const char *tag, int level, const char *format, va_list ap)
{
	char timestr[20];
	int wcnt, cnt1, cnt2;
	FILE *file;
	time_t logtime;
	struct tm *local;
	struct stat st;

	logtime = time(NULL);
	local = localtime(&logtime);
	snprintf(timestr, sizeof(timestr),"%d-%02d-%02d %02d:%02d:%02d",
			local->tm_year+1900,local->tm_mon+1,local->tm_mday,local->tm_hour,local->tm_min,local->tm_sec);

	cnt1 = snprintf(writebuf,sizeof(writebuf),"[%s %s/%s]:",timestr,tag,levelarray[(level/100)&0x0F]);
	if(cnt1 < 0)
		return -1;

	cnt2 = vsnprintf(&writebuf[cnt1],sizeof(writebuf)- cnt1,format,ap);
	if(cnt2 < 0)
		return -1;

	if(cnt2 >= sizeof(writebuf)- cnt1){
		writebuf[sizeof(writebuf) - 2] = '\n';
		writebuf[sizeof(writebuf) - 1] = '\0';
	}
	else{
		writebuf[cnt1 + cnt2] = '\n';
		writebuf[cnt1 + cnt2 + 1] = '\0';
	}
	
	//bl_sem_pv(semid, 2, -1);
	file = fopen(LOG_FILE, "ab+");
	if(!file){
	//	bl_sem_pv(semid, 2, 1);
		return -1;
	}
	stat(LOG_FILE,&st);
	if(st.st_size < FILE_MAX_SZE)
		fwrite(writebuf, strlen(writebuf) + 1, 1, file);
	fclose(file);
	//bl_sem_pv(semid, 2, 1);
	return 0;
}

void bl_log(const char *tag, int wpriority, const char *format, ...)
{
	va_list va;
	char *pcat_name = NULL;
	int priority = wpriority & 0xFFFF;
	bool isPrint = !!(wpriority & LOG_PRIORITY_PRINT);
	bool isWrite = !!(wpriority & LOG_PRIORITY_WRITE);
	
	if(tag && tag[0]) {
		va_start(va, format);
		//printf("start log_record()\n");
		log_record(tag, priority, format, va);
		va_end(va);
	}

	if(isWrite) {
		va_start(va, format);
		//printf("start log_write()\n");
		log_write(tag, priority, format, va);
		va_end(va);
	}

	if(isPrint) {
		va_start(va, format);
		vprintf(format, va);
		va_end(va);
		puts("\n");
	}
}













