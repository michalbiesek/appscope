#define _GNU_SOURCE

#include "ipc.h"
#include "runtimecfg.h"
#include "cJSON.h"

/* Inter-process communication module based on the message-queue
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

#define QUEUE_MSG_MAX  4
#define QUEUE_MSG_SIZE 256

#define INPUT_CMD_LEN(x) (sizeof(x)-1)
#define CMD_TABLE_SIZE(x) (sizeof(x)/sizeof(x[0]))

/* Output message function definition
 * Fills up the input buffer return the size of message
 */
typedef size_t (*out_msg_func_t)(char *, size_t);


struct ipcRequest {
  int ins;
//   short reqId;
//   short currentMsg;
//   short totalMsg;
  int param;
  char data[];
};

struct ipcResponse {
//   short reqId;
  int status;
//   short currentMsg;
//   short totalMsg;
  int param;
  char data[];
};

static size_t
cmdGetScopeStatus(char *buf, size_t len) {
    // Excluding the terminating null byte
    unsigned short status = (g_cfg.funcs_attached) ? 1 : 0;
    return scope_snprintf(buf, len, "%hu", status);
}

static size_t
cmdUnknown(char *buf, size_t len) {
    // Excluding the terminating null byte
    return scope_snprintf(buf, len, "Unknown");
}

typedef struct {
    ipc_cmd_t cmd;          // command instruction [in]
    out_msg_func_t func;    // output func [out]
} output_cmd_table_t;

output_cmd_table_t outputCmdTable[] = {
    {IPC_CMD_GET_SCOPE_STATUS,  cmdGetScopeStatus},
    {IPC_CMD_UNKNOWN,           cmdUnknown}
};

static int
ipcSend(mqd_t mqdes, const char *data, size_t len) {
    return scope_mq_send(mqdes, data, len, 0);
}

static ssize_t
ipcRecv(mqd_t mqdes, char *buf, size_t len) {
    return scope_mq_receive(mqdes, buf, len, 0);
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

long
ipcInfoMsgCount(mqd_t mqdes) {
    struct mq_attr attr;
    int res = scope_mq_getattr(mqdes, &attr);
    if (res == 0) {
        return attr.mq_curmsgs;
    }
    return -1;
}

bool
ipcIsActive(mqd_t mqdes, size_t *msgSize) {
    struct mq_attr attr;
    if (mqdes == (mqd_t) -1) {
        return FALSE;
    }
    int res = scope_mq_getattr(mqdes, &attr);
    if (res == 0) {
        *msgSize = attr.mq_msgsize;
        return TRUE;
    }
    return FALSE;
}

bool
ipcRequestMsgHandler(mqd_t mqDes, size_t mqSize, ipc_cmd_t *cmdRes) {
    char *buf = scope_malloc(mqSize);
    if (!buf) {
        return FALSE;
    }

    ssize_t recvLen = ipcRecv(mqDes, buf, mqSize);
    if (recvLen == -1) {
        scope_free(buf);
        return FALSE;
    }

    // Minimum request size to support metadata
    if (recvLen <= offsetof(struct ipcRequest, data)) {
        scope_free(buf);
        return FALSE;
    }

    struct ipcRequest *req = (struct ipcRequest *) buf;

    // Find the proper request
    for (int i = 0; i < CMD_TABLE_SIZE(outputCmdTable); ++i) {
        if (req->ins == outputCmdTable[i].cmd) {
            *cmdRes = outputCmdTable[i].cmd;
            scope_free(buf);
            return TRUE;
        }
    }

    // Handle data TODO
    *cmdRes = IPC_CMD_UNKNOWN;
    scope_free(buf);
    return TRUE;
}

bool
ipcResponseMsgHandler(mqd_t mqDes, size_t mqSize, ipc_cmd_t cmd) {
    bool res = FALSE;
    size_t len;

    if ((unsigned)(cmd) > IPC_CMD_UNKNOWN) {
        // Default Response ??
        return res;
    }

    // Allocate message from size readed from fd
    char *buf = scope_malloc(mqSize);
    if (!buf) {
        return res;
    }

    // Handle response
    for (int i = 0; i < CMD_TABLE_SIZE(outputCmdTable); ++i) {
        if (cmd == outputCmdTable[i].cmd) {
            len = outputCmdTable[i].func(buf, mqSize);
            break;
        }
    }

    // TODO error handling
    cJSON *response = cJSON_CreateObject();
    cJSON_AddNumberToObjLN(response, "status", 200);
    cJSON_AddNumberToObjLN(response, "data", )

    int sendRes = ipcSend(mqDes, (const char *) &response, respLen);
    if (sendRes != -1) {
        res = TRUE;
    }

    cJSON_Delete(response);
    scope_free(buf);

    return res;
}
