#ifndef __NSINFO_H__
#define __NSINFO_H__

#include "scopetypes.h"

// Check if switching namespace is required
bool nsInfoIsPidInChildNs(pid_t, pid_t *);
bool nsInfoIsPidInSameMntNs(pid_t);
uid_t nsInfoGetRootUid(pid_t);
gid_t nsInfoGetRootGid(pid_t);

#endif // __NSINFO_H__
