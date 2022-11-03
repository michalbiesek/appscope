#ifndef __NSINFO_H__
#define __NSINFO_H__

#include "scopestdlib.h"
#include "scopetypes.h"

// Retrieve namespace information

bool nsInfoIsPidGotSecondPidNs(pid_t, pid_t *);
bool nsInfoIsPidInSameMntNs(pid_t);
uid_t nsInfoTranslateUid(pid_t);
gid_t nsInfoTranslateGid(pid_t);

// Translate uid/gid before and restore after
int nsInfoShmOpen(const char *, int, mode_t , uid_t, gid_t, uid_t, gid_t);
int nsInfoOpen(const char *, int, uid_t, gid_t, uid_t, gid_t);
int nsInfoOpenWithMode(const char *, int, mode_t, uid_t, gid_t, uid_t, gid_t);
int nsInfoMkdir(const char *, mode_t, uid_t, gid_t, uid_t, gid_t);


#endif // __NSINFO_H__
