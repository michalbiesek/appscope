#include <stdlib.h>
#include <stddef.h>
#include <netdb.h>
#include "lookup.h"

FREEADDR_LOCK_OBJ_DEF;

void freeaddrinfo(struct addrinfo *p)
{
	size_t cnt;
	for (cnt=1; p->ai_next; cnt++, p=p->ai_next);
	struct aibuf *b = (void *)((char *)p - offsetof(struct aibuf, ai));
	b -= b->slot;
	freeaddrinfo_lock();
	if (!(b->ref -= cnt)) free(b);
	else freeaddrinfo_unlock();
}

void freeaddrinfo_fork_op(int who)
{
	if (who<0) freeaddrinfo_lock();
	else if (who>0) freeaddrinfo_resetlock();
	else freeaddrinfo_unlock();
}
