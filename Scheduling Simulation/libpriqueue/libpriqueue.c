/** @file libpriqueue.c
 */

#include <stdlib.h>
#include <stdio.h>

#include "libpriqueue.h"

/**
 Initializes the priqueue_t data structure.
 
 Assumtions
 - You may assume this function will only be called once per instance of priqueue_t
 - You may assume this function will be the first function called using an instance of priqueue_t.
 @param q a pointer to an instance of the priqueue_t data structure
 @param comparer a function pointer that compares two elements.
 See also @ref comparer-page
 */
void priqueue_init(priqueue_t *q, int (*comparer)(const void *, const void *)) {
	q->comparer = comparer;
	q->headMin = malloc(sizeof(priqueue_entry_t));
	q->headMin->next = NULL;
	q->headMin->data = NULL;

	q->size = 0;
}

/**
 Inserts the specified element into this priority queue.

 @param q a pointer to an instance of the priqueue_t data structure
 @param ptr a pointer to the data to be inserted into the priority queue
 @return The zero-based index where ptr is stored in the priority queue, where 0 indicates that ptr was stored at the front of the priority queue.
 */
int priqueue_offer(priqueue_t *q, void *ptr) {
	if (!q->headMin->data) { //if right after init
		q->headMin->data = ptr;
		(q->size)++;
		//printf("1head: %d\n",*(int*)(q->headMin->data));
		//printf("right after init insert %d\n",*(int*)ptr);
		return 0;
	} else {		//if we already have head
					//printf("0head: %d\n",*(int*)(q->headMin->data));
		int index = 0;
		priqueue_entry_t* cur;
		if (q->comparer(ptr, q->headMin->data) < 0) {//if it is small than headMin, than this become headMin
			cur = malloc(sizeof(priqueue_entry_t));
			cur->next = q->headMin;
			q->headMin = cur;
			q->headMin->data = ptr;
			(q->size)++;
			//printf("small than headMin insert %d\n",*(int*)ptr);
			return 0;
		}		//if not then do normal stuff

		cur = q->headMin;
		priqueue_entry_t* temp;
		int i = 0;
		//printf("2head: %d\n",*(int*)(q->headMin->data));
		while (cur->next) {		//traverse every entry except the last one
			i++;
			//printf("3head: %d\n",*(int*)(q->headMin->data));
			if (q->comparer(ptr, cur->next->data) < 0) {//if this is satisfied, start inserting
				temp = malloc(sizeof(priqueue_entry_t));
				temp->next = cur->next;
				cur->next = temp;
				temp->data = ptr;
				(q->size)++;
				//printf("in the middle insert %d\n",*(int*)ptr);
				return i;
			}
			cur = cur->next;
		}
		i++;
		//printf("4head: %d\n",*(int*)(q->headMin->data));
		cur->next = malloc(sizeof(priqueue_entry_t));
		cur = cur->next;
		cur->next = NULL;
		cur->data = ptr;
		(q->size)++;
		//printf("5head: %d\n",*(int*)(q->headMin->data));
		//printf("after tail insert %d\n",*(int*)ptr);
		return i;
	}
	return -1;
}

/**
 Retrieves, but does not remove, the head of this queue, returning NULL if
 this queue is empty.
 
 @param q a pointer to an instance of the priqueue_t data structure
 @return pointer to element at the head of the queue
 @return NULL if the queue is empty
 */
void *priqueue_peek(priqueue_t *q) {
	if (q->headMin->data) {		//if we already have head
		return q->headMin->data;
	}
	return NULL;
}

/**
 Retrieves and removes the head of this queue, or NULL if this queue
 is empty.
 
 @param q a pointer to an instance of the priqueue_t data structure
 @return the head of this queue
 @return NULL if this queue is empty
 */
void *priqueue_poll(priqueue_t *q) {
	if (q->headMin->data) {		//if we already have head
		void* ret = q->headMin->data;
		if (q->headMin->next) {		//if there is something after the head
			priqueue_entry_t* temphead = q->headMin;//this is for freeing the head
			q->headMin = q->headMin->next;
			free(temphead);
		} else {		//if it's only the head
			q->headMin->next = NULL;
			q->headMin->data = NULL;
		}
		(q->size)--;
		return ret;
	}
	return NULL;		//if we have nothing
}

/**
 Returns the number of elements in the queue.
 
 @param q a pointer to an instance of the priqueue_t data structure
 @return the number of elements in the queue
 */
int priqueue_size(priqueue_t *q) {
	return q->size;
}

/**
 Destroys and frees all the memory associated with q.
 
 @param q a pointer to an instance of the priqueue_t data structure
 */
void priqueue_destroy(priqueue_t *q) {
	priqueue_entry_t* cur = q->headMin;
	priqueue_entry_t* prev = q->headMin;
	while (cur) {
		prev = cur;
		cur = cur->next;
		free(prev);
	}
}
