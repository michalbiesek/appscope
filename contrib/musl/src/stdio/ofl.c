#include "stdio_impl.h"
#include "lock.h"
#include "fork_impl.h"

static FILE *ofl_head;
static volatile int ofl_lock[1];
volatile int *const __stdio_ofl_lockptr = ofl_lock;

FILE **__ofl_lock()
{
	LOCK(ofl_lock);
	return &ofl_head;
}

void __ofl_unlock()
{
	UNLOCK(ofl_lock);
}

void fork_ofl_lock_op(int who) {
	if (who<0) LOCK(ofl_lock);
	else UNLOCK(ofl_lock);
}