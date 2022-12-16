#define _GNU_SOURCE

#include "ipc.h"

#include "cJSON.h"
#include "dbg.h"
#include "runtimecfg.h"

/* Inter-process communication module based on the message-queue
 *
 * Message-queue system limits which are related to `ipcOpenConnection`
 * are defined in following files:
 *
 * "/proc/sys/fs/mqueue/msg_max"
 * - describes maximum number of messsages in a queue
 *
 * "/proc/sys/fs/mqueue/msgsize_max"
 * - describes maximum message size in a queue
 *
 * "/proc/sys/fs/mqueue/queues_max"
 * - describes system-wide limit on the number of message queues that can be created
 *
 * See details in: https://man7.org/linux/man-pages/man7/mq_overview.7.html
 */


// Size of the metadata for basic message and for the frame variant
#define METADATA_BASIC_SIZE (sizeof("{\"status\":200} "))
#define METADATA_FRAME_SIZE (sizeof("{\"status\":200,\"id\":100,\"max\":100,data:""} "))

// This must be inline with respStatVal in ipccmd.go
typedef enum {
    IPC_RESP_OK = 200,              // Response OK
    IPC_PARTIAL_CONTENT = 206,      // Partial content will be send
    IPC_BAD_REQUEST = 400,          // Invalid message syntax from client
    IPC_RESP_SERVER_ERROR = 500,    // Internal Server Error
    IPC_RESP_NOT_IMPLEMENTED = 501, // Method not implemented
} ipc_resp_status_t;

/*
 * Translates the internal status of parsing request to the response output status
 */
static ipc_resp_status_t
respStatusLookup(ipc_req_status_t req) {
    switch (req) {
    case IPC_REQ_OK:
        return IPC_RESP_OK;
    case IPC_REQ_INTERNAL_ERROR:
        return IPC_RESP_SERVER_ERROR;
    case IPC_REQ_NOT_JSON_ERROR:
    case IPC_REQ_MANDATORY_FIELD_ERROR:
        return IPC_BAD_REQUEST;
    default:
        // __builtin_unreachable();
        DBG(NULL);
        return IPC_RESP_SERVER_ERROR;
    }
}

/*
 * Sends the data using IPC mechanism
 */
static int
ipcSend(mqd_t mqdes, const char *data, size_t len) {
    return scope_mq_send(mqdes, data, len, 0);
}

/*
 * Receives the data using IPC mechanism
 */
static ssize_t
ipcRecv(mqd_t mqdes, char *buf, size_t len) {
    return scope_mq_receive(mqdes, buf, len, 0);
}

/*
 * Opens the IPC connection for sending data as non-block IO
 */
mqd_t ipcOpenWriteConnection(const char *name) {
    return scope_mq_open(name, O_WRONLY | O_NONBLOCK);
}

/*
 * Open the IPC connection for receiving data as non-block IO
 */
mqd_t
ipcOpenReadConnection(const char *name) {
    return scope_mq_open(name, O_RDONLY | O_NONBLOCK);
}

/*
 * Closes the specific IPC connection
 */
int
ipcCloseConnection(mqd_t mqdes) {
    return scope_mq_close(mqdes);
}

/*
 * Get the number of messages on specific message-queue
 */
long
ipcInfoMsgCount(mqd_t mqdes) {
    struct mq_attr attr;
    int res = scope_mq_getattr(mqdes, &attr);
    if (res == 0) {
        return attr.mq_curmsgs;
    }
    return -1;
}

/*
 * Checks if specific message-queue is active,
 * If active additionally returns the maximum message size
 */
bool
ipcIsActive(mqd_t mqdes, size_t *msgSize) {
    struct mq_attr attr;
    if (mqdes == (mqd_t)-1) {
        return FALSE;
    }

    if (scope_mq_getattr(mqdes, &attr) == -1) {
        return FALSE;
    }
    *msgSize = attr.mq_msgsize;
    return TRUE;
}

/* ipcRequestMsgHandler performs parsing of incoming request.
 * Returns status of parsing request and command
 */
ipc_req_status_t
ipcRequestMsgHandler(mqd_t mqDes, size_t msgSize, ipc_cmd_t *cmdRes) {
    ipc_cmd_t supportedCmd;
    ipc_req_status_t res = IPC_REQ_INTERNAL_ERROR;

    char *buf = scope_malloc(msgSize);
    if (!buf) {
        return res;
    }

    ssize_t recvLen = ipcRecv(mqDes, buf, msgSize);
    if (recvLen == -1) {
        goto cleanBuf;
    }

    // Verify if request is based on JSON-format
    cJSON *req = cJSON_Parse(buf);
    if (!req) {
        res = IPC_REQ_NOT_JSON_ERROR;
        goto cleanBuf;
    }

    if (!cJSON_IsObject(req)) {
        res = IPC_REQ_NOT_JSON_ERROR;
        goto cleanJson;
    }

    // Mandatory field
    cJSON *cmdReq = cJSON_GetObjectItemCaseSensitive(req, "req");
    if (!cmdReq || !cJSON_IsNumber(cmdReq)) {
        res = IPC_REQ_MANDATORY_FIELD_ERROR;
        goto cleanJson;
    }

    // Search for cmd request
    for (supportedCmd = 0; supportedCmd < IPC_CMD_UNKNOWN; ++supportedCmd) {
        if (cmdReq->valueint == supportedCmd) {
            break;
        }
    }

    if (cmdReq->valueint == IPC_CMD_CJSON_FRAMED) {
        // TODO join the request wait to receive all frames
    }

    *cmdRes = supportedCmd;

    res = IPC_REQ_OK;

cleanJson:
    cJSON_Delete(req);

cleanBuf:
    scope_free(buf);

    return res;
}

static bool
ipcSendSingleResponse(mqd_t mqDes, char *resp, size_t respLen) {
    return (ipcSend(mqDes, resp, respLen) != -1);
}

static bool
ipcSendSplitResponse(mqd_t mqDes, size_t maxMsgSize, char *resp, size_t respLen) {
    bool res = FALSE;
    
    if (maxMsgSize < METADATA_FRAME_SIZE) {
        return res;
    }
    size_t dataMaxSize = maxMsgSize - METADATA_FRAME_SIZE;

    int totalFrames = respLen/dataMaxSize;
    if (respLen%dataMaxSize) {
        totalFrames += 1;
    }

    size_t msgOffset = 0;
    size_t respCounter = respLen;

    for (int i = 1; i <= totalFrames; ++i) {
        char tempDataBuf[4096] = {0};
        size_t dataSize;

        if (respCounter > dataMaxSize) {
            respCounter -= dataMaxSize;
            dataSize = dataMaxSize;
        } else if (respCounter == dataMaxSize){
            dataSize = respCounter;
        }

        cJSON *frame = cJSON_CreateObject();
        cJSON_AddNumberToObjLN(frame, "status", IPC_PARTIAL_CONTENT);
        cJSON_AddNumberToObjLN(frame, "id", i);
        cJSON_AddNumberToObjLN(frame, "max", totalFrames);

        // Split the message
        scope_memcpy(tempDataBuf, resp + msgOffset, dataSize);
        cJSON_AddStringToObjLN(frame, "data", (char *)tempDataBuf);
        msgOffset += dataSize;

        char *out = cJSON_PrintUnformatted(frame);
        size_t frameLen = scope_strlen(out);

        int sendRes = ipcSend(mqDes, out, frameLen);
        if (sendRes != -1) {
            res = TRUE;
        } else {
            // TODO: Add error handling
        }
        // TODO: Add wait and retry mechanism
        cJSON_Delete(frame);
    }
    return res;
}

bool
ipcResponseMsgHandler(mqd_t mqDes, size_t maxMsgSize, ipc_cmd_t cmd, ipc_req_status_t reqStatus) {
    bool res = FALSE;

    if (maxMsgSize < METADATA_BASIC_SIZE) {
        return res;
    }

    cJSON *resp = cJSON_CreateObject();
    if (!resp) {
        return res;
    }
    ipc_resp_status_t respStatus = respStatusLookup(reqStatus);

    if (respStatus == IPC_RESP_OK) {
        switch (cmd) {
            case IPC_CMD_GET_SCOPE_STATUS:
                cJSON_AddNumberToObjLN(resp, "status", respStatus);
                cJSON_AddBoolToObjLN(resp, "scoped", (g_cfg.funcs_attached));
                break;
            case IPC_CMD_UNKNOWN:
                cJSON_AddNumberToObjLN(resp, "status", IPC_RESP_NOT_IMPLEMENTED);
                break;
            case IPC_CMD_GET_SCOPE_CFG:
                // TODO: add real implementation
                cJSON_AddNumberToObjLN(resp, "status", respStatus);
                cJSON_AddNumberToObjLN(resp, "fieldTest1", IPC_RESP_NOT_IMPLEMENTED);
                cJSON_AddNumberToObjLN(resp, "fieldTest2", IPC_RESP_NOT_IMPLEMENTED);
                cJSON_AddNumberToObjLN(resp, "fieldTest3", IPC_RESP_NOT_IMPLEMENTED);
                cJSON_AddNumberToObjLN(resp, "fieldTest4", IPC_RESP_NOT_IMPLEMENTED);
                cJSON_AddNumberToObjLN(resp, "fieldTest5", IPC_RESP_NOT_IMPLEMENTED);
                break;
            default:
                DBG(NULL);
                break;
        }
    } else {
        // Response based on request parsing error
        cJSON_AddNumberToObjLN(resp, "status", respStatus);
    }

    char *out = cJSON_PrintUnformatted(resp);
    size_t outLen = scope_strlen(out);

    // if total length of message exceed the size of frame 
    if (outLen > maxMsgSize) {
        res = ipcSendSplitResponse(mqDes, maxMsgSize, out, outLen);
    } else {
        res = ipcSendSingleResponse(mqDes, out, outLen);
    }

    cJSON_Delete(resp);

    return res;
}
