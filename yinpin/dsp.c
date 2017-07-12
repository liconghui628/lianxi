#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <linux/soundcard.h>

#define LENGTH 3
#define RATE	8000
#define SIZE 	8
#define CHANNELS 1

unsigned char buf[LENGTH*RATE*SIZE*CHANNELS/8];

int main()
{
	int fd, arg, status;
	fd = open("/dev/dsp", O_RDWR);
	if(fd < 0){
		perror("open /dev/dsp error");
		exit(1);
	}
	//设置采样时的量化位数
	arg = SIZE;
	status = ioctl(fd, SOUND_PCM_WRITE_BITS, &arg);
	if(status == -1)
		perror("SOUND_PCM_WRITE_BITS ioctl error");
	if(arg != SIZE)
		perror("unable to set sample size");

	//设置采样时的声道数目
	arg = CHANNELS;
		perror("SOUND_PCM_WRITE_RATE ioctl error");
	status = ioctl(fd, SOUND_PCM_WRITE_CHANNELS, &arg);
	if(status == -1)
		perror("SOUND_PCM_WRITE_CHANNLS ioctl error");
	if(arg != CHANNELS)
		perror("unable to set number of channels");

	//设置采样频率
	arg = RATE;
	status = ioctl(fd, SOUND_PCM_WRITE_RATE, &arg);
	if(status == -1)
		perror("SOUND_PCM_WRITE_RATE ioctl error");

	while(1){
		printf("Say something:\n");
		status = read(fd, buf, sizeof(buf));
		printf("you said:\n");
		status = write(fd, buf, sizeof(buf));
		//在继续录音前等待回放结束
		status = ioctl(fd, SOUND_PCM_SYNC, 0);
		if(status == -1)
			perror("SOUND_PCM_SYNC ioctl error");
	}	
	return 1;
}
