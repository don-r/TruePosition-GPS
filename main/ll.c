
#include "ll.h"

LinkedList_t * ll_create() {
	LinkedList_t *list = (LinkedList_t *)malloc(sizeof(LinkedList_t));
	list->head=NULL;
	list->tail=NULL;
	list->length = 0;
	return list;
}

int ll_insertAt( LinkedList_t *list, void *node_data, int position) {
	if (position == 0 || list->length == 0) {
		LinkedListNode_t *oldHead = list->head;
		list->head = (LinkedListNode_t *)malloc( sizeof(LinkedListNode_t));
		list->head->data = node_data;
		list->head->next = oldHead;
		if (oldHead != NULL) oldHead->previous = list->head;
		else {
			list->head->previous = NULL;
			list->tail = list->head;
		}
	} else if (position < 0 || position >= list->length ) {
		LinkedListNode_t *oldTail = list->tail;
		list->tail = (LinkedListNode_t *)malloc(sizeof(LinkedListNode_t));
		list->tail->data = node_data;
		list->tail->previous = oldTail;
		if (oldTail != NULL) oldTail->next = list->tail;
		list->tail->next = NULL;
		if (list->head == NULL) list->head = list->tail;
	} else {
		LinkedListNode_t *node = list->head;
		for (int i = 0; i < position && node->next != NULL; i++) {
			node = node->next;
		}
		LinkedListNode_t *oldNext = node;
		node = (LinkedListNode_t *)malloc(sizeof(LinkedListNode_t));
		node->next = oldNext;
		node->previous = oldNext->previous;
		node->previous->data = node_data;
	}
		
	return ++list->length;
}

int ll_removeAt( LinkedList_t *list, int position ) {
	if ( list->length == 0) return 0;
	if (position == 0) {
		LinkedListNode_t *oldHead = list->head;
		list->head = list->head->next;
		if (list->head) list->head->previous = NULL;
		else list->tail = NULL;
		free( oldHead->data );
		free( oldHead);
	} else if (position < 0) {
		LinkedListNode_t *oldTail = list->tail;
		list->tail = list->tail->previous;
		if (list->tail) list->tail->next = NULL;
		else list->head = NULL;
		free( oldTail->data );
		free( oldTail );
	}
	return --list->length;
}

void * ll_find( LinkedList_t *list, void *val, int (*cmp_func)(void*,void*))  {
	bool ret = false;
	LinkedListNode_t *n = list->head;
	while ( ret != 0 && n != NULL ) {
		ret = cmp_func( val, n->data );
		if (ret != 0) n = n->next;
	}
	return n == NULL ? NULL : n->data;
}