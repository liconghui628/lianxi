#ifndef _BL_SEM_H_
#define _BL_SEM_H_

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

union semun
{
    int val;
    struct semid_ds *buf;
    unsigned short *array;
    struct seminfo *__buf;
};

int bl_sem_init(key_t key, int nsems, int inval[], int *iscreat);

void bl_sem_exit(int semid);

int bl_sem_pv(int semid, int num, int op);

#ifdef __cplusplus
} 
#endif	// #ifdef __cplusplus
#endif 	// _BL_SEM_H_
