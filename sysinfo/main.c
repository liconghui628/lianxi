#include <stdio.h>

int main()
{
	char cpu[7] = {'8','%'};
	char mem[7] = {'2','5','%'};
	char disk[7] = {'5','0','%'};
	char mac_str[18] = {0};
	char ip_str[16] = {0};

	printf("Content-type: text/html\n\n");  
	cpuinfo(cpu);
	printf("%s",cpu);
	printf(";");
	
	meminfo(mem);
	printf("%s",mem);
	printf(";");

	diskinfo(disk);
	printf("%s",disk);
	printf(";");

	netinfo(mac_str, ip_str, sizeof(mac_str),sizeof(ip_str));
	printf("%s",mac_str);
	printf(";");
	printf("%s",ip_str);
	printf(";");

	upfile_result();
	printf(";");
}
