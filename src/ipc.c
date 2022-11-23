#define _GNU_SOURCE

#include "ipc.h"

/* Message-queue
 *
 * Message-queue system limits which are related to `ipcOpenConnection`
 * are defined in following files:
 * 
 * "/proc/sys/fs/mqueue/msg_max"
 * - describes maximum number of messsages in a queue (QUEUE_MSG_MAX)
 *
 * "/proc/sys/fs/mqueue/msgsize_max"
 * - describes maximum message size in a queue (QUEUE_MSG_SIZE)
 * 
 * "/proc/sys/fs/mqueue/queues_max"
 * - describes system-wide limit on the number of message queues that can be created
 * 
 * See details in: https://man7.org/linux/man-pages/man7/mq_overview.7.html
 */

#define QUEUE_MSG_MAX  10
#define QUEUE_MSG_SIZE 8192

int
ipcSend(mqd_t mqdes, const char *data, size_t len) {
    return scope_mq_send(mqdes, data, len, 0);
}

int
ipcSendWithTimeout(mqd_t mqdes, const char *data, size_t len, const struct timespec *req) {
    return scope_mq_timedsend(mqdes, data, len, 0, req);
}

ssize_t
ipcRecv(mqd_t mqdes, char *buf, size_t len) {
    return scope_mq_receive(mqdes, buf, len, 0);
}

ssize_t
ipcRecvWithTimeout(mqd_t mqdes, char *buf, size_t len, const struct timespec *req) {
    return scope_mq_timedreceive(mqdes, buf, len, 0, req);
}

mqd_t
ipcCreateNonBlockReadConnection(const char *name) {
    struct mq_attr attr = {.mq_flags = 0, 
                               .mq_maxmsg = QUEUE_MSG_MAX,
                               .mq_msgsize = QUEUE_MSG_SIZE,
                               .mq_curmsgs = 0};
    return scope_mq_open(name, O_RDONLY | O_CREAT | O_CLOEXEC | O_NONBLOCK, 0666, &attr);     
}

mqd_t
ipcOpenWriteConnection(const char *name) {
    return scope_mq_open(name, O_WRONLY | O_NONBLOCK);
}

int
ipcCloseConnection(mqd_t mqdes) {
    return scope_mq_close(mqdes);
}

int
ipcDestroyConnection(const char *name) {
    return scope_mq_unlink(name); 
}

int
ipcGetInfo(mqd_t mqdes, long *mqFlags, long *mqMaxMsg, long *mqMsgSize, long *mqCurMsgs) {
    struct mq_attr attr;
    int res = scope_mq_getattr(mqdes, &attr);
    *mqFlags = attr.mq_flags;
    *mqMaxMsg = attr.mq_maxmsg;
    *mqMsgSize = attr.mq_msgsize;
    *mqCurMsgs = attr.mq_curmsgs;
    return res;
}

bool
ipcIsActive(mqd_t mdes) {
    return mqdes != (mqd_t) -1;
}