/** @file libmapreduce.c */
/* 
 * CS 241
 * The University of Illinois
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>

#include "libmapreduce.h"
#include "libds/libds.h"

pid_t * pids;
int epoll_fd;
pthread_t worker;
int numStrings;
int** fds;
static const int BUFFER_SIZE = 2048; /**< Size of the buffer used by read_from_fd(). */

/**
 * Adds the key-value pair to the mapreduce data structure.  This may
 * require a reduce() operation.
 *
 * @param key
 *    The key of the key-value pair.  The key has been malloc()'d by
 *    read_from_fd() and must be free()'d by you at some point.
 * @param value
 *    The value of the key-value pair.  The value has been malloc()'d
 *    by read_from_fd() and must be free()'d by you at some point.
 * @param mr
 *    The pass-through mapreduce data structure (from read_from_fd()).
 */
static void process_key_value(const char *key, const char *value,
		mapreduce_t *mr) {
	unsigned long revision = 0;
	const char* value2 = datastore_get(&(mr->ds), key, &revision);
	
	if (value2) {
		const char* updateValue = (*mr->myreduce)(value2, value);
		int newRev = datastore_update(&(mr->ds), key, updateValue, revision);
		if (newRev == 0)
			printf("ERROR, no key found!!\n");
		free((void*) value2);
		free((void*) key);
		free((void*) value);
		free((void*) updateValue);
	} else {
		datastore_put(&(mr->ds), key, value);
		free((void*)key);
		free((void*)value);
	}
}

/**
 * Helper function.  Reads up to BUFFER_SIZE from a file descriptor into a
 * buffer and calls process_key_value() when for each and every key-value
 * pair that is read from the file descriptor.
 *
 * Each key-value must be in a "Key: Value" format, identical to MP1, and
 * each pair must be terminated by a newline ('\n').
 *
 * Each unique file descriptor must have a unique buffer and the buffer
 * must be of size (BUFFER_SIZE + 1).  Therefore, if you have two
 * unique file descriptors, you must have two buffers that each have
 * been malloc()'d to size (BUFFER_SIZE + 1).
 *
 * Note that read_from_fd() makes a read() call and will block if the
 * fd does not have data ready to be read.  This function is complete
 * and does not need to be modified as part of this MP.
 *
 * @param fd
 *    File descriptor to read from.
 * @param buffer
 *    A unique buffer associated with the fd.  This buffer may have
 *    a partial key-value pair between calls to read_from_fd() and
 *    must not be modified outside the context of read_from_fd().
 * @param mr
 *    Pass-through mapreduce_t structure (to process_key_value()).
 *
 * @retval 1
 *    Data was available and was read successfully.
 * @retval 0
 *    The file descriptor fd has been closed, no more data to read.
 * @retval -1
 *    The call to read() produced an error.
 */
static int read_from_fd(int fd, char *buffer, mapreduce_t *mr) {
	/* Find the end of the string. */
	int offset = strlen(buffer);

	/* Read bytes from the underlying stream. */
	int bytes_read = read(fd, buffer + offset, BUFFER_SIZE - offset);
	if (bytes_read == 0)
		return 0;
	else if (bytes_read < 0) {
		fprintf(stderr, "error in read.\n");
		return -1;
	}

	buffer[offset + bytes_read] = '\0';

	/* Loop through each "key: value\n" line from the fd. */
	char *line;
	while ((line = strstr(buffer, "\n")) != NULL) {
		*line = '\0';

		/* Find the key/value split. */
		char *split = strstr(buffer, ": ");
		if (split == NULL)
			continue;

		/* Allocate and assign memory */
		char *key = malloc((split - buffer + 1) * sizeof(char));
		char *value = malloc((strlen(split) - 2 + 1) * sizeof(char));

		strncpy(key, buffer, split - buffer);
		key[split - buffer] = '\0';

		strcpy(value, split + 2);

		/* Process the key/value. */
		process_key_value(key, value, mr);

		/* Shift the contents of the buffer to remove the space used by the processed line. */
		memmove(buffer, line + 1, BUFFER_SIZE - ((line + 1) - buffer));
		buffer[BUFFER_SIZE - ((line + 1) - buffer)] = '\0';
	}

	return 1;
}
void* workerT(void* m) {
	mapreduce_t* mr = (mapreduce_t *) m;
	char** buf = malloc(numStrings * sizeof(char*));
	int i = 0;
	for(i = 0; i < numStrings; i++) {
		buf[i] = malloc((BUFFER_SIZE + 1) * sizeof(char));
		buf[i][0] = '\0';
	}
	
	int active_fd = numStrings;
	//printf("numStrings: %d\n", numStrings);
	while (active_fd > 0) {
		//printf("active_fd: %d\n", active_fd);
		
		struct epoll_event ev;
		epoll_wait(epoll_fd, &ev, 1, -1);
		
		int j = 0;
		for (j = 0; j < numStrings; j++) {
			if (fds[j][0] == ev.data.fd)
				break;
		}
		
		
		int ret = read_from_fd(ev.data.fd, buf[j], mr);
		if (ret == 1) {
			//printf("Successfully add buf\n");
			//printf("now is buf[%d]\n", j);
		} else if (ret == 0) {
			//this means its not reading anything, since the ev.data.fd is closed
			//printf("Close buf\n");
			epoll_ctl(epoll_fd, EPOLL_CTL_DEL, ev.data.fd, NULL);
			
			active_fd--;
			//printf("active_fd--: %d\n",active_fd);
		} else {
			fprintf(stderr, "ERROR in read_from_fd\n");
			break;
		}
	}
	for(i = 0; i < numStrings; i++) {
		free(buf[i]);
	}
	free(buf);
	
	return NULL;
}
/**
 * Initialize the mapreduce data structure, given a map and a reduce
 * function pointer.
 */
void mapreduce_init(mapreduce_t *mr, void (*mymap)(int, const char *),
		const char *(*myreduce)(const char *, const char *)) {
	epoll_fd = epoll_create(10);
	mr->mymap = mymap;
	mr->myreduce = myreduce;
	datastore_init(&mr->ds);
}

/**
 * Starts the map() processes for each value in the values array.
 * (See the MP description for full details.)
 */
void mapreduce_map_all(mapreduce_t *mr, const char **values) {
	int i = 0;
	while (values[i]) {
		i++;
	}
	numStrings = i;
	pids = malloc(numStrings * sizeof(pid_t));

	struct epoll_event event[numStrings];
	fds = malloc(numStrings * sizeof(int*));
	for (i = 0; i < numStrings; i++) {
		fds[i] = malloc(2 * sizeof(int));
		pipe(fds[i]);
		int read_fd = fds[i][0], write_fd = fds[i][1];
		pid_t pid = fork();
		if (pid == 0) {
			close(read_fd);
			//printf("write_fd: %d\n", write_fd);
			(*mr->mymap)(write_fd, values[i]);
			exit(0);
		} else if (pid > 0) {
			close(write_fd);
			pids[i] = pid;
		}
		memset(&event[i], 0, sizeof(event[i]));
		event[i].events = EPOLLIN;
		event[i].data.fd = read_fd;
		//printf("add read_fd: %d\n", read_fd);
		epoll_ctl(epoll_fd, EPOLL_CTL_ADD, read_fd, &event[i]);
	}
	pthread_create(&worker, NULL, &workerT, mr);
}

/**
 * Blocks until all the reduce() operations have been completed.
 * (See the MP description for full details.)
 */
void mapreduce_reduce_all(mapreduce_t *mr) {
	int status;
	int i = 0;
	for(i = 0; i < numStrings; i++){
		waitpid(pids[i], &status, 0);
	}
	pthread_join(worker, NULL);
}

/**
 * Gets the current value for a key.
 * (See the MP description for full details.)
 */
const char *mapreduce_get_value(mapreduce_t *mr, const char *result_key) {
	return datastore_get(&(mr->ds), result_key, NULL);
}

/**
 * Destroys the mapreduce data structure.
 */
void mapreduce_destroy(mapreduce_t *mr) {
	datastore_destroy(&(mr->ds));
	free(pids);
	
	int i = 0;
	for(i = 0; i < numStrings; i++) {
		free(fds[i]);
	}
	free(fds);
}
