#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <errno.h>

#include "bl_shm.h"


int bl_shm_init(key_t key, int sze)
{
	int shmid;

	if(sze <= 0)
		sze = SHM_SZE;
    shmid = shmget(key, sze, 0666 | IPC_CREAT | IPC_EXCL);
    if((shmid < 0) && (errno == EEXIST))
        shmid = shmget(key, sze, 0666);

	return shmid;
}

void *bl_shm_ataddr(int shmid)
{
	if(shmid < 0)
		return NULL;

    return shmat(shmid, NULL, 0);
}

void bl_shm_exit(int shmid, void *shmaddr)
{
	if(shmaddr)
    	shmdt(shmaddr);
    shmctl(shmid, IPC_RMID, NULL);
}
