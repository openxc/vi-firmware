#include <stdlib.h>
#include "mem_tracking.h"
#include <util/debug_connector.h>

#undef malloc
#undef free

unsigned long totalBytes = 0;
unsigned long totalMallocs = 0;

void* custom_malloc(size_t sz, const char* file, int line) {
	//c_debug("%s: %d: Using custom_malloc", file, line);
#if 1
	size_t* ptr = (size_t*) malloc(sizeof(size_t) + sz); // Storing size of malloc in first size_t bytes
	
	if (!ptr) {
		c_debug("%s: %d: Mallocing %d bytes failed", file, line, sz);
		return 0;
	}
	
	c_debug("%s: %d: Successfully malloc'd %d bytes: %x", file, line, sz, ptr);
	*ptr = sz;
	totalBytes += sz;
	totalMallocs++;

	return (void*) (ptr + 1);
#else
	void* ptr = malloc(sz);
	
	if (!ptr)
		c_debug("%s: %d: Mallocing %d bytes failed", file, line, sz);
	else
		c_debug("%s: %d: Successfully malloc'd %d bytes: %x", file, line, sz, ptr);

	return ptr;
#endif
}

void custom_free(void* ptr, const char* file, int line) {
	//c_debug("%s: %d: Using custom_free", file, line);
#if 1
	size_t* sz_ptr = ((size_t*) ptr) - 1;
	size_t sz = *sz_ptr;
	totalBytes -= sz;
	totalMallocs--;
	free(sz_ptr);
	c_debug("%s: %d: Successfully freed %d bytes: %x", file, line, sz, sz_ptr);
#else
	free(ptr);
	c_debug("%s: %d: Successfully freed: %x", file, line, ptr);
#endif
}