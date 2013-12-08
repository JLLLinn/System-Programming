/** @file alloc.c */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
/** Structure for each dicitonary entry. */
typedef struct _dictionary_entry_t {
	size_t size;
	//int free;
	struct _dictionary_entry_t *next; //this is completed
	char ptr[];
} dictionary_entry_t;

//static int firstinit=1;
dictionary_entry_t *dictionary = NULL;
size_t METADATAS = sizeof(dictionary_entry_t);
//static int fCounter =0;

/**
 * Allocate space for array in memory
 *
 * Allocates a block of memory for an array of num elements, each of them size
 * bytes long, and initializes all its bits to zero. The effective result is
 * the allocation of an zero-initialized memory block of (num * size) bytes.
 *
 * @param num
 *    Number of elements to be allocated.
 * @param size
 *    Size of elements.
 *
 * @return
 *    A pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory, a
 *    NULL pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/calloc/
 */
void *calloc(size_t num, size_t size) {
	/* Note: This function is complete. You do not need to modify it. */
	void *ptr = malloc(num * size);

	if (ptr)
		memset(ptr, 0x00, num * size);

	return ptr;
}

/**
 * Allocate memory block
 *
 * Allocates a block of size bytes of memory, returning a pointer to the
 * beginning of the block.  The content of the newly allocated block of
 * memory is not initialized, remaining with indeterminate values.
 *
 * @param size
 *    Size of the memory block, in bytes.
 *
 * @return
 *    On success, a pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a null pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/malloc/
 */
void *malloc(size_t size) {
	//int freeCounter=fCounter;
	//1.check to see if we can reuse one of the previous allocated spaces
	dictionary_entry_t **p = &dictionary;
	dictionary_entry_t **chosen = NULL;

	while ((*p) != NULL) {
		if ((*p)->size >= size) {
			//find the first match (if is it the first one that I found || if it's not the first, but it's got a small(better)size)
			if (chosen == NULL || ((*chosen) && (*p)->size < (*chosen)->size)) {
				chosen = p;
			}
		}
		p = &(*p)->next;
	}

	if (chosen) {
		void *ptr = (*chosen)->ptr;
		*chosen = (*chosen)->next;
		return ptr;
	}
dictionary_entry_t *new = sbrk(METADATAS+size);
new->size = size;
new -> next = NULL;

return new->ptr;

}

/**
 * Deallocate space in memory
 *
 * A block of memory previously allocated using a call to malloc(),
 * calloc() or realloc() is deallocated, making it available again for
 * further allocations.
 *
 * Notice that this function leaves the value of ptr unchanged, hence
 * it still points to the same (now invalid) location, and not to the
 * null pointer.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(),
 *    calloc() or realloc() to be deallocated.  If a null pointer is
 *    passed as argument, no action occurs.
 */
void free(void *ptr) {
// "If a null pointer is passed as argument, no action occurs."
if (!ptr)
	return;

dictionary_entry_t * head = (((void*) ptr) - METADATAS);
head->next = dictionary;
dictionary = head;

return;
}

/**
 * Reallocate memory block
 *
 * The size of the memory block pointed to by the ptr parameter is changed
 * to the size bytes, expanding or reducing the amount of memory available
 * in the block.
 *
 * The function may move the memory block to a new location, in which case
 * the new location is returned. The content of the memory block is preserved
 * up to the lesser of the new and old sizes, even if the block is moved. If
 * the new size is larger, the value of the newly allocated portion is
 * indeterminate.
 *
 * In case that ptr is NULL, the function behaves exactly as malloc, assigning
 * a new block of size bytes and returning a pointer to the beginning of it.
 *
 * In case that the size is 0, the memory previously allocated in ptr is
 * deallocated as if a call to free was made, and a NULL pointer is returned.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(), calloc()
 *    or realloc() to be reallocated.
 *
 *    If this is NULL, a new block is allocated and a pointer to it is
 *    returned by the function.
 *
 * @param size
 *    New size for the memory block, in bytes.
 *
 *    If it is 0 and ptr points to an existing block of memory, the memory
 *    block pointed by ptr is deallocated and a NULL pointer is returned.
 *
 * @return
 *    A pointer to the reallocated memory block, which may be either the
 *    same as the ptr argument or a new location.
 *
 *    The type of this pointer is void*, which can be cast to the desired
 *    type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a NULL pointer is returned, and the memory block pointed to by
 *    argument ptr is left unchanged.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/realloc/
 */
void *realloc(void *ptr, size_t size) {
//printf("%dNow begin reallocing pointer: %p and size: %d\n",testcounter,ptr,size);
if (!ptr) {
	return malloc(size);
}
if (!size) {
	free(ptr);
	return NULL;
}
dictionary_entry_t *p = (dictionary_entry_t*) (ptr - METADATAS);

void *return_ptr = malloc(size);
size_t old_size = p->size;
memcpy(return_ptr, ptr, (size > old_size ? old_size : size));
free(ptr);
return return_ptr;
}
