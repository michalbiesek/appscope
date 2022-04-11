#include <stdlib.h>

extern void* mi_free(void *) __attribute__((__weak__));

void free(void *p)
{	if (mi_free)
		mi_free(p);
	else 
		__libc_free(p);
}
