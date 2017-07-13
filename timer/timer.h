#ifndef __TIMER_H__
#define __TIMER_H__

typedef void TimerFunc(void *arg);
extern int timer_add(int sec, TimerFunc *timer_cb, void *arg, int once);
extern int timer_del(int timer_id);

#endif //__TIMER_H__
