#include <stdio.h>

void zheban(int array[], int size)
{
	int i, j, low, mid, high;
	int tmp;
	if(!array || size <= 0) return ;
	for(i = 1; i < size; i++){
		low = 0;
		high = i - 1;
		tmp = array[i];
		while(low <= high){
			mid = (low+high)/2;
			if(array[mid] > tmp)
				high = mid - 1;
			else
				low = mid + 1;
		}
		j = i;
		while(j>low){
			array[j] = array[j-1];
			j--;
		}
		array[low] = tmp;
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
	int array[7] = {5,8,10,7,4,3,9};
	zheban(array, sizeof(array)/sizeof(array[0]));
	printArray(array, sizeof(array)/sizeof(array[0]));

	int array1[10] = {0,5,1,8,4,3,9,1,0,18};
	zheban(array1, sizeof(array1)/sizeof(array1[0]));
	printArray(array1, sizeof(array1)/sizeof(array1[0]));
}
