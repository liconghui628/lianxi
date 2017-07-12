#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <time.h>

#define NUM_OF_ARRAY 1000000
#define RAND_OF_SEED time(NULL)
#define RANGE_OF_MIN 0
#define RANGE_OF_MAX 10000

typedef int(*COMFUNC)(const void *, const void *);

void qsort(void *base, size_t nmemb, size_t size, int(*compar)(const void *, const void *));
void sort_quick(void *base, size_t low, size_t high, size_t size, COMFUNC compar, void *tmp);
size_t quick_part(void *base, size_t low, size_t high, size_t size, COMFUNC compar, void *tmp);

int compare_int(const void *data1, const void *data2)
{
    return *(int *)data1 - *(int *)data2;
}

/*
int main(int argc, char *argv[])
{
    int i, cnt;
    int *arr = (int *)malloc(NUM_OF_ARRAY * sizeof(int));
    clock_t start_t, end_t;

    srand(RAND_OF_SEED);
    for(i = 0; i < NUM_OF_ARRAY; i++) {
        arr[i] = rand() % (RANGE_OF_MAX - RANGE_OF_MIN) + RANGE_OF_MIN;
    }
    start_t = clock();
    qsort(arr, NUM_OF_ARRAY, sizeof(int), compare_int);
    end_t = clock();
    cnt = NUM_OF_ARRAY > 100 ? 100 : NUM_OF_ARRAY;
    for(i = 0; i < cnt; i++) {
        printf("%-10d", arr[i]);
        if((i + 1) % 10 == 0) {
            printf("\n");
        }
    }
    printf("The time of sort used is:%fs\n", (end_t - start_t) * 1.0 / CLOCKS_PER_SEC);

    return 0;
}
*/

///////////////////////////////////////////////////////////////////////////////////////////
void qsort(void *base, size_t nmemb, size_t size, int(*compar)(const void *, const void *))
{
    if(!base) {
        return;
    }

    void *tmp = malloc(size);
    sort_quick(base, 0, nmemb - 1, size, compar, tmp);
	if(tmp) {
		free(tmp);
		tmp = NULL;
	}
}

void sort_quick(void *base, size_t low, size_t high, size_t size, COMFUNC compar, void *tmp)
{
    size_t mdl;

	if(low < high && high != ~(0U)) {
		mdl = quick_part(base, low, high, size, compar, tmp);
		sort_quick(base, low, mdl - 1, size, compar, tmp);
		sort_quick(base, mdl + 1, high, size, compar, tmp);
	}
}

size_t quick_part(void *base, size_t low, size_t high, size_t size, COMFUNC compar, void *tmp)
{
	int i;
	size_t low_loc, high_loc;

	low_loc = low * size;
	high_loc = high * size;
	for(i = 0; i < size; i++) {
		*((char *)tmp + i) = *((char *)base + low_loc + i);
	}
	while(low < high && high != ~(0U)) {
		while(low < high && high != ~(0U) && compar((char *)base + high_loc, tmp) >= 0) {
            high--;
            high_loc -= size;
        }
        for(i = 0; i < size; i++) {
           *((char *)base + low_loc + i) = *((char *)base + high_loc + i);
        }
        while(low < high && compar(tmp, (char *)base + low_loc) >= 0) {
            low++;
            low_loc += size;
        }
        for(i = 0; i < size; i++) {
           *((char *)base + high_loc + i) = *((char *)base + low_loc + i);
        }
    }
    for(i = 0; i < size; i++) {
        *((char *)base + low_loc + i) = *((char *)tmp + i);
    }

    return low;
}
