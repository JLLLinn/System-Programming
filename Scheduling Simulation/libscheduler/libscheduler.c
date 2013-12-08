/** @file libscheduler.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libscheduler.h"
#include "../libpriqueue/libpriqueue.h"

/**
 Stores information making up a job to be scheduled including any statistics.

 You may need to define some global variables or a struct to store your job queue elements. 
 */

typedef struct _job_entry_t {
	int valid; //0 false, 1 true
	int id;
	int arrivial, leave;
	int duration, remaining;
	int respond;
	int priority;
	int lastTimeInCore; //the last time it entered a core, used to calculate the remaining time.
	//remaining = remaining - (curTime - lastTimeInCore)
	//everytime the job gets into a core this gets updated
	struct _job_entry_t* next;	//points to next job
} job_entry_t;

typedef struct _job_t {
	int jobCounter;	//how many cores died in this core
	int waiting, turnaround, respond;//set to 0 when initialized, update every time a job dies.
	int currentJob;	//stores the current running job's ID, -1 means idle
	job_entry_t* curJobPointer;	//current job's priority
} job_t;

scheme_t schemeType;

int coreNumber;
job_t* corelistById;
int jobNumber;
job_entry_t* joblistById;

priqueue_t* jobQ;

int FCFScompare(const void * a, const void * b) {
	return (((job_entry_t*) a)->arrivial - ((job_entry_t*) b)->arrivial);
}

int SJFcompare(const void * a, const void * b) {
	if (((job_entry_t*) a)->duration == ((job_entry_t*) b)->duration) {
		return (((job_entry_t*) a)->arrivial - ((job_entry_t*) b)->arrivial);
	}
	return (((job_entry_t*) a)->duration - ((job_entry_t*) b)->duration);
}

int PRIcompare(const void * a, const void * b) {
	if (((job_entry_t*) a)->priority == ((job_entry_t*) b)->priority) {
		return (((job_entry_t*) a)->arrivial - ((job_entry_t*) b)->arrivial);
	}
	return (((job_entry_t*) a)->priority - ((job_entry_t*) b)->priority);
}

int RRcompare(const void * a, const void * b) {
	return 1;	//return 1 since it always goes to the end
}

int PSJFcompare(const void * a, const void * b) {
	if (((job_entry_t*) a)->remaining == ((job_entry_t*) b)->remaining) {
		return (((job_entry_t*) a)->arrivial - ((job_entry_t*) b)->arrivial);
	}
	return (((job_entry_t*) a)->remaining - ((job_entry_t*) b)->remaining);
}
/**
 Initalizes the scheduler.
 
 Assumptions:
 - You may assume this will be the first scheduler function called.
 - You may assume this function will be called once once.
 - You may assume that cores is a positive, non-zero number.
 - You may assume that scheme is a valid scheduling scheme.

 @param cores the number of cores that is available by the scheduler. These cores will be known as core(id=0), core(id=1), ..., core(id=cores-1).
 @param scheme  the scheduling scheme that should be used. This value will be one of the six enum values of scheme_t
 */
void scheduler_start_up(int cores, scheme_t scheme) {

	schemeType = scheme;
	coreNumber = cores;
	jobQ = malloc(sizeof(priqueue_t));

	corelistById = malloc((cores) * sizeof(job_t));	//for every core there is a job_t
	int i = 0;
	for (i = 0; i < cores; i++) {
		corelistById[i].waiting = 0;
		corelistById[i].turnaround = 0;
		corelistById[i].respond = 0;
		corelistById[i].currentJob = -1;	//idle
		corelistById[i].jobCounter = 0;
		corelistById[i].curJobPointer = NULL;
	}

	jobNumber = 0;
	joblistById = malloc(sizeof(job_entry_t));

	joblistById->valid = 0;	//initialize this job, header contains nothing
	joblistById->id = -1;
	joblistById->arrivial = -1;
	joblistById->leave = -1;
	joblistById->duration = -1;
	joblistById->remaining = -1;
	joblistById->respond = -1;
	joblistById->priority = -1;
	joblistById->next = NULL;
	joblistById->lastTimeInCore = -1;

	if (scheme == FCFS) {	//change for other type
		priqueue_init(jobQ, FCFScompare);
		return;
	}
	if (scheme == SJF) {	//change for other type
		priqueue_init(jobQ, SJFcompare);
		return;
	}
	if (scheme == PRI) {	//change for other type
		priqueue_init(jobQ, PRIcompare);
		return;
	}
	if (scheme == RR) {	//change for other type
		priqueue_init(jobQ, RRcompare);
		return;
	}
	if (scheme == PPRI) {	//change for other type
		priqueue_init(jobQ, PRIcompare);
		return;
	}
	if (scheme == PSJF) {	//change for other type
		priqueue_init(jobQ, PSJFcompare);
		return;
	}
}

/**
 Called when a new job arrives.
 
 If multiple cores are idle, the job should be assigned to the core with the
 lowest id.
 If the job arriving should be scheduled to run during the next
 time cycle, return the zero-based index of the core the job should be
 scheduled on. If another job is already running on the core specified,
 this will preempt the currently running job.
 Assumptions:
 - You may assume that every job wil have a unique arrival time.

 @param job_number a globally unique identification number of the job arriving.
 @param time the current time of the simulator.
 @param running_time the total number of time units this job will run before it will be finished.
 @param priority the priority of the job. (The lower the value, the higher the priority.)
 @return index of core job should be scheduled on
 @return -1 if no scheduling changes should be made. 
 
 */
int scheduler_new_job(int job_number, int time, int running_time,
		int priority) {
	job_entry_t* curJob = joblistById;
	while (curJob->next)
		curJob = curJob->next;

	curJob->next = malloc(sizeof(job_entry_t));
	curJob = curJob->next;

	curJob->valid = 1;	//initialize this job
	curJob->id = job_number;
	curJob->arrivial = time;
	curJob->leave = -1;
	curJob->duration = running_time;
	curJob->remaining = running_time;
	curJob->respond = -1;
	curJob->priority = priority;
	curJob->next = NULL;
	curJob->lastTimeInCore = -1;

	if (schemeType == FCFS || schemeType == SJF || schemeType == PRI) {
		int i = 0;
		for (i = 0; i < coreNumber; i++) {	//look for idle core
			if (corelistById[i].currentJob == -1) {	//if we find an idle core, put it to the core
				corelistById[i].currentJob = job_number;
				curJob->respond = time;
				return i;
			}
		}
		priqueue_offer(jobQ, curJob);//if we don't find an idle core, put it to the Q
		return -1;
	}

	if (schemeType == RR) {
		int i = 0;
		for (i = 0; i < coreNumber; i++) {	//look for idle core
			if (corelistById[i].currentJob == -1) {	//if we find an idle core, put it to the core
				corelistById[i].currentJob = job_number;
				curJob->respond = time;
				curJob->lastTimeInCore = time;
				return i;
			}
		}
		priqueue_offer(jobQ, curJob);//if we don't find an idle core, put it to the Q
		return -1;
	}
	if (schemeType == PPRI) {
		//printf("Hey I get to here!\n");
		int i = 0;
		int worstCoreNumber = -1;
		int worstCorePri = -1;
		for (i = 0; i < coreNumber; i++) {//look for idle core, meanwhild go throught every core to find the worst job (if any)
			if (corelistById[i].currentJob == -1) {	//if we find an idle core, put it to the core
				corelistById[i].currentJob = job_number;
				corelistById[i].curJobPointer = curJob;
				//printf("curJob Id: %d", corelistById[i].curJobPointer->id);
				curJob->respond = time;
				curJob->lastTimeInCore = time;
				return i;
			}
			//if this core is not idle and if it's the first time finding a job
			int p = corelistById[i].curJobPointer->priority;
			//printf("Hey I'm here!\n");
			if (worstCoreNumber == -1) {//first time and new job's priority is higher
				if (priority < p) {
					worstCoreNumber = i;
					worstCorePri = p;
				}
			} else {	//if it's not the first time finding a core
				if (p > worstCorePri) {	//if it's even worse
					worstCoreNumber = i;
					worstCorePri = p;
				}
				if (p == worstCorePri) {	//if it's samely worse
					//if we have a later job, then swap it
					if (corelistById[i].curJobPointer->arrivial
							> corelistById[worstCoreNumber].curJobPointer->arrivial) {
						worstCoreNumber = i;
						worstCorePri = p;
					}
				}
			}
		}
		//printf("worstCoreNumber: %d, worstCorePri: %d\n",worstCoreNumber,worstCorePri);
		//if it gets to here, means there is no idle core
		//1.if there is a worst core
		if (worstCoreNumber != -1) {

			job_entry_t* coreJob = corelistById[worstCoreNumber].curJobPointer;
			//printf("If this is printed, then coreJob got problem!\n");
			//printf("core number: %d, jobID: %d, arrival: %d, leave: %d, duration: %d, remaining: %d, respond: %d\n",worstCoreNumber, coreJob->id,coreJob->arrivial,coreJob->leave,coreJob->duration,coreJob->remaining,coreJob->respond);
			coreJob->remaining = coreJob->remaining
					- (time - coreJob->lastTimeInCore);
			if (time == coreJob->respond) {
				coreJob->respond = -1;
			}
			corelistById[worstCoreNumber].currentJob = job_number;
			corelistById[worstCoreNumber].curJobPointer = curJob;
			curJob->lastTimeInCore = time;
			curJob->respond = time;
			priqueue_offer(jobQ, coreJob);
			return worstCoreNumber;
		}			//2.if there is not a worst core,then just put it to the Q
		priqueue_offer(jobQ, curJob);
		return -1;
	}

	if (schemeType == PSJF) {
		//printf("Hey I get to here!\n");
		int i = 0;
		int worstCoreNumber = -1;
		int worstCoreRemaining = -1;
		for (i = 0; i < coreNumber; i++) {//look for idle core, meanwhild go throught every core to find the worst job (if any)
			if (corelistById[i].currentJob == -1) {	//if we find an idle core, put it to the core
				corelistById[i].currentJob = job_number;
				corelistById[i].curJobPointer = curJob;
				//printf("curJob Id: %d", corelistById[i].curJobPointer->id);
				curJob->respond = time;
				curJob->lastTimeInCore = time;
				return i;
			}
			//if this core is not idle and if it's the first time finding a job
			int remain = corelistById[i].curJobPointer->remaining
					- (time - corelistById[i].curJobPointer->lastTimeInCore);
			//printf("Hey I'm here!\n");
			if (worstCoreNumber == -1) {//first time and new job's remaining (which is also running time) is shorter
				if (running_time < remain) {
					worstCoreNumber = i;
					worstCoreRemaining = remain;
				}
			} else {	//if it's not the first time finding a core
				if (remain > worstCoreRemaining) {	//if it's even worse
					worstCoreNumber = i;
					worstCoreRemaining = remain;
				}
				if (remain == worstCoreRemaining) {	//if it's samely worse
					//if we have a later job, then swap it
					if (corelistById[i].curJobPointer->arrivial
							> corelistById[worstCoreNumber].curJobPointer->arrivial) {
						worstCoreNumber = i;
						worstCoreRemaining = remain;
					}
				}
			}
		}
		//printf("worstCoreNumber: %d, worstCoreRemaining: %d\n",worstCoreNumber,worstCoreRemaining);
		//if it gets to here, means there is no idle core
		//1.if there is a worst core
		if (worstCoreNumber != -1) {
			job_entry_t* coreJob = corelistById[worstCoreNumber].curJobPointer;
			//printf("If this is printed, then coreJob got problem!\n");
			//printf("core number: %d, jobID: %d, arrival: %d, leave: %d, duration: %d, remaining: %d, respond: %d\n",worstCoreNumber, coreJob->id,coreJob->arrivial,coreJob->leave,coreJob->duration,coreJob->remaining,coreJob->respond);
			coreJob->remaining = coreJob->remaining
					- (time - coreJob->lastTimeInCore);
			if (time == coreJob->respond) {
				coreJob->respond = -1;
			}
			corelistById[worstCoreNumber].currentJob = job_number;
			corelistById[worstCoreNumber].curJobPointer = curJob;
			curJob->lastTimeInCore = time;
			curJob->respond = time;
			priqueue_offer(jobQ, coreJob);
			return worstCoreNumber;
		}			//2.if there is not a worst core,then just put it to the Q
		priqueue_offer(jobQ, curJob);
		return -1;
	}

	return -1;
}

/**
 Called when a job has completed execution.
 
 The core_id, job_number and time parameters are provided for convenience.
 You may be able to calculate the values with your own data structure.
 If any job should be scheduled to run on the core free'd up by the
 finished job, return the job_number of the job that should be scheduled to
 run on core core_id.
 
 @param core_id the zero-based index of the core where the job was located.
 @param job_number a globally unique identification number of the job.
 @param time the current time of the simulator.
 @return job_number of the job that should be scheduled to run on core core_id
 @return -1 if core should remain idle.
 */
int scheduler_job_finished(int core_id, int job_number, int time) {
	//update this job info
	job_entry_t* curJob = joblistById;
	while (curJob) {
		if (curJob->id == job_number) {
			curJob->leave = time;
			curJob->remaining = 0;
			break;
		}
		curJob = curJob->next;
	}

	//updating core info
	//printf("jobID: %d, arrival: %d, leave: %d, duration: %d, remaining: %d, respond: %d\n",curJob->id,curJob->arrivial,curJob->leave,curJob->duration,curJob->remaining,curJob->respond);
	corelistById[core_id].respond += ((curJob->respond) - (curJob->arrivial));
	corelistById[core_id].turnaround += (curJob->leave - curJob->arrivial);
	corelistById[core_id].waiting += (curJob->leave - curJob->arrivial
			- curJob->duration);
	corelistById[core_id].jobCounter++;
	//printf("total: respond: %d, turnaround: %d, waiting: %d, jobCounter: %d\n",corelistById[core_id].respond,corelistById[core_id].turnaround,corelistById[core_id].waiting,corelistById[core_id].jobCounter);
	corelistById[core_id].currentJob = -1;	//no job is running now

	//cases
	if (schemeType == FCFS || schemeType == SJF || schemeType == PRI) {
		job_entry_t* temp = priqueue_poll(jobQ);
		if (temp) {	//if it returns the head, then run the head and return the job number
			temp->respond = time;
			corelistById[core_id].currentJob = temp->id;
			return (temp->id);
		}
		return -1;
	}
	if (schemeType == RR || schemeType == PPRI || schemeType == PSJF) {
		job_entry_t* temp = priqueue_poll(jobQ);
		if (temp) {	//if it returns the head, then run the head and return the job number
			if (temp->respond == -1) {//if it's the first time it enter a core, set the respond time
				temp->respond = time;
			}
			temp->lastTimeInCore = time;
			corelistById[core_id].currentJob = temp->id;
			corelistById[core_id].curJobPointer = temp;
			return (temp->id);
		}
		return -1;
	}

	return -1;

}

/**
 When the scheme is set to RR, called when the quantum timer has expired
 on a core.
 
 If any job should be scheduled to run on the core free'd up by
 the quantum expiration, return the job_number of the job that should be
 scheduled to run on core core_id.

 @param core_id the zero-based index of the core where the quantum has expired.
 @param time the current time of the simulator. 
 @return job_number of the job that should be scheduled on core cord_id
 @return -1 if core should remain idle
 */
int scheduler_quantum_expired(int core_id, int time) {
	job_entry_t* curJob = joblistById;
	while (curJob) {
		if (curJob->id == (corelistById[core_id].currentJob)) {	//Find The Job! Yeh
			curJob->remaining = curJob->remaining - curJob->lastTimeInCore;
			priqueue_offer(jobQ, curJob);	//put it to the end of the Q
			corelistById[core_id].currentJob = -1;//no job is running on this core
			break;
		}
		curJob = curJob->next;
	}

	job_entry_t* temp = priqueue_poll(jobQ);	//find the head job in Q
	if (temp) {	//if it returns the head, then run the head and return the job number
		if (temp->respond == -1) {//if it's the first time it enter a core, set the respond time
			temp->respond = time;
		}
		temp->lastTimeInCore = time;
		corelistById[core_id].currentJob = temp->id;
		return (temp->id);
	}

	return -1;
}

/**
 Returns the average waiting time of all jobs scheduled by your scheduler.

 Assumptions:
 - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
 @return the average waiting time of all jobs scheduled.
 */
float scheduler_average_waiting_time() {
	float totalJobs = 0;
	float totalWaiting = 0;
	int i = 0;
	for (i = 0; i < coreNumber; i++) {
		totalJobs += corelistById[i].jobCounter;
		totalWaiting += corelistById[i].waiting;
	}
	float ret = totalWaiting / totalJobs;
	return ret;
}

/**
 Returns the average turnaround time of all jobs scheduled by your scheduler.

 Assumptions:
 - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
 @return the average turnaround time of all jobs scheduled.
 */
float scheduler_average_turnaround_time() {
	float totalJobs = 0;
	float totalTurnaround = 0;
	int i = 0;
	for (i = 0; i < coreNumber; i++) {
		totalJobs += corelistById[i].jobCounter;
		totalTurnaround += corelistById[i].turnaround;
	}
	float ret = totalTurnaround / totalJobs;
	return ret;
}

/**
 Returns the average response time of all jobs scheduled by your scheduler.

 Assumptions:
 - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
 @return the average response time of all jobs scheduled.
 */
float scheduler_average_response_time() {
	float totalJobs = 0;
	float totalrespond = 0;
	int i = 0;
	for (i = 0; i < coreNumber; i++) {
		totalJobs += corelistById[i].jobCounter;
		totalrespond += corelistById[i].respond;
	}
	float ret = totalrespond / totalJobs;
	return ret;
}

/**
 Free any memory associated with your scheduler.
 
 Assumptions:
 - This function will be the last function called in your library.
 */
void scheduler_clean_up() {
	priqueue_destroy(jobQ);
	free(jobQ);
	free(corelistById);
	job_entry_t* temp = joblistById;
	job_entry_t* tempprev;
	while (temp) {
		tempprev = temp;
		temp = temp->next;
		free(tempprev);
	}
}

/**
 This function may print out any debugging information you choose. This
 function will be called by the simulator after every call the simulator
 makes to your scheduler.

 In our provided output, we have implemented this function to list the jobs in the order they are to be scheduled.
 Furthermore, we have also listed the current state of the job (either running on a given core or idle).
 For example, if we have a non-preemptive algorithm and job(id=4) has began running,
 job(id=2) arrives with a higher priority, and job(id=1) arrives with a lower priority,
 the output in our sample output will be:

 2(-1) 4(0) 1(-1)  
 4(0)
 This function is not required and will not be graded. You may leave it
 blank if you do not find it useful.
 */
void scheduler_show_queue() {
	//
	int i = 0;
	for (i = 0; i < coreNumber; i++) {
		if (corelistById[i].currentJob >= 0) {
			printf("%d(%d) ", corelistById[i].currentJob, i);
		}
	}
	priqueue_entry_t *q = jobQ->headMin;
	while (q && q->data) {
		printf("%d(-1) ", ((job_entry_t*) (q->data))->id);
		q = q->next;
	}
}
