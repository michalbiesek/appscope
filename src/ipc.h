#ifndef __IPC_H__
#define __IPC_H__

#include "scopestdlib.h"
#include "scopetypes.h"

// Send message from Inter-process communication
int ipcSend(mqd_t, const char *, size_t);
int ipcSendWithTimeout(mqd_t, const char *, size_t, const struct timespec *);

// Receive message from Inter-process communication
ssize_t ipcRecv(mqd_t, char *, size_t);
ssize_t ipcRecvWithTimeout(mqd_t, char *, size_t, const struct timespec *);

// Manage Inter-process communication
mqd_t ipcCreateNonBlockReadConnection(const char *);
mqd_t ipcOpenWriteConnection(const char *);
int ipcCloseConnection(mqd_t);
int ipcDestroyConnection(const char *);
int ipcGetInfo(mqd_t, long *, long *, long *, long *);
bool ipcIsActive(mqd_t);


#endif // __IPC_H__
