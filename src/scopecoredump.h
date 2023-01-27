#ifndef __SCOPECOREDUMP_H__
#define __SCOPECOREDUMP_H__

#include "scopetypes.h"

bool scopeCoreDumpGenerate(pid_t);
bool scopeUnwindCoreDumpGenerate(void);

#endif // __SCOPECOREDUMP_H__
