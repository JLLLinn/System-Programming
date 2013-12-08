/** @file parmake.c */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include "parser.h"
#include "queue.h"
#include "rule.h"
#define MAX_LENGTH 200
queue_t* rules_q;
queue_t* finished_rule_q; //stores the finished rules,**stores char*

pthread_mutex_t mutex; //this is for rules_q
pthread_cond_t cond; //when a thread(rule) is finished
pthread_mutex_t finish_q_mutex; //for finished q
/**
 * Entry point to parmake.
 */
void parsed_new_target(char *target) {
	rule_t* rule = (rule_t*) malloc(sizeof(rule_t));
	rule_init(rule);
	rule->target = (char*) malloc(sizeof(char) * MAX_LENGTH);
	strcpy(rule->target, target);

	queue_enqueue(rules_q, rule); //insert this rule to rule Q
	//queue_enqueue(targets_q,rule->target);//insert the target into target Q
}
void parsed_new_dependency(char *target, char *dependency) {

	rule_t * tempRule = NULL;
	int i = 0;
	char * depBuf = (char*) malloc(sizeof(char) * MAX_LENGTH);
	strcpy(depBuf, dependency);

	for (i = 0; i < queue_size(rules_q); i++) {
		tempRule = (rule_t*) queue_at(rules_q, i);
		if (strcmp(tempRule->target, target) == 0) {
			queue_enqueue(tempRule->deps, depBuf);
			return;
		}
	}

	fprintf(stderr, "No Matching Target Found In parsed_new_dependency!");
	return;
}
void parsed_new_command(char *target, char *command) {
	char * cmdBuf = (char*) malloc(sizeof(char) * MAX_LENGTH);
	strcpy(cmdBuf, command);
	rule_t * tempRule = NULL;
	int i = 0;
	for (i = 0; i < queue_size(rules_q); i++) {
		tempRule = (rule_t*) queue_at(rules_q, i);
		if (strcmp(tempRule->target, target) == 0) {
			queue_enqueue(tempRule->commands, cmdBuf);
			return;

		}
	}

	fprintf(stderr, "No Matching Target Found In parsed_new_command!");
	return;
}
/*
 * return 1 if rule is finished, 0 not ready
 */
int rule_finished(char* rule_char_p) {
	int i = 0;
	//using finished_q, lock mutex
	pthread_mutex_lock(&finish_q_mutex);
	for (i = 0; i < queue_size(finished_rule_q); i++) {
		if (strcmp(rule_char_p, (char*) queue_at(finished_rule_q, i)) == 0) {
			pthread_mutex_unlock(&finish_q_mutex);
			return 1;
		}
	}
	pthread_mutex_unlock(&finish_q_mutex);
	return 0;
}

/*
 * return 1 if dependencies are all ready, 0 not ready
 */
int check_deps(rule_t* rule_p) {
	int length = queue_size(rule_p->deps);
	if (length == 0)
		return 1;	//if length is 0 then return 1;
	int i = 0;
	for (i = 0; i < length; i++) {
		char* temp = queue_at(rule_p->deps, i);
		//if this is not a file (is a rule), then check if it's finished
		if (access(temp, F_OK) == -1) {
			if (rule_finished(temp) == 0) {
				return 0;
			}
		}
	}
	return 1;
}

/*
 * this will check every rule in ruleQ to find a available rule to run
 * this will dequeue the returned rule
 */
rule_t* check_all_rule() {
	int i = 0;
	//check rules one by one
	for (i = 0; i < queue_size(rules_q); i++) {
		if (check_deps(queue_at(rules_q, i)) == 1) {
			return queue_remove_at(rules_q, i);
		}
	}
	return NULL;
}
/*
 * this will return 1 if every dependency are file
 */
int every_dependency_is_file(rule_t* rule_p) {
	int i = 0;
	char* dep;
	int length = queue_size(rule_p->deps);
	if (length == 0)
		return 0;

	for (i = 0; i < length; i++) {
		dep = queue_at(rule_p->deps, i);
		//if we find one dependency is not a file,meaning it's a rule,return 0
		if (access(dep, F_OK) == -1) {
			return 0;
		}
	}
	return 1;
}

/*
 * this will run the given rule
 */
void run_rule(rule_t* rule_p) {
	int i = 0;
	for (i = 0; i < queue_size(rule_p->commands); i++) {
		if (system(queue_at(rule_p->commands, i)) != 0) {
			exit(1);
		}
	}
}

void* run_rules(void* p) {
	rule_t* run_rule_p = NULL;
	int rule_q_size = 0;	//# of rules remain in rule_q
	int i = 0;
	do {
		pthread_mutex_lock(&mutex);
		rule_q_size = queue_size(rules_q);
		//if we can't find any available rule and the size is bigger than 0
		while (rule_q_size > 0 && (run_rule_p = check_all_rule()) == NULL) {
			pthread_cond_wait(&cond, &mutex);
			rule_q_size = queue_size(rules_q);
		}
		pthread_mutex_unlock(&mutex);
		//if the rule q is empty, this thread is done
		if (rule_q_size == 0)
			return NULL;

		//now starting to run the rule or whatever
		if (every_dependency_is_file(run_rule_p) == 1) {
			//is not a file on disk
			if (access(run_rule_p->target, F_OK) == -1) {
				run_rule(run_rule_p);
			} else {	//is a file on disk,check times
				struct stat cur_rule_statbuf;
				stat(run_rule_p->target, &cur_rule_statbuf);
				for (i = 0; i < queue_size(run_rule_p->deps); i++) {
					struct stat cur_dep_statbuf;
					stat(queue_at(run_rule_p->deps, i), &cur_dep_statbuf);
					if (cur_dep_statbuf.st_mtime - cur_rule_statbuf.st_mtime
							>= 0) {
						run_rule(run_rule_p);//run this rule and enqueue finished rule
						break;
					}
				}
			}
		} else {
			run_rule(run_rule_p);
		}

		//now we enqueue this rule to finished Q
		pthread_mutex_lock(&finish_q_mutex);
		queue_enqueue(finished_rule_q, run_rule_p->target);
		pthread_cond_broadcast(&cond);//let every body know I have finish a rule
		pthread_mutex_unlock(&finish_q_mutex);

		if (run_rule_p) {
			int j;
			for (j = 0; j < queue_size(run_rule_p->deps); j++) {
				//printf("freeing target: %s dep: %s\n",run_rule_p->target,queue_at(run_rule_p->deps,j));
				free(queue_at(run_rule_p->deps, j));
			}
			int k;
			for (k = 0; k < queue_size(run_rule_p->commands); k++) {
				//printf("freeing target: %s cmd: %s\n",run_rule_p->target,queue_at(run_rule_p->commands,k));
				free(queue_at(run_rule_p->commands, k));
			}
			rule_destroy(run_rule_p);
			free(run_rule_p);
		}

	} while (1);
	return NULL;
}

int main(int argc, char **argv) {
	int opt;
	int threads = 1;
	char* makefile = NULL;
	int i = 0;	//must be used temporary and init to 0 every time
	char** targets = NULL;
	/*
	 * part1: parse string
	 */
	while ((opt = getopt(argc, argv, "f:j:")) != -1) {
		switch (opt) {
		case 'f':
			makefile = optarg;
			break;
		case 'j':
			threads = atoi(optarg);
			break;
		default: /* '?' */
			fprintf(stderr, "Usage: %s [-j threads] [-f makefile] [targets]\n",
					argv[0]);
			exit (EXIT_FAILURE);
		}
	}
	//read in target, last element is null
	if (optind < argc) {
		targets = malloc(sizeof(char*) * (argc - optind + 1));
		for (i = 0; i < (argc - optind); i++) {
			targets[i] = argv[i + optind];
		}
		targets[i] = NULL;
	}
	char bigMakefile[] = "./Makefile";
	char smaMakefile[] = "./makefile";
	//check validation of makefile
	if (makefile) {	//if we have something in makefile
		if (access(makefile, R_OK) == -1) {	//and unable to access
			fprintf(stderr, "Can't access specified makefile path\n");
			return -1;
		}
	} else {	//if nothing in makefile
		if (access("./Makefile", R_OK) == 0) {	//if able to access makefile
			makefile = bigMakefile;
		} else if (access("./makefile", R_OK) == 0) {
			makefile = smaMakefile;
		} else {
			fprintf(stderr, "Can't access Makefile/makefile\n");
			return -1;
		}
	}
	//printf("makefile here: %s\n",makefile);

	/*
	 * part2
	 *
	 */
	rules_q = (queue_t*) malloc(sizeof(queue_t));
	finished_rule_q = (queue_t*) malloc(sizeof(queue_t));
	queue_init(rules_q);
	//queue_init(targets_q);
	queue_init(finished_rule_q);
	parser_parse_makefile(makefile, targets, &parsed_new_target,
			&parsed_new_dependency, &parsed_new_command);
	//testing
	/*rule_t* tempRule = NULL;
	 for(i = 0;i<queue_size(rules_q);i++){
	 tempRule = (rule_t*)queue_at(rules_q,i);
	 printf("target: %s\n",tempRule->target);
	 int j;
	 for(j = 0;j<queue_size(tempRule->deps);j++){
	 printf(" dependency: %s\n",(char*)queue_at(tempRule->deps,j));
	 }
	 int k;
	 for(k = 0;k<queue_size(tempRule->commands);k++){
	 printf(" cmd: %s\n",(char*)queue_at(tempRule->commands,k));
	 }
	 }*/

	/*
	 * part3
	 */
	pthread_mutex_init(&mutex, NULL);
	pthread_mutex_init(&finish_q_mutex, NULL);
	pthread_cond_init(&cond, NULL);

	pthread_t* thread_list = (pthread_t*) malloc(sizeof(pthread_t) * threads);
	for (i = 0; i < threads; i++) {
		pthread_create(&thread_list[i], NULL, run_rules, NULL);
	}

	for (i = 0; i < threads; i++) {
		pthread_join(thread_list[i], NULL);
	}

	pthread_cond_destroy(&cond);
	pthread_mutex_destroy(&mutex);
	pthread_mutex_destroy(&finish_q_mutex);

	//free memory
	rule_t* temp = NULL;
	for (i = 0; i < queue_size(rules_q); i++) {
		temp = queue_at(rules_q, i);
		int j;
		for (j = 0; j < queue_size(temp->deps); j++) {
			//printf("freeing 1dep: %s\n",queue_at(temp->deps,j));
			free(queue_at(temp->deps, j));
		}
		int k;
		for (k = 0; k < queue_size(temp->commands); k++) {
			//printf("freeing 1cmd: %s\n",queue_at(temp->commands,k));
			free(queue_at(temp->commands, k));
		}
		rule_destroy(temp);
		//printf("freeing 1tar: %s\n",queue_at(temp->target,k));
		free(temp->target);
		free(temp);
	}
	queue_destroy(rules_q);
	for (i = 0; i < queue_size(finished_rule_q); i++) {
		//printf("freeing tar: %s\n",queue_at(finished_rule_q,i));
		free(queue_at(finished_rule_q, i));
	}
	queue_destroy(finished_rule_q);
	free(thread_list);
	free(rules_q);
	free(finished_rule_q);
	free(targets);
	return 0;
}
