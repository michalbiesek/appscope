#include <stdlib.h>

extern void* mi_realloc(void *, size_t) __attribute__((__weak__));

void *realloc(void *p, size_t n)
{
	if (mi_realloc)
		return mi_realloc(p, n);
	return __libc_realloc(p, n);
}
