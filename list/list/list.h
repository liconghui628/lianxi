#ifndef __LIST_H__
#define __LIST_H__
#include <stdlib.h>

typedef struct node{
	struct node *next;
	void *data;
}NODE;

typedef int(*COMPARFUNC)(const void *, const void *);

//创建
NODE* list_create(void);

//插入节点
int list_insert(NODE *head, void *data, int index);

//删除节点,如果节点数据为动态分配内存，则应注意释放
void* list_remove(NODE *head, int index);

//修改节点
void* list_modif(NODE *head, void *data, int index);

//得到节点个数
int list_size(NODE *head);

//得到节点数据
void* list_getdata(NODE *head, int index);

//排序
int list_sort(NODE *head, size_t num, size_t width, COMPARFUNC compar);

//销毁链表
int list_destroy(NODE *head);

#endif
