#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include "bl_sem.h"

int bl_sem_init(key_t key, int nsems, int inval[], int* iscreat)
{
    int i;
    int semid;
    union semun buf;

    semid = semget(key, nsems, 0666 | IPC_CREAT | IPC_EXCL);
    if(semid == -1) {
		printf("error\n");
        if(errno != EEXIST)
			return -1;
		*iscreat = 0;
        semid = semget(key, nsems, 0666);
        return semid;
    }
    for(i = 0; i < nsems; i++) {
        buf.val = inval[i];
        semctl(semid, i, SETVAL, buf);
    }

	return semid;
}

void bl_sem_exit(int semid)
{
	semctl(semid, IPC_RMID, 0);

	return;
}

int bl_sem_pv(int semid, int num, int op)
{
    struct sembuf buf;

    buf.sem_num = num;
    buf.sem_op = op;
    buf.sem_flg = 0;

    return semop(semid, &buf, 1);
}
