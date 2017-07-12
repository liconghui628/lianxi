#ifndef _BL_SHM_H_
#define _BL_SHM_H_

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SHM_SZE 	(0x20000) //128k
#define LINE_SZE 	(0x20)

int bl_shm_init(key_t key, int sze);

void *bl_shm_ataddr(int shmid);

void bl_shm_exit(int shmid, void *shmaddr);

#ifdef __cplusplus
} 
#endif	// #ifdef __cplusplus
#endif // _BL_SHM_H_

