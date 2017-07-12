#include <stdio.h>

void maopao(int array[], int num)
{
	int i = 0, j = 0, cnt = 0;
	int tmp = 0;
	for(i = 0; i < num; i++){
		for(j = 0; j < num-i; j++){
			if(array[j] > array[j+1]){
				tmp = array[j];
				array[j] = array[j+1];
				array[j+1] = tmp;
			}
			cnt++;
		}
	}
	printf("%s() cnt = %d\n",__func__, cnt);
}

void printArray(int array[], int num)
{
	//printf("%s():num = %d\n",__func__, num);
	int i = 0;
	for(i = 0; i < num; i++){
		printf("%d ",array[i]);
	}
	printf("\n");
}

int main()
{
	int array[5] = {5,8,4,3,9};
	maopao(array, sizeof(array)/sizeof(array[0]));
	printArray(array, sizeof(array)/sizeof(array[0]));

	int array1[8] = {0,5,1,8,4,3,9,1};
	maopao(array1, sizeof(array1)/sizeof(array1[0]));
	printArray(array1, sizeof(array1)/sizeof(array1[0]));
}
