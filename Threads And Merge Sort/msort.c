/** @file msort.c */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
struct qsort_thread_info {
	int* start_p;
	int size;
};
struct mergesort_thread_info {
	int f;
	int* start_p1;
	int* start_p2;

	int* output;

	int size1;
	int size2;

};

void readFile(int *lnCount, int** inputArray) {

	char* line = NULL;
	size_t line_size = 0;
	(*lnCount) = 0;
	(*inputArray) = (int*) malloc(sizeof(int) * 1024);
	int curSize = 1024;
	int line_ct = 0;

	while (1) {
		line_ct = getline(&line, &line_size, stdin);
		if (line_ct <= 0)
			break;
		if (curSize <= (*lnCount)) {
			(*inputArray) = (int*) realloc(*inputArray,
					sizeof(int) * 2 * curSize);
			curSize *= 2;
		}
		(*inputArray)[(*lnCount)] = atoi(line);
		(*lnCount)++;
	}
	free(line);
}

int compare(const void * a, const void * b) //This is for qsort
		{
	return (*(int*) a - *(int*) b);
}

void *qsortT(void *ptr) {
	struct qsort_thread_info *tinfo = ptr;
	//printf("starting point: %d, size: %d\n",*(tinfo->start_p),tinfo->size);
	qsort(tinfo->start_p, tinfo->size, sizeof(int), compare);
	fprintf(stderr, "Sorted %d elements.\n", tinfo->size);
	return NULL;
}

void *mergeSortT(void *ptr) {
	struct mergesort_thread_info *tinfo = ptr;
	//printf("starting point: %d and %d, size: %d and %d\n",*(tinfo->start_p1),*(tinfo->start_p2),tinfo->size1,tinfo->size2);
	int* p1 = tinfo->start_p1, *p2 = tinfo->start_p2, *out = tinfo->output;
	int size1 = tinfo->size1, size2 = tinfo->size2;
	//printf("%d Begin: p1:%d p2: %d, size1: %d, size2: %d\n",tinfo->f,*p1,*p2,size1,size2);
	int s1 = 0, s2 = 0;
	int i, dupes = 0;

	//printf("Hey!!\n");
	for (;;) {
		//if(tinfo->f==2)
		//printf("p1:%d p2: %d, s1: %d, s2: %d\n",*p1,*p2,s1,s2);
		if ((*p1) < (*p2)) {
			//printf("1\n");
			*out = *p1;
			p1++;
			out++;
			s1++;
			//printf("2now p1: %d\n",*p1);
		} else {
			if ((*p1) > (*p2)) {
				//printf("2\n");
				//printf("3now p1: %d\n",*p1);
				*out = *p2;
				p2++;
				out++;
				s2++;
				//printf("1now p1: %d\n",*p1);
			} else {				//equal
									//printf("3\n");
				*out = *p1;
				p1++;
				out++;
				s1++;
				*out = *p2;
				p2++;
				out++;
				s2++;
				dupes++;
			}
		}

		if (s1 >= size1) {
			if (s2 >= size2) {
				fprintf(stderr,
						"Merged %d and %d elements with %d duplicates.\n", s1,
						s2, dupes);
				return NULL;				//reach the end
			} else {
				for (i = s2; i < size2; i++) {
					*out = *p2;
					p2++;
					out++;
					s2++;
				}
				fprintf(stderr,
						"Merged %d and %d elements with %d duplicates.\n", s1,
						s2, dupes);
				return NULL;
			}
		}
		if (s2 >= size2) {
			for (i = s1; i < size1; i++) {
				*out = *p1;
				p1++;
				out++;
				s1++;
			}
			fprintf(stderr, "Merged %d and %d elements with %d duplicates.\n",
					s1, s2, dupes);
			return NULL;
		}

	}
	//printf("Hey!!!\n");

	return NULL;
}

int main(int argc, char **argv) {
	int *inputArray;
	int input_ct;				//num of elements
	int segment_count = atoi(argv[1]);//the input is divided into segment_cout parts

	readFile(&input_ct, &inputArray);

	int values_per_segment; /**< Maximum number of values per segment. */
	int values_last_segment;

	if (input_ct % segment_count == 0) {
		values_per_segment = input_ct / segment_count;
		values_last_segment = values_per_segment;
	} else {
		values_per_segment = (input_ct / segment_count) + 1;
		values_last_segment = input_ct
				- values_per_segment * (segment_count - 1);
		//printf("last number: %d\n",values_last_segment);
	}

	//now begins qsort part
	int** p2Segs = (int**) malloc(sizeof(int*) * segment_count);
	int i = 0;
	int* cur = inputArray;
	for (i = 0; i < segment_count; i++) {
		p2Segs[i] = cur;
		cur += values_per_segment;
	}
	/*for(i=0;i<segment_count;i++){
	 printf("%d, ",*p2Segs[i]);
	 }*/

	pthread_t *threads = malloc(sizeof(pthread_t) * segment_count);
	struct qsort_thread_info *qinfo = malloc(
			segment_count * sizeof(struct qsort_thread_info));
	for (i = 0; i < (segment_count - 1); i++) {
		qinfo[i].size = values_per_segment;
		qinfo[i].start_p = p2Segs[i];
		pthread_create(&threads[i], NULL, qsortT, &qinfo[i]);
	}
	qinfo[i].size = values_last_segment;
	qinfo[i].start_p = p2Segs[i];
	pthread_create(&threads[i], NULL, qsortT, &qinfo[i]);

	for (i = 0; i < segment_count; i++) {
		pthread_join(threads[i], NULL);
		//printf("Waited for thread %d\n",i);
	}

	int j = 0;
	/*for(i=0;i<segment_count;i++){
	 if(i==segment_count-1){
	 printf("%d: ",i);
	 for(j=0;j<values_last_segment;j++){
	 printf(" %i",p2Segs[i][j]);
	 }
	 printf("\n");
	 break;
	 }else{
	 printf("%d: ",i);
	 for(j=0;j<values_per_segment;j++){
	 printf(" %i",p2Segs[i][j]);
	 }
	 printf("\n");
	 }
	 }*/

	//now begins merge sort
	int* output = malloc(sizeof(int) * input_ct);
	int** p2Segs2 = NULL;
	struct mergesort_thread_info *minfo = NULL;

	while (1) {
		if (segment_count == 1) {
			break;
		}
		p2Segs2 = (int**) realloc(p2Segs2,
				sizeof(int*) * (segment_count / 2 + 1));
		minfo = realloc(minfo,
				(segment_count / 2) * sizeof(struct mergesort_thread_info));
		threads = realloc(threads, sizeof(pthread_t) * (segment_count / 2));
		int out = 0;

		for (i = 0; i < (segment_count - 1); i += 2) {
			minfo[i / 2].f = (i / 2);
			p2Segs2[i / 2] = &(output[out]);
			minfo[i / 2].size1 = values_per_segment;
			if ((i + 2) == segment_count) {
				minfo[i / 2].size2 = values_last_segment;
				//printf("last seg size: %d\n",values_last_segment);
			} else {
				minfo[i / 2].size2 = values_per_segment;
			}
			minfo[i / 2].start_p1 = p2Segs[i];
			minfo[i / 2].start_p2 = p2Segs[i + 1];
			minfo[i / 2].output = p2Segs2[i / 2];
			out += (minfo[i / 2].size1 + minfo[i / 2].size2);
			//printf("i:%d\n",i);
			pthread_create(&threads[i / 2], NULL, mergeSortT, &minfo[i / 2]);
		}
		p2Segs2[i / 2] = &(output[out]);

		for (i = 0; i < (segment_count - 1); i += 2) {
			pthread_join(threads[i / 2], NULL);
			//printf("Waited for thread %d\n",i);
		}

		values_per_segment *= 2;
		if (segment_count % 2 == 1) {
			//printf("After sorting we are at: %d\n",out);
			for (i = out; i < input_ct; i++) {
				output[i] = inputArray[i];
				//printf("output[%d]: %d\n",i, output[i]);
			}

		} else {
			values_last_segment += (values_per_segment / 2);
		}
		//printf("11las seg value: %d\n",values_last_segment);

		segment_count =
				(segment_count % 2 == 1) ?
						(segment_count / 2 + 1) : (segment_count / 2);

		int** temp;
		temp = p2Segs2;
		p2Segs2 = p2Segs;
		p2Segs = temp;

		int* tempp;
		tempp = inputArray;
		inputArray = output;
		output = tempp;

		/*for(i=0;i<segment_count;i++){
		 if(i==segment_count-1){
		 printf("%d: ",i);
		 //printf("las seg value: %d\n",values_last_segment);
		 for(j=0;j<values_last_segment;j++){
		 printf(" %i",p2Segs[i][j]);
		 }
		 printf("\n");
		 break;
		 }else{
		 printf("%d: ",i);
		 for(j=0;j<values_per_segment;j++){
		 printf(" %i",p2Segs[i][j]);
		 }
		 printf("\n");
		 }
		 }*/
	}

	for (i = 0; i < input_ct; i++) {
		printf("%i\n", inputArray[i]);
	}

	/*for(;;){//for every iteration

	 }*/

	free(p2Segs);
	free(threads);
	free(qinfo);
	free(minfo);
	free(inputArray);
	free(output);
	free(p2Segs2);

	/*int j=0;
	 for(i=0;i<segment_count;i++){
	 if(i==segment_count-1){
	 printf("%d: ",i);
	 for(j=0;j<values_last_segment;j++){
	 printf(" %i",p2Segs[i][j]);
	 }
	 printf("\n");
	 break;
	 }else{
	 printf("%d: ",i);
	 for(j=0;j<values_per_segment;j++){
	 printf(" %i",p2Segs[i][j]);
	 }
	 printf("\n");
	 }
	 }*/

	/*int i=0;
	 for(i=0;i<lnCount;i++){
	 printf("%i ",inputArray[i]);
	 }
	 printf("\n%i",lnCount);*/

	return 0;
}
