#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOTAL_ALLOCS 6000
#define ALLOC_SIZE 1024*1024

int main()
{
	malloc(1);
	int i;
	void *ptr = NULL;
	int testcounter=0;
	for (i = 0; i < TOTAL_ALLOCS; i++)
	{
		ptr = malloc(ALLOC_SIZE);
		if (ptr == NULL)
		{
			return 1;
		}
    memset(ptr, 0xab, ALLOC_SIZE);

		free(ptr);
	}

	printf("Memory was allocated and freed!\n");	
	return 0;
}
