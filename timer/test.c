#include <stdio.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include "timer.h"

void timer_func(void *arg)
{
    int *arg2 = (int *)arg;
    printf("[pid:%lu][%ld]timer_func%d\n",pthread_self(), time(NULL),*arg2);
    //sleep(2);
}

void *thread_function1(void *arg)
{
    int i = 0;
    int a[10] = {1,2,3,4,5,6,7,8,9,10};
    int id[10] = {0};
    for (i = 0; i < sizeof(a)/sizeof(a[0]); i++) {
        id[i] = timer_add(a[i], timer_func, &a[i], 1);
    }
    sleep(20);
    for (i = 0; i < sizeof(a)/sizeof(a[0]); i++) {
        timer_del(id[i]);
    }
    while(1);
    return 0;
}

int main() 
{
   // pthread_t thread1;
   // pthread_create(&thread1, NULL, thread_function1, NULL);
    int i = 0;
    int a[10] = {1,2,3,4,5,6,7,8,9,10};
    int id[10] = {0};
    for (i = 0; i < sizeof(a)/sizeof(a[0]); i++) {
        id[i] = timer_add(a[i], timer_func, &a[i], 1);
    }
   /* sleep(20);
    for (i = 0; i < sizeof(a)/sizeof(a[0]); i++) {
        timer_del(id[i]);
    }*/
    while(1);
    return 0;
}
