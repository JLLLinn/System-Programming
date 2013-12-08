/** @file shell.c */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "log.h"
#define LAR_BUF_SIZE 100

log_t Log;

/**
 * Starting point for shell.
 */
const char *prompt = "(pid=%i)%s$ ";
const char *pInfo = "Command executed by pid=%d\n";

void simple_shell() {
	ssize_t line_ct;
	size_t line_size = 0;
	char *line = NULL;
	pid_t pid;
	char* cwd;
	char cwdBuf[LAR_BUF_SIZE];
	char* matchp = NULL;
	char bangFlag = 0;
	for (;;) {
		pid = getpid();
		cwd = getcwd(cwdBuf, LAR_BUF_SIZE);

		fflush (stdout);

		if (matchp) {
			free(line);
			line_size = 0;
			line = matchp;
			matchp = NULL;
			bangFlag = 1;
			//printf("\n");
		} else {
			fflush(stdout);
			printf(prompt, pid, cwd);
			line_ct = getline(&line, &line_size, stdin);
			if (line_ct <= 0) {
				printf("\n");
				break;
			}
			line[--line_ct] = '\0'; //remove trailing newline
			//printf("line ct is%d\n",line_ct);
			//printf("size is: %d\n",line_size);
		}

		if (!strcmp(line, "exit")) {
			printf(pInfo, pid);
			break;
		}

		if (!strncmp(line, "cd ", 3)) {
			printf(pInfo, pid);
			log_push(&Log, line);
			if (chdir(line + 3) == -1) {
				printf("%s: No such file or directory\n", line + 3);
			}
			if (bangFlag == 1) {
				free(line);
				bangFlag = 0;
			}
			continue;
		}

		if (!strcmp(line, "!#")) {
			printf(pInfo, pid);
			log_printAll(&Log);
			continue;
		}

		if (line[0] == '!') {
			printf(pInfo, pid);
			if (strlen(line) > 1) {
				char* matchpp;
				if ((matchpp = log_search(&Log, line + 1)) != NULL) {
					size_t cpy = (strlen(matchpp) + 1) * sizeof(char);
					matchp = (char*) malloc(cpy);
					memcpy(matchp, matchpp, cpy);
					printf("%s matches %s\n", line + 1, matchp);
					continue;
				} else {
					printf("No Match\n");
					continue;
				}
			} else {
				continue;
			}
		}

		//fork(); ->execvp(argv[0], argv)//full credit
		//system(line);
		log_push(&Log, line);
		pid_t child_pid = fork();

		if (child_pid == 0) {
			printf(pInfo, getpid());
			char* cur = line;
			int argc = 1;
			int i = 0, l = strlen(cur);

			for (; i < l; cur++, i++) {
				if (*cur == ' ' && *(cur + 1) != ' ') {
					argc++;
				}
			}
			//printf("hey i get to here, argc is: %d\n",argc);
			char**argv = malloc(sizeof(char*) * (argc + 1));
			i = 1;
			int len = 0;
			argv[0] = line;
			for (cur = line; len < l; cur++, len++) {
				if (*cur == ' ' && *(cur + 1) != ' ') {
					argv[i] = cur + 1;
					*cur = '\0';
					i++;
				}
			}
			//printf("hey i get to here, argv[0] is: %s, i is: %d\n",argv[0],i);
			argv[i] = NULL;
			//printf("command is: %s|%s|%s END\n",argv[0],argv[1],argv[2]);
			if (execvp(argv[0], argv) == -1) {
				int c = 0;
				for (; c < argc; c++) {
					printf("%s ", argv[c]);
				}
				printf(": not found\n");
			}
			free(argv);

			free(line);
			log_destroy(&Log);
			exit(1);

			//break;
		} else {
			waitpid(child_pid, NULL, WUNTRACED);
		}
		if (bangFlag == 1) {
			free(line);
			bangFlag = 0;
		}

	}

	free(line);
}

int main() {
	log_init(&Log);
	simple_shell();
	log_destroy(&Log);
	//printf("FINISHED!\n");
	return 0;

}
