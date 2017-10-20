
// ll.h

#ifndef LL_H
#define LL_H

#include <stdlib.h>
#include <stdbool.h>

typedef struct LinkedListNode_t {
	struct LinkedListNode_t *next;
	struct LinkedListNode_t *previous;
	void *data;
} LinkedListNode_t;

typedef struct LinkedList_t {
	LinkedListNode_t *head;
	LinkedListNode_t *tail;
	int length;
} LinkedList_t;

LinkedList_t * ll_create();
int ll_insertAt( LinkedList_t *, void *, int );
int ll_removeAt( LinkedList_t *, int  );
void * ll_find( LinkedList_t *, void *, int (*)(void*,void*));

#endif