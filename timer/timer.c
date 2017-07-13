#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>
#include <pthread.h>

#define MAX_TIMER_COUNT (50)

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

static TimerNode *g_timer_head = NULL;
static TimerNode *g_timer_tail = NULL;
static pthread_mutex_t *g_mutex = NULL;

static int timer_init(void);
static int timer_count(void);
static void sigalarm_handle(int sigint);
static int set_timer(int sec);
int timer_add(int sec, TimerFunc *timer_cb, void *arg, int repeat);
int timer_del(int timer_id);

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

    DEBUG("[%ld]F:%s,f:%s,L:%d,timer_count:%d\n", time(NULL), __FILE__, __func__, __LINE__, timer_count());

    DEBUG("--------------------------------------------------------------------\n");
    for(tmpNode = g_timer_head->next; tmpNode != g_timer_tail; tmpNode = tmpNode->next){
        // 时间到则执行定时任务，没到时间则更新剩余时间sec
        tmpNode->sec--;
        //DEBUG("F:%s,f:%s,L:%d,[CHK]: timer_id:%d, sec:%ld repeat:%d\n", __FILE__, __func__, __LINE__,tmpNode->timer_id, tmpNode->sec, tmpNode->repeat);
        if (tmpNode->sec <= 0) {
            DEBUG("F:%s,f:%s,L:%d,[GET]: timer_id:%d, sec:%ld repeat:%d\n", __FILE__, __func__, __LINE__,tmpNode->timer_id, tmpNode->sec, tmpNode->repeat);
            tmpNode->timer_cb(tmpNode->arg);
        }
    }

    // 整理任务链表，删除超时的单次任务，重排重复任务
    tmpNode = g_timer_head->next;
    while (tmpNode != g_timer_tail) {
        //DEBUG("F:%s,f:%s,L:%d,[RECHK]: timer_id:%d, sec:%ld repeat:%d\n", __FILE__, __func__, __LINE__,tmpNode->timer_id, tmpNode->sec, tmpNode->repeat);
        if (tmpNode->sec > 0) {//任务链表时有序的，如果当前节点没到时间，则其后节点一定也没到时间
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


// 初始化定时器链表
static int timer_init(void)
{
    int ret = -1;
    DEBUG("F:%s,f:%s,L:%d start\n", __FILE__, __func__, __LINE__);
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

    //初始互斥锁，用于多线程并发访问
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
    DEBUG("F:%s,f:%s,L:%d,[ADD]: timer_id:%d, sec:%ld repeat:%d\n", __FILE__, __func__, __LINE__,newNode->timer_id, newNode->sec, newNode->repeat);
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
