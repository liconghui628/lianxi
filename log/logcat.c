#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include "bl_log.h"
#include "bl_sem.h"
#include "bl_shm.h"

#define TAG	"TEST"

extern char *shmaddr;
extern int semid,shmid;

typedef struct{
	char 	s[TAG_SZE];
	int		m;
	int		l;
	int		L;
	int 	v;
	bool 	c;
	bool	e;
	bool 	b;
}LogCat;

static char levelarray[][8] = {
	{"fatal"}, {"alert"}, {"crit"}, {"error"}, {"warn"}, 
	{"notice"}, {"info"}, {"debug"}, {"trace"}, {"notset"}, {"unknown"}
};

//exit handler
static void exit_handler()
{
	static bool exeFlg = false;
	if(shmaddr && semid && !exeFlg){
		printf("readcnt : %d\n",((LogInfo*)shmaddr)->readcnt);
		bl_sem_pv(semid, 2, -1);
		((LogInfo*)shmaddr)->readcnt--;
		bl_sem_pv(semid, 2, 1);
		printf("readcnt : %d\n",((LogInfo*)shmaddr)->readcnt);
	}
	exeFlg = true;
}

//signal handler
static void signal_handler(int sig)
{
	exit_handler();
	exit(0);
}

static void print_help()
{
	printf("********LOGCAT HELP INTERFACE********\n");
	printf("logcat is a supply debugging function\n"
		   "application, its need a share memory \n"
		   "to work.                             \n");
	printf("params:                              \n");
	printf(" -s filter condition,supply 8 mostly.\n");
	printf(" -l output log level least, include  \n"
		   "    'trace' 'debug' 'info' 'notice'  \n"
		   "    'warn' 'error' 'crit' 'alert'    \n"
		   "    'fatal'.                         \n");
	printf(" -L filter condition,same as -l,but  \n"
		   "    only show one level.             \n");
	printf(" -m log level limit, limit some lower\n"
		   "    level to output to share memory  \n"
		   "    param same with -l, default level\n"
		   "    is warn.                         \n");
	printf(" -c clear previous log.              \n");
	printf(" -h show this help.                  \n");
	printf("*************************************\n");

	return;
}

static int get_level_num(char levelarray[][8], char *levelstr)
{
	int i;
	for(i = 0; i < 11; i++){
		if(!strncmp(levelarray[i], levelstr, 8))
			return i;
	}
	return -1;
}

static int deserialize_args(int argc, char *argv[], LogCat *logcat)
{
	int opt, i;
	if(!logcat)
		return -1;

	logcat->m = -1;
	logcat->l = -1;
	logcat->L = -1;
	logcat->c = false;
	logcat->e = false;
	logcat->b = false;

	while((opt = getopt(argc, argv, "s:l:L:m:v:cebh")) != -1){
		switch(opt)
		{
		case 's':
			if(optarg)
				strncpy(logcat->s, optarg, TAG_SZE);
			break;
		case 'l':
			if(optarg) {
				logcat->l = get_level_num(levelarray, optarg);	
				if(logcat->l < 0)
					return -1;
			}
			break;
		case 'L':
			if(optarg) {
				logcat->L = get_level_num(levelarray, optarg);	
				if(logcat->L < 0)
					return -1;
			}
			break;
		case 'm':
			if(optarg) {
				logcat->m = get_level_num(levelarray, optarg);	
				if(logcat->m < 0)
					return -1;
			}
			break;
		case 'v':
			logcat->v = atoi(optarg);
			break;
		case 'c':
			logcat->c = true;
			break;
		case 'e':
			logcat->e = true;
			break;
		case 'b':
			logcat->b = true;
			break;
		case 'h':
			print_help();
			return -1;
			break;
		default:
			printf("unsupport param(%c), add -h to help.\n", opt);
			return -1;
			break;
		}
	}
	return 0;
}



int main(int argc, char *argv[])
{
	int i = 0, ret, loglines;
	bool firsttime = true;
	char *logptr, *logend, *logcurrent, *logfirst;
	struct sigaction act;
	LogInfo *loginfo;
	LogData *logdata;

	bl_log_init();

	atexit(exit_handler);
	act.sa_handler = signal_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGALRM, &act, 0);
	sigaction(SIGKILL, &act, 0);
	sigaction(SIGINT, &act, 0);

	LogCat *logcat = calloc(1, sizeof(LogCat));
	
	loginfo = (LogInfo*)shmaddr;
	logfirst = shmaddr + loginfo->firstline * LINE_SZE;
	logcurrent = shmaddr + loginfo->currentline * LINE_SZE;
	logend = shmaddr + loginfo->totalline * LINE_SZE;

	bl_sem_pv(semid, 2, -1);
	loginfo->readcnt++;
	bl_sem_pv(semid, 2, 1);

	ret = deserialize_args(argc, argv, logcat);
	if(ret < 0)
		exit(0);

	if(logcat->m >= 0){	// -m
		bl_sem_pv(semid, 2, -1);
		loginfo->loglimit = logcat->m;
		bl_sem_pv(semid, 2, 1);
	}

	if(logcat->c)    // -c
		logptr = logcurrent;
	else{
		logptr = logcurrent + LOG_SZE;
		if(logptr >= logend)
			logptr = shmaddr + LOG_SZE;
	}

	printf("%p,%d,%d\n",shmaddr,semid,shmid);
	while(1){
		if(logcat->c)
			bl_sem_pv(semid, 0, -1);
		else{
			if(!firsttime)
				bl_sem_pv(semid, 0, -1);
		}

		if(logptr < shmaddr || logptr > logend)
			exit(0);
		logdata = (LogData*)logptr;

		while(logdata->magicnum != MAGIC_NUM){
			logptr += LINE_SZE;
			if(logptr >= logend)
				logptr = shmaddr + LINE_SZE;
			logdata = (LogData*)logptr;
		}
		loglines = logdata->loglines;

		if(!strncmp(logdata->logtag, logcat->s, TAG_SZE)){	//TAG
			if(logcat->L >= 0){	// -L
				//printf("m:%d, L:%d, l:%d if222222222222222\n",logcat->m,logcat->L,logcat->l);
				if(logdata->loglevel == logcat->L){
					printf("[%s %s/%s]:%s\n",
							logdata->logtime,logdata->logtag,levelarray[logdata->loglevel],logdata->logmsg);
				}
			}
			else if(logcat->l >= 0){	// -l
				//printf("m:%d, L:%d, l:%d else if333333333333333333\n",logcat->m,logcat->L,logcat->l);
				if(logdata->loglevel <= logcat->l){
					printf("[%s %s/%s]:%s\n",
							logdata->logtime,logdata->logtag,levelarray[logdata->loglevel],logdata->logmsg);
				}
			}
			else{
				//printf(" m:%d, L:%d, l:%d else444444444444444\n",logcat->m,logcat->L,logcat->l);
					printf("[%s %s/%s]:%s\n",
							logdata->logtime,logdata->logtag,levelarray[logdata->loglevel],logdata->logmsg);
			}
		}

		if((logptr + (loglines * LINE_SZE)) >= logend)
			logptr = shmaddr + LINE_SZE;
		else
			logptr += loglines * LINE_SZE;
		
		if(logptr == logcurrent)
			firsttime = false;
	}
	return 0;
}
