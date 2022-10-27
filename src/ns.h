#ifndef __NS_H__
#define __NS_H__

#include "scopetypes.h"

// Check if switching namespace is required
bool nsIsPidInChildNs(pid_t, pid_t *);
bool nsIsPidInSameMntNs(pid_t);

// Operation performed from host to container
int nsForkAndExec(pid_t, pid_t, char);
int nsConfigure(pid_t, void *, size_t);
service_status_t nsService(pid_t, const char *);

// Operation performed from container to host
int nsHostStart(void);

uid_t nsGetRootUid(pid_t);
gid_t nsGetRootGid(pid_t);

#endif // __NS_H__
