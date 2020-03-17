#include <stdio.h>

void xier(int array[], int size)
{
	int i, j, gap;

	if(!array || size <= 0) return ;
	
	for(gap = n/2; gap > 0; gap/=2){
		for(i = 0; i < gap; i++){
			int temp = array[i];
			j = 0;
			while(j < gap && array[j] > temp){
				array[j]
				j++;
			}
		}
	}
	
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
	xier(array, sizeof(array)/sizeof(array[0]));
	printArray(array, sizeof(array)/sizeof(array[0]));

	int array1[8] = {0,5,1,8,4,3,9,1};
	xier(array1, sizeof(array1)/sizeof(array1[0]));
	printArray(array1, sizeof(array1)/sizeof(array1[0]));
}
