#include <stdio.h>
#include <stdlib.h>
#include "list.h"

NODE* list_create(void)
{
	NODE* head = calloc(1, sizeof(NODE));
	NODE* tail = calloc(1, sizeof(NODE));

	if(!head || !tail)
		return NULL;

	head->next = tail;

	return head;
}

int list_size(NODE *head)
{
	int i = 0;
	NODE *node = NULL;
	for(node = head; node; node = node->next)
		i++;
	return i-2;	//exclude head and tail
}

int list_insert(NODE* head, void *data, int index)
{
	int i;
	NODE *node, *newNode;

	if(!head || index < 0)
		return -1;

	if(index > list_size(head) + 1)
		return -1;

	node = head;
	for(i = 0; i < index; i++)
		node = node->next;
	newNode = calloc(1,sizeof(NODE));
	if(!newNode)
		return -1;
	printf("data:%p\n",data);
	newNode->data = data;
	printf("newNode->data:%p\n",newNode->data);
	newNode->next = node->next;
	node->next = newNode;
	return 0;
}

void* list_remove(NODE *head, int index)
{
	int i;
	NODE *node, *retNode;
	void *retdata;

	if(!head || index < 0)
		return NULL;
	
	if(index > list_size(head) -1)
		return NULL;

	node = head;
	for(i = 0; i < index; i++)
		node = node->next;
	retNode = node->next;
	node->next = node->next->next;
	retdata = retNode->data;
	return retdata;
}	

void* list_modif(NODE *head, void *data, int index)
{
	int i;
	NODE *node;
	void* olddata;

	if(!head || !data)
		return NULL;
	if(index > list_size(head) - 1)
		return NULL;
	
	node = head;
	for(i = 0; i <= index; i++)
		node = node->next;
	olddata = node->data;
	node->data = data;
	return olddata;
}

void* list_getdata(NODE *head, int index)
{
	int i;
	NODE *node;

	if(!head)
		return NULL;
	if(index > list_size(head) - 1)
		return NULL;

	node = head;
	for(i = 0; i <= index; i++)
		node = node->next;
	return node->data;
}


int list_sort(NODE *head, size_t num, size_t width, COMPARFUNC compar)
{
	void **argv = NULL;
	NODE *node;
	int i = 0;

	if(!head || num <= 0 || width <= 0)
		return -1;

	node = head;
	//保存链表各节点数据到argv
	while(node->next){
		node = node->next;
		argv[i] = node->data;
		i++;
	}

	qsort(argv, i, sizeof(void*), compar);

	//把排好序的argv中的数据存回链表各个节点
	i = 0;
	node = head;
	while(node->next){
		node = node->next;
		node->data = argv[i];
		i++;
	}
	return 0;
}

int list_destroy(NODE *head)
{
	NODE *p, *q;

	if(!head)
		return -1;
	
	p = head;
	while(p){
		q = p->next;
		free(p->data);
		free(p);
		p = q;
	}
	return 0;
}

