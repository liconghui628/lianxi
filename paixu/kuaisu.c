#include <stdio.h>

int partition(int *array, int low, int high)
{
	int key;
	static int cnt = 0;

	key = array[low];
	while(low < high){
		while(low < high && array[high] >= key)
			high--, cnt++;
		array[low] = array[high];
		while(low < high && array[low] <= key)
			low++, cnt++;
		array[high] = array[low];
	}
	array[low] = key;
	printf("%s() cnt = %d\n",__func__, cnt);
	return low;
}

void quick(int *array, int low, int high)
{
	int index = 0;
	if(low >= high)
		return ;

	index = partition(array,low,high);
	quick(array,low,index);
	quick(array,index+1,high);
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
	quick(array, 0, sizeof(array)/sizeof(array[0])-1);
	printArray(array, sizeof(array)/sizeof(array[0]));

	int array1[8] = {0,5,1,8,4,3,9,1};
	quick(array1, 0,sizeof(array1)/sizeof(array1[0])-1);
	printArray(array1, sizeof(array1)/sizeof(array1[0]));
}
