#include <stdio.h>
#include <sys/statfs.h>
#include <sys/vfs.h>
#include <errno.h>
 
void diskinfo(char *disk)
{
	long long total_size, free_size;
    struct statfs disk_info;
    char *path = "/";
    int ret = 0;

    if (ret == statfs(path, &disk_info) == -1)
    {
		return;
    }

	//printf("%ld %ld \n",disk_info.f_blocks, disk_info.f_bfree);
	//printf("%d%%\n",(int)(100 * (disk_info.f_blocks - disk_info.f_bfree) / disk_info.f_blocks));
	snprintf(disk, 7, "%3.1lf%%",(100.0 * (disk_info.f_blocks - disk_info.f_bfree) / disk_info.f_blocks));
}
