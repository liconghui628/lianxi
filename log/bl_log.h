#ifndef __BL_LOG_H__
#define __BL_LOG_H__

#include <time.h>
#include <stdarg.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TAG_SZE				(0x10)
#define MAGIC_NUM			(0xFFFFFFFF)
#define LOG_SZE				(10<<10)//10K one log buff support
#define LOG_DEFAULT_LEVEL	4		//warn
#define LOG_FILE			"./logfile.txt"
#define FILE_MAX_SZE		(1<<20) //1M


typedef enum {
    /** fatal */	LOG_PRIORITY_FATAL		= 000, 
    /** alert */	LOG_PRIORITY_ALERT		= 100, 
    /** crit */	    LOG_PRIORITY_CRIT		= 200, 
    /** error */	LOG_PRIORITY_ERROR		= 300, 
    /** warn */	    LOG_PRIORITY_WARN		= 400, 
    /** notice */	LOG_PRIORITY_NOTICE		= 500, 
    /** info */	    LOG_PRIORITY_INFO		= 600, 
    /** debug */	LOG_PRIORITY_DEBUG		= 700,
    /** trace */	LOG_PRIORITY_TRACE		= 800,
    /** notset */	LOG_PRIORITY_NOTSET		= 900,
    /** unknown */	LOG_PRIORITY_UNKNOWN	= 1000
} LogPriorityLevel;

#define LOG_PRIORITY_PRINT		0x00010000
#define LOG_PRIORITY_WRITE		0x00020000
#define LOG_PRIORITY_RUNWR		0x00040000

#define LOGP_PRIORITY_FATAL		(LOG_PRIORITY_PRINT | LOG_PRIORITY_FATAL)
#define LOGP_PRIORITY_ALERT		(LOG_PRIORITY_PRINT | LOG_PRIORITY_ALERT)
#define LOGP_PRIORITY_CRIT		(LOG_PRIORITY_PRINT | LOG_PRIORITY_CRIT)
#define LOGP_PRIORITY_ERROR		(LOG_PRIORITY_PRINT | LOG_PRIORITY_ERROR)
#define LOGP_PRIORITY_WARN		(LOG_PRIORITY_PRINT | LOG_PRIORITY_WARN)
#define LOGP_PRIORITY_NOTICE	(LOG_PRIORITY_PRINT | LOG_PRIORITY_NOTICE)
#define LOGP_PRIORITY_INFO		(LOG_PRIORITY_PRINT | LOG_PRIORITY_INFO)
#define LOGP_PRIORITY_DEBUG		(LOG_PRIORITY_PRINT | LOG_PRIORITY_DEBUG)
#define LOGP_PRIORITY_TRACE		(LOG_PRIORITY_PRINT | LOG_PRIORITY_TRACE)
#define LOGP_PRIORITY_NOTSET	(LOG_PRIORITY_PRINT | LOG_PRIORITY_NOTSET)
#define LOGP_PRIORITY_UNKNOWN	(LOG_PRIORITY_PRINT | LOG_PRIORITY_UNKNOWN)

#define LOGW_PRIORITY_FATAL		(LOG_PRIORITY_WRITE | LOG_PRIORITY_FATAL)
#define LOGW_PRIORITY_ALERT		(LOG_PRIORITY_WRITE | LOG_PRIORITY_ALERT)
#define LOGW_PRIORITY_CRIT		(LOG_PRIORITY_WRITE | LOG_PRIORITY_CRIT)
#define LOGW_PRIORITY_ERROR		(LOG_PRIORITY_WRITE | LOG_PRIORITY_ERROR)
#define LOGW_PRIORITY_WARN		(LOG_PRIORITY_WRITE | LOG_PRIORITY_WARN)
#define LOGW_PRIORITY_NOTICE	(LOG_PRIORITY_WRITE | LOG_PRIORITY_NOTICE)
#define LOGW_PRIORITY_INFO		(LOG_PRIORITY_WRITE | LOG_PRIORITY_INFO)
#define LOGW_PRIORITY_DEBUG		(LOG_PRIORITY_WRITE | LOG_PRIORITY_DEBUG)
#define LOGW_PRIORITY_TRACE		(LOG_PRIORITY_WRITE | LOG_PRIORITY_TRACE)
#define LOGW_PRIORITY_NOTSET	(LOG_PRIORITY_WRITE | LOG_PRIORITY_NOTSET)
#define LOGW_PRIORITY_UNKNOWN	(LOG_PRIORITY_WRITE | LOG_PRIORITY_UNKNOWN)

#define LOGPW_PRIORITY_FATAL	(LOG_PRIORITY_PRINT | LOG_PRIORITY_WRITE | LOG_PRIORITY_FATAL)
#define LOGPW_PRIORITY_ALERT	(LOG_PRIORITY_PRINT | LOG_PRIORITY_WRITE | LOG_PRIORITY_ALERT)
#define LOGPW_PRIORITY_CRIT		(LOG_PRIORITY_PRINT | LOG_PRIORITY_WRITE | LOG_PRIORITY_CRIT)
#define LOGPW_PRIORITY_ERROR	(LOG_PRIORITY_PRINT | LOG_PRIORITY_WRITE | LOG_PRIORITY_ERROR)
#define LOGPW_PRIORITY_WARN		(LOG_PRIORITY_PRINT | LOG_PRIORITY_WRITE | LOG_PRIORITY_WARN)
#define LOGPW_PRIORITY_NOTICE	(LOG_PRIORITY_PRINT | LOG_PRIORITY_WRITE | LOG_PRIORITY_NOTICE)
#define LOGPW_PRIORITY_INFO		(LOG_PRIORITY_PRINT | LOG_PRIORITY_WRITE | LOG_PRIORITY_INFO)
#define LOGPW_PRIORITY_DEBUG	(LOG_PRIORITY_PRINT | LOG_PRIORITY_WRITE | LOG_PRIORITY_DEBUG)
#define LOGPW_PRIORITY_TRACE	(LOG_PRIORITY_PRINT | LOG_PRIORITY_WRITE | LOG_PRIORITY_TRACE)
#define LOGPW_PRIORITY_NOTSET	(LOG_PRIORITY_PRINT | LOG_PRIORITY_WRITE | LOG_PRIORITY_NOTSET)
#define LOGPW_PRIORITY_UNKNOWN	(LOG_PRIORITY_PRINT | LOG_PRIORITY_WRITE | LOG_PRIORITY_UNKNOWN)

#define LOGWP_PRIORITY_FATAL	(LOG_PRIORITY_PRINT | LOG_PRIORITY_WRITE | LOG_PRIORITY_FATAL)
#define LOGWP_PRIORITY_ALERT	(LOG_PRIORITY_PRINT | LOG_PRIORITY_WRITE | LOG_PRIORITY_ALERT)
#define LOGWP_PRIORITY_CRIT		(LOG_PRIORITY_PRINT | LOG_PRIORITY_WRITE | LOG_PRIORITY_CRIT)
#define LOGWP_PRIORITY_ERROR	(LOG_PRIORITY_PRINT | LOG_PRIORITY_WRITE | LOG_PRIORITY_ERROR)
#define LOGWP_PRIORITY_WARN		(LOG_PRIORITY_PRINT | LOG_PRIORITY_WRITE | LOG_PRIORITY_WARN)
#define LOGWP_PRIORITY_NOTICE	(LOG_PRIORITY_PRINT | LOG_PRIORITY_WRITE | LOG_PRIORITY_NOTICE)
#define LOGWP_PRIORITY_INFO		(LOG_PRIORITY_PRINT | LOG_PRIORITY_WRITE | LOG_PRIORITY_INFO)
#define LOGWP_PRIORITY_DEBUG	(LOG_PRIORITY_PRINT | LOG_PRIORITY_WRITE | LOG_PRIORITY_DEBUG)
#define LOGWP_PRIORITY_TRACE	(LOG_PRIORITY_PRINT | LOG_PRIORITY_WRITE | LOG_PRIORITY_TRACE)
#define LOGWP_PRIORITY_NOTSET	(LOG_PRIORITY_PRINT | LOG_PRIORITY_WRITE | LOG_PRIORITY_NOTSET)
#define LOGWP_PRIORITY_UNKNOWN	(LOG_PRIORITY_PRINT | LOG_PRIORITY_WRITE | LOG_PRIORITY_UNKNOWN)

typedef struct {
	int 				magicnum;	
	int					loglines;
	char 				logtime[20];
	LogPriorityLevel	loglevel;
	char 				logtag[TAG_SZE];
	char 				logmsg[0];
}LogData;

typedef struct {
	int 	firstline;
	int 	currentline;
	int		totalline;
	int 	loglimit;
	int 	readcnt;
	bool	isfull;
}LogInfo;

int bl_log_init(void);
void bl_log_fini(void);

void bl_log(const char *tag, int wpriority, const char *format, ...);
int log_record(const char *tag, int level, const char *format, va_list ap);
int log_write(const char *tag, int level, const char *format, va_list ap);

#ifdef __cplusplus
} 
#endif	// #ifdef __cplusplus
#endif	//
