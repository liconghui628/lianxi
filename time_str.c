#include <stdio.h>
#include <time.h>
#include <string.h>

char* time_str(const char *type)
{
	time_t curTime;
	struct tm *local_time;
	int index = 0, i = 0, j = 0; 
	int year, month, day, hour, min, sec;
	char c;
	char year_str[9] = {0};
	static char strTime[30] = {0};

	if(!type || strlen(type) >= 20)
		return NULL;

	curTime = time(NULL);
	local_time = localtime(&curTime);
	year = local_time->tm_year + 1900;
	month = local_time->tm_mon + 1;
	day = local_time->tm_mday;
	hour = local_time->tm_hour;
	min = local_time->tm_min;
	sec = local_time->tm_sec;

	strncpy(strTime, type, sizeof(strTime));
	while(strTime[j] != '\0'){
		if(strTime[j] != 'Y' && strTime[j] != 'y' && strTime[j] != 'm' && strTime[j] != 'M' && strTime[j] != 'd' && strTime[j] != 'D' 
				&& strTime[j] != 'h' && strTime[j] != 'H' && strTime[j] != 'm' && strTime[j] != 'M' && strTime[j] != 's' && strTime[j] != 'S'){
			j++;
			continue;
		}
		//year
		i = 0;
		while(strTime[j] == 'y' || strTime[j] == 'Y') i++, j++;
		if(i > 0){
			snprintf(year_str, sizeof(year_str), "%08d", year);
			c = strTime[j]; // '-' or ':'
			strncpy(&strTime[j - i], (const char*)&year_str[strlen(year_str) - i], i+1);
			strTime[j] = c;
		}

		//month
		i = 0;
		while(strTime[j] == 'M') i++, j++; 
		if(i > 0){
			c = strTime[j]; // '-' or ':'
			snprintf(&strTime[j - i], i + 1, "%02d",month);
			strTime[j] = c;
		}

		//day
		i = 0;
		while(strTime[j] == 'd' || strTime[j] == 'D') i++, j++; 
		if(i > 0){
			c = strTime[j]; // '-' or ':'
			snprintf(&strTime[j - i], i + 1, "%02d",day);
			strTime[j] = c;
		}

		//hour
		i = 0;
		while(strTime[j] == 'h' || strTime[j] == 'H') i++, j++; 
		if(i > 0){
			c = strTime[j]; // '-' or ':'
			snprintf(&strTime[j - i], i + 1, "%02d",hour);
			strTime[j] = c;
		}
	
		//min
		i = 0;
		while(strTime[j] == 'm') i++, j++; 
		if(i > 0){
			c = strTime[j]; // '-' or ':'
			snprintf(&strTime[j - i], i + 1, "%02d",min);
			strTime[j] = c;
		}
	
		//sec
		i = 0;
		while(strTime[j] == 's' || strTime[j] == 'S') i++, j++; 
		if(i > 0){
			c = strTime[j]; // '-' or ':'
			snprintf(&strTime[j - i], i + 1, "%02d",sec);
			strTime[j] = c;
		}
	}
	return strTime;
}

int main()
{
	printf("%s\n", time_str("YY-MM-DD"));
	printf("%s\n", time_str("YYy-MM-dd"));
	printf("%s\n", time_str("YYYY-MM-dD"));
	printf("%s\n", time_str("YYYY-MM-dd hh:mm:ss"));
	printf("%s\n", time_str("YYYY/MM/DD hh:mm:ss"));
	printf("%s\n", time_str("dd/MM/yyyY hh mm ss"));
	return 0;
}
