#include <stdio.h>
#include <stdlib.h>

#define K 1024
#define M (1024 * 1024)
#define G (1024 * 1024 * 1024)

#ifdef PART2
  #define START_MALLOC_SIZE (1536 * M)
  #define STOP_MALLOC_SIZE  (1 * K)
#else
  #define START_MALLOC_SIZE (1 * G)
  #define STOP_MALLOC_SIZE  (1 * K)
#endif
//int testcounter=0;
void *reduce(void *ptr, int size)
{
	//printf("%d\n",testcounter);
	if (size > STOP_MALLOC_SIZE)
	{
		//printf("now begin realloc %p to size: %d\n",ptr,size/2);
		void *ptr1 = realloc(ptr, size / 2);
		//printf("now begin alloc size:%d\n",size/2);
		void *ptr2 = malloc(size / 2);
		//printf("hey i get to here2\n");
		if (ptr1 == NULL || ptr2 == NULL)
		{
			printf("Memory failed to allocate!\n");
			exit(1);
		}
		//printf("hey i get to here3\n");
		ptr1 = reduce(ptr1, size / 2);
		ptr2 = reduce(ptr2, size / 2);

		if (*((int *)ptr1) != size / 2 || *((int *)ptr2) != size / 2)
		{
			printf("Memory failed to contain correct data after many allocations!\n");
			exit(2);
		}
		//printf("hey i get to here4\n");
		//printf("ln40 now begin free ptr2 %p\n",ptr2);
		free(ptr2);
		//printf("ln42 now begin realloc ptr1 %p to size: %d\n",ptr1,size);
		ptr1 = realloc(ptr1, size);
		//printf("hey i get to here5\n");
		if (*((int *)ptr1) != size / 2)
		{
			printf("Memory failed to contain correct data after realloc()!\n");
			exit(3);
		}
		//printf("hey i get to here6\n");
		*((int *)ptr1) = size;
		return ptr1;
	}
	
	else
	{
		*((int *)ptr) = size;
		return ptr;
	}


}


int main()
{
	//printf("now begin allocating size :1\n");
	malloc(1);
	//printf("allocating size 1 success\n");
	int size = START_MALLOC_SIZE;
	//printf("Hey here\n");
	while (size > STOP_MALLOC_SIZE)
	{
		//printf("size is :%d",size);
		//printf("now begin allocating size :%d\n",size);
		void *ptr = malloc(size);
		
		ptr = reduce(ptr, size / 2);
		//printf("2size is :%d",size);
		free(ptr);
		
		size /= 2;
		//printf("size: %d\n",size);
	}

	printf("Memory was allocated, used, and freed!\n");	
	return 0;
}
