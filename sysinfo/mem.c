#include <stdio.h>
#include <unistd.h>
void meminfo(char *mem)
{
	unsigned long cpu_num, page_size, page_num, free_page_num;
	unsigned long long total_size, free_size, used_size;
	
	cpu_num = sysconf (_SC_NPROCESSORS_CONF);
	//printf("cpu_num = %lu\n",cpu_num);

	page_size = sysconf(_SC_PAGESIZE);
	//printf("page_size = %lu\n",page_size);

	page_num = sysconf (_SC_PHYS_PAGES);
	//printf("page_num = %lu\n",page_num);

	free_page_num = sysconf (_SC_AVPHYS_PAGES);
	//printf("free_page_num = %lu\n",free_page_num);

	total_size = page_size * page_num;
	//printf("total_size = %llu\n",total_size);

	free_size = page_size * free_page_num;
	//printf("free_size = %llu\n",free_size);

	used_size = total_size - free_size;
	//printf("used_size = %llu\n",used_size);

	//printf("内存使用率为：%d%%\n",(int)(100.0 * used_size / total_size));
	
	snprintf(mem, 7, "%3.1lf%%",(100.0 * used_size / total_size));
	
}
