/** @file log.h */

#ifndef __LOG_H_
#define __LOG_H_

typedef struct _log_entry_t {
	struct _log_entry_t *prev;
	struct _log_entry_t *next;
	char* value;

} log_entry_t;

typedef struct _log_t {
	log_entry_t *tailEntry; //this is the tail
	int* size;

} log_t;

void log_init(log_t *l);
void log_destroy(log_t* l);
void log_push(log_t* l, const char *item);
char *log_search(log_t* l, const char *prefix);
void log_printAll(log_t *l);

#endif
