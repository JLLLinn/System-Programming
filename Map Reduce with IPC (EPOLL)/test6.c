/* 
 * CS 241
 * The University of Illinois
 */

#define _GNU_SOURCE
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#include "libmapreduce.h"

#define CHARS_PER_INPUT 30000
#define INPUTS_NEEDED 10

static const char *long_key = "long_key";

/* Takes input string and maps the longest
 * word to the key, long_key.
 */
void map(int fd, const char *data) {
	unsigned int i = 0;
	char* curLongestPt = data;
	int curLongestLen = 0;
	char* p = data;
	int counter = 0;

	for (i = 0; i < strlen(data); i++) {
		if (isalpha(data[i])) {
			counter++;
			continue;
		} else {
			if (counter > curLongestLen) {
				curLongestPt = p;
				curLongestLen = counter;
			}
			counter = 0;
			p = (char*)&data[i + 1];
		}
	}
	//dealing with the last word
	if (counter > curLongestLen) {
		curLongestPt = p;
		curLongestLen = counter;
	}

	char w[50];
	memcpy(w, curLongestPt, curLongestLen);
	w[curLongestLen] = '\0';

	char s[100];
	sprintf(s, "long_key: %s\n", w);

	write(fd, s, strlen(s));
	close(fd);

}

/* Takes two words and reduces to the longer of the two
 */
const char *reduce(const char *value1, const char *value2) {

	char* result;

	if (strlen(value1) > strlen(value2)) {
		asprintf(&result, "%s", value1);
	} else {
		asprintf(&result, "%s", value2);
	}
	return result;
}

int main() {
	FILE *file = fopen("alice.txt", "r");
	char s[1024];
	int i;

	char **values = malloc(INPUTS_NEEDED * sizeof(char *));
	int values_cur = 0;

	values[0] = malloc(CHARS_PER_INPUT + 1);
	values[0][0] = '\0';

	while (fgets(s, 1024, file) != NULL) {
		if (strlen(values[values_cur]) + strlen(s) < CHARS_PER_INPUT)
			strcat(values[values_cur], s);
		else {
			values_cur++;
			values[values_cur] = malloc(CHARS_PER_INPUT + 1);
			values[values_cur][0] = '\0';
			strcat(values[values_cur], s);
		}
	}

	values_cur++;
	values[values_cur] = NULL;

	fclose(file);

	mapreduce_t mr;
	mapreduce_init(&mr, map, reduce);

	mapreduce_map_all(&mr, (const char **) values);
	mapreduce_reduce_all(&mr);

	const char *result_longest = mapreduce_get_value(&mr, long_key);

	if (result_longest == NULL) {
		printf("MapReduce returned (null).  The longest word was not found.\n");
	} else {
		printf("Longest Word: %s\n", result_longest);
		free((void *) result_longest);
	}

	mapreduce_destroy(&mr);

	for (i = 0; i < values_cur; i++)
		free(values[i]);
	free(values);

	return 0;
}
