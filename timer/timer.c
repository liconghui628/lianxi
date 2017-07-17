#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>
#include <pthread.h>

#define MAX_TIMER_COUNT (50)
#define MAX_ARRAY_SIZE  (5)
#define MAX_THREAD_COUNT (5)

#define _DEBUG_ 0

#if _DEBUG_
#define DEBUG(...) printf(__VA_ARGS__)
#else
#define DEBUG(...) 
#endif

typedef void TimerFunc(void *arg);

typedef struct node{
	struct node *next;
	struct node *prev;
    time_t sec;
    time_t sec_repeat;
    TimerFunc *timer_cb;
	void *arg;
    int repeat;
    unsigned int timer_id;
}TimerNode;

typedef struct ThreadData{
    pthread_t threadID;
    pthread_mutex_t *mutex;
    pthread_cond_t *cond;
    TimerNode *timerArray[MAX_ARRAY_SIZE];
    int arrindex;
    int haveData;
}TimerThread;

static TimerNode *g_timer_head = NULL;
static TimerNode *g_timer_tail = NULL;
static pthread_mutex_t *g_mutex = NULL;
static TimerThread *ThreadArray = NULL;

static int timer_init(void);
static int timer_count(void);
static void sigalarm_handle(int sigint);
static int set_timer(int sec);
int timer_add(int sec, TimerFunc *timer_cb, void *arg, int repeat);
int timer_del(int timer_id);

//分发定时任务给线程
static int timer_dispatch(TimerNode *timerNode)
{
    DEBUG("F:%s,f:%s,L:%d\n",  __FILE__, __func__, __LINE__);
    int i = 0, j = 0, index = 0, size = 0;
    //查看排队队列中是否已经有这个定时任务，有则不再重复添加
    for (i = 0; i < MAX_THREAD_COUNT; i++) {
        for (j = 0; j < MAX_ARRAY_SIZE; j++) {
            if (ThreadArray[i].timerArray[j] != NULL) {
                if (ThreadArray[i].timerArray[j]->timer_id == timerNode->timer_id) {
                    printf("F:%s,f:%s,L:%d, timerNode[ID:%u] already add in ThreadArray[%d].timerArray[%d] \n", 
                            __FILE__, __func__, __LINE__, timerNode->timer_id, i, j);
                    return 0;
                }
            }
        }
    }
    
    //排队队列中没有这个定时任务，添加
    //找一个当前没有任务的线程
    for (i = 0; i < MAX_THREAD_COUNT; i++) {
        if (ThreadArray[i].haveData == 0) {
            for (j = 0; j < MAX_ARRAY_SIZE; j++) {
                if (ThreadArray[i].timerArray[j] == NULL) {
                    ThreadArray[i].timerArray[j] = timerNode;
                    ThreadArray[i].haveData++;
                    DEBUG("F:%s,f:%s,L:%d, wake up [PID:%lu]\n",  __FILE__, __func__, __LINE__, ThreadArray[i].threadID);
                    pthread_cond_signal(ThreadArray[i].cond);
                    return 0;
                }
            }
        }
    }

    //找一个负载最小的线程分发
    size = ThreadArray[0].haveData;
    for (i = 0; i < MAX_THREAD_COUNT; i++) {
        if (ThreadArray[i].haveData < size) {
            size = ThreadArray[i].haveData;
            index = i;
        }
    }
    DEBUG("F:%s,f:%s,L:%d, index = %d [PID:%lu]\n",  __FILE__, __func__, __LINE__, index,ThreadArray[index].threadID);
    //在正在处理的元素后面开始添加
    //pthread_mutex_lock(ThreadArray[index].mutex);
    for (j = ThreadArray[index].arrindex; j < MAX_ARRAY_SIZE; j++) {
        if (ThreadArray[index].timerArray[j] != NULL) {
            printf("F:%s,f:%s,L:%d, ThreadArray[%d].timerArray[%d].timer_id=%u\n", __FILE__, __func__, __LINE__,index, j, ThreadArray[index].timerArray[j]->timer_id);
        } else {
            printf("F:%s,f:%s,L:%d, add timerNode[ID:%u] to ThreadArray[%d].timerArray[%d]\n", 
                    __FILE__, __func__, __LINE__,timerNode->timer_id, index, j);
            ThreadArray[index].timerArray[j] = timerNode;
            ThreadArray[index].haveData++;
            //pthread_mutex_unlock(ThreadArray[index].mutex);
            DEBUG("F:%s,f:%s,L:%d, wake up [PID:%lu]\n",  __FILE__, __func__, __LINE__, ThreadArray[index].threadID);
            pthread_cond_signal(ThreadArray[index].cond);
            return 0;
        }
    }
    //pthread_mutex_unlock(ThreadArray[index].mutex);
    printf("F:%s,f:%s,L:%d, timerID:%u dispatch failed !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", __FILE__, __func__, __LINE__, timerNode->timer_id);
    return -1;
}

//设置定时器
static int set_timer(int sec)
{
    DEBUG("F:%s,f:%s,L:%d\n", __FILE__, __func__, __LINE__);
    struct itimerval itm;
    memset(&itm, 0, sizeof(struct itimerval));
    itm.it_interval.tv_sec = sec;
    itm.it_value.tv_sec = sec;
    DEBUG("F:%s,f:%s,L:%d\n", __FILE__, __func__, __LINE__);
    return setitimer(ITIMER_REAL, &itm, NULL);
}

// alrm信号处理函数
static void sigalarm_handle(int sigint)
{
    TimerNode *tmpNode = NULL, *delNode = NULL, *tmpNode2 = NULL, *nextNode = NULL;
    if (sigint != SIGALRM) return;

    DEBUG("--------------------------------------------------------------------\n");
    DEBUG("[%ld]F:%s,f:%s,L:%d,timer_count:%d\n", time(NULL), __FILE__, __func__, __LINE__, timer_count());

    for(tmpNode = g_timer_head->next; tmpNode != g_timer_tail; tmpNode = tmpNode->next){
        // 时间到则执行定时任务，没到时间则更新剩余时间sec
        tmpNode->sec--;
        //DEBUG("F:%s,f:%s,L:%d,[CHK]: timer_id:%d, sec:%ld repeat:%d\n", __FILE__, __func__, __LINE__,tmpNode->timer_id, tmpNode->sec, tmpNode->repeat);
        if (tmpNode->sec <= 0) {
            DEBUG("F:%s,f:%s,L:%d,[GET]: timer_id:%d, sec:%ld repeat:%d\n", __FILE__, __func__, __LINE__,tmpNode->timer_id, tmpNode->sec, tmpNode->repeat);
            //tmpNode->timer_cb(tmpNode->arg);
            //分发给线程
            if (timer_dispatch(tmpNode) < 0) {
                printf("F:%s,f:%s,L:%d,handle failed, timer_id:%d, sec:%ld repeat:%d\n", __FILE__, __func__, __LINE__,tmpNode->timer_id, tmpNode->sec, tmpNode->repeat);
            }
        }
    }

    // 整理任务链表，删除超时的单次任务，重排重复任务
    tmpNode = g_timer_head->next;
    while (tmpNode != g_timer_tail) {
        //DEBUG("F:%s,f:%s,L:%d,[RECHK]: timer_id:%d, sec:%ld repeat:%d\n", __FILE__, __func__, __LINE__,tmpNode->timer_id, tmpNode->sec, tmpNode->repeat);
        //任务链表是有序的，如果当前节点没到时间，则其后节点一定也没到时间
        if (tmpNode->sec > 0) {
            break;
        } else {
            tmpNode->prev->next = tmpNode->next;
            tmpNode->next->prev = tmpNode->prev;
            if(tmpNode->repeat == 0){
                //删除失效的单次定时任务
                DEBUG("F:%s,f:%s,L:%d,[DEL]: timer_id:%d, sec:%ld repeat:%d\n", __FILE__, __func__, __LINE__,tmpNode->timer_id, tmpNode->sec, tmpNode->repeat);
                delNode = tmpNode;
                tmpNode = tmpNode->next;
                free(delNode);
            } else {
                //重新排列重复定时任务
                tmpNode->sec = tmpNode->sec_repeat;
                nextNode = tmpNode->next;
                DEBUG("F:%s,f:%s,L:%d,[READD]: timer_id:%d, sec:%ld repeat:%d\n", __FILE__, __func__, __LINE__,tmpNode->timer_id, tmpNode->sec, tmpNode->repeat);
                tmpNode2 = tmpNode->next;
                while (tmpNode2 != g_timer_tail) {
                    if (tmpNode2->sec > tmpNode->sec) {
                        break;
                    }
                    tmpNode2 = tmpNode2->next;
                }
                tmpNode2->prev->next = tmpNode;
                tmpNode->prev = tmpNode2->prev;
                tmpNode2->prev = tmpNode;
                tmpNode->next = tmpNode2;
                tmpNode = nextNode;
            }
            continue;
        }
        tmpNode = tmpNode->next;
    }
    DEBUG("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n\n");
}

// 初始化信号处理
static int sigalarm_init(void)
{
    struct sigaction new, old;
    int ret = -1;
    DEBUG("F:%s,f:%s,L:%d start\n", __FILE__, __func__, __LINE__);

    memset(&new, 0, sizeof(struct sigaction));
    memset(&old, 0, sizeof(struct sigaction));
    new.sa_flags = 0;
    new.sa_handler = sigalarm_handle;
    sigaction(SIGALRM, &new, &old);
    ret = set_timer(1);
    DEBUG("F:%s,f:%s,L:%d end, ret=%d\n", __FILE__, __func__, __LINE__,ret);
    return ret;
}

//线程数据处理
static void thread_data_handle(TimerThread *threadData)
{
    DEBUG("F:%s,f:%s,L:%d [PID:%lu] start data handle\n", __FILE__, __func__, __LINE__, pthread_self());
    int i = 0;
    TimerNode *timerNode = NULL;
    for (i = 0; i < MAX_ARRAY_SIZE; i++) {
        DEBUG("F:%s,f:%s,L:%d  i=%d\n", __FILE__, __func__, __LINE__,i);
        threadData->arrindex = i;
        timerNode = threadData->timerArray[i];
        if (timerNode != NULL) {
            DEBUG("F:%s,f:%s,L:%d [PID:%lu] handle index:%d timerID:%u\n", __FILE__, __func__, __LINE__, pthread_self(), i, timerNode->timer_id);
            DEBUG("[%ld]F:%s,f:%s,L:%d  timer_cb start---------------------------------------\n", time(NULL),__FILE__, __func__, __LINE__);
            timerNode->timer_cb(timerNode->arg);
            DEBUG("[%ld]F:%s,f:%s,L:%d  timer_cb end++++++++++++++++++++++++++++++++++++++++++\n", time(NULL), __FILE__, __func__, __LINE__);
            threadData->timerArray[i] = NULL;
        }
    }
    threadData->arrindex = 0;
    threadData->haveData = 0;
    DEBUG("F:%s,f:%s,L:%d [PID:%lu] end data handle\n", __FILE__, __func__, __LINE__, pthread_self());
}

//线程处理函数
static void* timer_thread_handle(void *arg)
{
    DEBUG("F:%s,f:%s,L:%d [PID:%lu] start\n", __FILE__, __func__, __LINE__, pthread_self());
    TimerThread *threadData = (TimerThread*)arg;
    if (threadData->threadID != pthread_self()) {
        printf("threadData->threadID:%lu, pthread_self:%lu, error\n",threadData->threadID, pthread_self());
        return 0;
    }

    pthread_mutex_lock(threadData->mutex);
    while (1){
        while (threadData->haveData == 0) {
            DEBUG("F:%s,f:%s,L:%d [PID:%lu] cond wait...\n", __FILE__, __func__, __LINE__, pthread_self());
            pthread_cond_wait(threadData->cond, threadData->mutex);
        }
        DEBUG("F:%s,f:%s,L:%d [PID:%lu] cond wake up\n", __FILE__, __func__, __LINE__, pthread_self());
        thread_data_handle(threadData);
    }
    DEBUG("F:%s,f:%s,L:%d [PID:%lu] end\n", __FILE__, __func__, __LINE__, pthread_self());
    pthread_mutex_unlock(threadData->mutex);
    return 0;
}

// 初始化处理定时任务的线程
static int pthread_init(void)
{
    int i = 0, ret = -1;
    ThreadArray = calloc(MAX_THREAD_COUNT, sizeof(TimerThread));
    for (i = 0; i < MAX_THREAD_COUNT; i++) {
        ThreadArray[i].mutex = calloc(1, sizeof(pthread_mutex_t));
        ThreadArray[i].cond = calloc(1, sizeof(pthread_cond_t));
        if (ThreadArray[i].mutex == NULL || ThreadArray[i].cond == NULL) {
            printf("F:%s,f:%s,L:%d thread %d, calloc mutex or cond error!\n", __FILE__, __func__, __LINE__, i);
            continue;
        }
        pthread_mutex_init(ThreadArray[i].mutex, NULL);
        pthread_cond_init(ThreadArray[i].cond, NULL);
        ret = pthread_create(&ThreadArray[i].threadID, NULL, timer_thread_handle, &ThreadArray[i]);
        if (ret == -1)
            printf("F:%s,f:%s,L:%d pthread_create %d error!\n", __FILE__, __func__, __LINE__, i);
        DEBUG("F:%s,f:%s,L:%d pthread_create %d [PID:%lu] success!\n", __FILE__, __func__, __LINE__, i, ThreadArray[i].threadID);
    }
}

// 初始化定时器链表
static int timer_init(void)
{
    int ret = -1;
    DEBUG("F:%s,f:%s,L:%d start\n", __FILE__, __func__, __LINE__);
    //初始定时器处理线程,把时间到的定时任务分发给这些线程
    pthread_init();

    //初始链表头节点和尾节点
    g_timer_head = calloc(1, sizeof(TimerNode));
    g_timer_tail = calloc(1, sizeof(TimerNode));
    if (g_timer_head == NULL || g_timer_tail == NULL) {
        DEBUG("F:%s,f:%s,L:%d calloc error\n", __FILE__, __func__, __LINE__);
        return -1;
    }
    g_timer_head->next = g_timer_tail;
    g_timer_tail->prev = g_timer_head;
    g_timer_head->prev = NULL;
    g_timer_tail->next = NULL;

    //初始互斥锁，用于多线程并发添加或删除定时任务
    g_mutex = calloc(1, sizeof(pthread_mutex_t));
    ret = pthread_mutex_init(g_mutex, NULL);
    if (ret != 0) {
        DEBUG("F:%s,f:%s,L:%d calloc error\n", __FILE__, __func__, __LINE__);
        return -1;
    }

    //初始信号处理
    ret = sigalarm_init();
    DEBUG("F:%s,f:%s,L:%d end, ret=%d\n", __FILE__, __func__, __LINE__, ret);
    return ret;
}

//获取定时器链表大小
static int timer_count()
{
    int count = 0;
    TimerNode *tmpNode = NULL;

    for(tmpNode = g_timer_head->next; tmpNode != g_timer_tail;tmpNode = tmpNode->next)
        count++;
    return count;
}

// 添加定时任务
int timer_add(int sec, TimerFunc *timer_cb, void *arg, int repeat)
{
    static unsigned int timer_id = 0;
    TimerNode *newNode = NULL, *tmpNode = NULL;
  
    // init
    if (g_timer_head == NULL) {
        // create timer list
        if (timer_init() == -1) return -1;
    }
    //头节点记录绝对时间
    g_timer_head->sec = time(NULL);

    // count
    if (timer_count() > MAX_TIMER_COUNT) return -1;

    newNode = calloc(1, sizeof(TimerNode));
    if (newNode == NULL) return -1;
    newNode->sec = sec;
    newNode->sec_repeat = sec;
    newNode->timer_cb = timer_cb;
    newNode->arg = arg;
    newNode->repeat = repeat;
    newNode->timer_id = timer_id++;

    // 插入定时任务
    pthread_mutex_lock(g_mutex);
    DEBUG("[PID:%lu] locked\n", pthread_self());
    for(tmpNode = g_timer_head->next; tmpNode != g_timer_tail;tmpNode = tmpNode->next){
        if (tmpNode->sec > newNode->sec) {
            break;
        }
    }
    tmpNode->prev->next = newNode;
    newNode->prev = tmpNode->prev;
    newNode->next = tmpNode;
    tmpNode->prev = newNode;
    printf("F:%s,f:%s,L:%d,[ADD]: timer_id:%d, sec:%ld repeat:%d\n", __FILE__, __func__, __LINE__,newNode->timer_id, newNode->sec, newNode->repeat);
    pthread_mutex_unlock(g_mutex);
    DEBUG("[PID:%lu] unlocked\n", pthread_self());
    return newNode->timer_id;
}

//删除定时任务
int timer_del(int timer_id)
{
    TimerNode *tmpNode = NULL;

    pthread_mutex_lock(g_mutex);
    DEBUG("[PID:%lu] locked\n", pthread_self());
    for(tmpNode = g_timer_head->next; tmpNode != g_timer_tail;tmpNode = tmpNode->next){
        if (tmpNode->timer_id == timer_id) {
            tmpNode->prev->next = tmpNode->next;
            tmpNode->next->prev = tmpNode->prev;
            free(tmpNode);
            DEBUG("F:%s,f:%s,L:%d,[DEL]: timer_id:%d, sec:%ld repeat:%d\n", __FILE__, __func__, __LINE__,tmpNode->timer_id, tmpNode->sec, tmpNode->repeat);
            pthread_mutex_unlock(g_mutex);
            DEBUG("[%lu] unlocked\n", pthread_self());
            return 0;
        }
    }
    DEBUG("F:%s,f:%s,L:%d end, timer_id:%d not find\n", __FILE__, __func__, __LINE__, tmpNode->timer_id);
    pthread_mutex_unlock(g_mutex);
    DEBUG("[PID:%lu] unlocked\n", pthread_self());
    return -1;
}
