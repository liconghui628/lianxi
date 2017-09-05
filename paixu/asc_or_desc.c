#include <stdio.h>

int asc_or_desc(int *arr, int itemNum)
{
    int ordor = 0, i = 0, diff = 0, same = 0, diffValue = 0;
    if (arr == NULL || itemNum < 1){
        printf("param error\n");
        return -1;
    }
    for (i = 0; i < itemNum-1; i++) {
        diffValue = arr[i] - arr[i+1];
        if (diffValue == 0)
            same ++;
        else if (diffValue < 0)
            diff ++;
        else 
            diff --;
    }
    if (diff + same == itemNum - 1)
        ordor = 0;
    else if (diff - same == 1 - itemNum)
        ordor = 1;
    else 
        ordor = 2;
    return ordor;
}


int main ()
{
    int ordor = 0, i = 0;
    int arr[] = {1,2,3,4,5,6} ;
    //int arr[] = {6,5,4,3,2,1} ;
    //int arr[] = {6,5,4,2,1,3} ;
    ordor = asc_or_desc(arr, sizeof(arr)/sizeof(arr[0]));
    for (i = 0; i < sizeof(arr)/sizeof(arr[0]); i++)
        printf("[%d] ", arr[i]);
    printf("\n");
    printf("ordor = %d\n",ordor);
    return 0;
}
