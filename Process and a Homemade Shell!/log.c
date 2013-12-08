/** @file log.c */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "log.h"

/**
 * Initializes the log.
 *
 * You may assuem that:
 * - This function will only be called once per instance of log_t.
 * - This function will be the first function called per instance of log_t.
 * - All pointers will be valid, non-NULL pointer.
 *
 * @returns
 *   The initialized log structure.
 */
void log_init(log_t *l) {
	l->tailEntry = malloc(sizeof(log_entry_t));
	l->tailEntry->prev = NULL;
	l->tailEntry->next = NULL;
	l->tailEntry->value = NULL;
	//printf("Log init successful\n");
}

/**
 * Frees all internal memory associated with the log.
 *
 * You may assume that:
 * - This function will be called once per instance of log_t.
 * - This funciton will be the last function called per instance of log_t.
 * - All pointers will be valid, non-NULL pointer.
 *
 * @param l
 *    Pointer to the log data structure to be destoryed.
 */
void log_destroy(log_t* l) {
	log_entry_t *cur = l->tailEntry;
	log_entry_t *mightBeLast = cur;

	for (; cur;) {
		free(cur->value);
		mightBeLast = cur;
		cur = cur->prev;
		if (cur) {
			free(cur->next);
		} else {
			free(mightBeLast);
			break;
		}
	}
}

/**
 * Push an item to the log stack.
 *
 *
 * You may assume that:
 * - All pointers will be valid, non-NULL pointer.
 *
 * @param l
 *    Pointer to the log data structure.
 * @param item
 *    Pointer to a string to be added to the log.
 */
void log_push(log_t* l, const char *item) {
	//printf("now pushing command: %s\nprint all the command right now\n",item);
	//log_printAll(l);
	size_t cpy = (strlen(item) + 1) * sizeof(char);
	char* new = malloc(cpy);
	memcpy(new,item,cpy);
	if (!l->tailEntry->value) {	//if right after init
		l->tailEntry->value = new;
		return;
	}
	log_entry_t *cur = l->tailEntry;
	cur->next = malloc(sizeof(log_entry_t));
	cur->next->prev = cur;
	//printf("tail value: %s\ntail prev value: %s\n",cur->next->value,cur->value);
	cur = cur->next;
	cur->next = NULL;
	cur->value=new;
	l->tailEntry = cur;

	//printf("print all the command after func\n");
	//log_printAll(l);
	//printf("tail value: %s\ntail prev value: %s\n",cur->value,cur->prev->value);
}

/**
 * Preforms a newest-to-oldest search of log entries for an entry matching a
 * given prefix.
 *
 * This search starts with the most recent entry in the log and
 * compares each entry to determine if the query is a prefix of the log entry.
 * Upon reaching a match, a pointer to that element is returned.  If no match
 * is found, a NULL pointer is returned.
 *
 *
 * You may assume that:
 * - All pointers will be valid, non-NULL pointer.
 *
 * @param l
 *    Pointer to the log data structure.
 * @param prefix
 *    The prefix to test each entry in the log for a match.
 *
 * @returns
 *    The newest entry in the log whose string matches the specified prefix.
 *    If no strings has the specified prefix, NULL is returned.
 */
char *log_search(log_t* l, const char *prefix) {
	if (!l->tailEntry->value) {	//if right after init
		return NULL;
	}
	log_entry_t *cur = l->tailEntry;
	for (; cur; cur = cur->prev) {
		if (strncmp(prefix, cur->value, strlen(prefix)) == 0) {
			return cur->value;
		}
	}
	return NULL;
}

void log_printAll(log_t *l) {
	if (!l->tailEntry->value) {	//if right after init
		return;
	}
	log_entry_t *head = l->tailEntry;
	while (head->prev) {
		head = head->prev;
	}
	log_entry_t *cur = head;
	for (; cur; cur = cur->next) {
		printf("%s\n", cur->value);
	}
	return;
}
