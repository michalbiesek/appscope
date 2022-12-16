#define _GNU_SOURCE

#include "ipc.h"

#include "cJSON.h"
#include "dbg.h"
#include "runtimecfg.h"
#include <errno.h>
#include <time.h>

/* Inter-process communication module based on the message-queue
 *
 * Message-queue system limits which are defined in following files:
 *
 * "/proc/sys/fs/mqueue/msg_max"
 * - describes maximum number of messsages in a queue
 *
 * "/proc/sys/fs/mqueue/maxMsgSize_max"
 * - describes maximum message size in a queue
 *
 * "/proc/sys/fs/mqueue/queues_max"
 * - describes system-wide limit on the number of message queues that can be created
 *
 * See details in: https://man7.org/linux/man-pages/man7/mq_overview.7.html
 */


/*
 * ipc_cmd_t describes the scope request command retrieves from IPC communication
 * IMPORTANT NOTE:
 * ipc_cmd_t must be inline with client: requestCmd
 */
typedef enum {
    IPC_CMD_GET_SCOPE_STATUS, // Retrieves scope status of application (enabled or disabled)
    IPC_CMD_GET_SCOPE_CFG,    // Retrieves the current configuration
    IPC_CMD_UNKNOWN,          // Should be last - points to unsupported message
} ipc_cmd_t;

// Size of the metadata for basic message and for the frame variant
#define METADATA_FRAME_SIZE (sizeof("{\"status\":200,\"id\":100,\"max\":100,data:""} "))
#define RETRY_COUNT 10

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
respStatusLookup(req_parse_status_t status) {
    switch (status) {
    case REQ_PARSE_OK:
        return IPC_RESP_OK;
    case REQ_PARSE_INTERNAL_ERROR:
        return IPC_RESP_SERVER_ERROR;
    case REQ_PARSE_JSON_ERROR:
    case REQ_PARSE_MISS_MANDATORY_ERROR:
        return IPC_BAD_REQUEST;
    default:
        // TODO: Add scope layer for compiler hint
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
 * Checks if specific message-queue is active,
 * If active additionally returns the maximum message size
 * and numbers of messages in the queque
 */
bool
ipcIsActive(mqd_t mqdes, size_t *maxMsgSize, long *msgCount) {
    struct mq_attr attr;
    if (mqdes == (mqd_t)-1) {
        return FALSE;
    }

    if (scope_mq_getattr(mqdes, &attr) == -1) {
        return FALSE;
    }
    *maxMsgSize = attr.mq_maxmsg;
    *msgCount = attr.mq_curmsgs;
    return TRUE;
}

static char *
ipcParseSingleFrame(const char *msgBuf, req_parse_status_t *parseStatus, int *uniqVal) {
    char *res = NULL;

    // Verify if request is based on JSON-format
    cJSON *mqReq = cJSON_Parse(msgBuf);
    if (!mqReq) {
        *parseStatus = REQ_PARSE_JSON_ERROR;
        goto end;
    }

    if (!cJSON_IsObject(mqReq)) {
        *parseStatus = REQ_PARSE_JSON_ERROR;
        goto cleanJson;
    }

    // TODO: Validate request should be equals 0
    // cJSON *cmdReq = cJSON_GetObjectItemCaseSensitive(req, "req");
    // if (!cmdReq || !cJSON_IsNumber(cmdReq)) {
    //     res = IPC_REQ_MANDATORY_FIELD_ERROR;
    //     goto cleanJson;
    // }

    cJSON *mqUniq = cJSON_GetObjectItemCaseSensitive(mqReq, "uniq");
    if (!mqUniq || !cJSON_IsNumber(mqUniq)) {
        *parseStatus = REQ_PARSE_MISS_MANDATORY_ERROR;
        goto cleanJson;
    }
    *uniqVal = mqUniq->valueint;

    // TODO: Validate frame ID's and max ID's

    cJSON *mqData = cJSON_GetObjectItemCaseSensitive(mqReq, "data");
    if (!mqData || !cJSON_IsString(mqData)) {
        *parseStatus = REQ_PARSE_MISS_MANDATORY_ERROR;
        goto cleanJson;
    }

    res = scope_strdup(mqData->valuestring);
    *parseStatus = REQ_PARSE_OK;

cleanJson:
    cJSON_Delete(mqReq);

end:
    return res;
}

/*
 * Parse single request, returns parse request status and command
 */
// static char *
// ipcParseSingleReq(mqd_t mqDes, char *msgBuf, size_t msgSize, req_parse_status_t *parseStatus) {
//     ipc_cmd_t supportedCmd;
//     ipc_req_status_t res = IPC_REQ_INTERNAL_ERROR;
//     ssize_t recvLen = ipcRecv(mqDes, msgBuf, msgSize);

//     if (recvLen == -1) {
//         return res;
//     }

//     // Verify if request is based on JSON-format
//     cJSON *req = cJSON_Parse(msgBuf);
//     if (!req) {
//         return IPC_REQ_NOT_JSON_ERROR;
//     }

//     if (!cJSON_IsObject(req)) {
//         res = IPC_REQ_NOT_JSON_ERROR;
//         goto cleanJson;
//     }

//     // Mandatory field
//     cJSON *cmdReq = cJSON_GetObjectItemCaseSensitive(req, "req");
//     if (!cmdReq || !cJSON_IsNumber(cmdReq)) {
//         res = IPC_REQ_MANDATORY_FIELD_ERROR;
//         goto cleanJson;
//     }

//     // TODO add other fields


//     cJSON *cmdReq = cJSON_GetObjectItemCaseSensitive(req, "req");
//     if (!cmdReq || !cJSON_IsNumber(cmdReq)) {
//         res = IPC_REQ_MANDATORY_FIELD_ERROR;
//         goto cleanJson;
//     }

//     *parseStatus = IPC_REQ_OK;
//     res = IPC_REQ_OK;

// cleanJson:
//     cJSON_Delete(req);

//     return res;
// }


/* ipcMQRequestHandler performs parsing of incoming message queue request
 * Returns status of parsing message queuque request and command in case of success
 * TODO add support for multiple frames
 */
char *
ipcMQRequestHandler(mqd_t mqDes, size_t maxMsgSize, req_parse_status_t *parseStatus, int *uniqueReq) {
    char *res = NULL;

    *parseStatus = REQ_PARSE_INTERNAL_ERROR;

    // Allocate maximum buffer for single meesage
    char *msgBuf = scope_malloc(maxMsgSize);
    if (!msgBuf) {
        return res;
    }

    // TODO add support for multiple frames
    ssize_t recvLen = ipcRecv(mqDes, msgBuf, maxMsgSize);
    if (recvLen == -1) {
        scope_free(msgBuf);
        return res;
    }
    res = ipcParseSingleFrame(msgBuf, parseStatus, uniqueReq);

    scope_free(msgBuf);

    return res;
}

    // bool firstFrame = TRUE;

    // int firstUniq;

    // // Assume that this is single-frame message 
    // int frameIdMax = 1;
    // int frameIdCurrent = 1;
    // int currentSize = 0;

    // for (int i = 1; i <= frameIdMax; ++i) {


    //     ssize_t recvLen = ipcRecv(mqDes, msgBuf, maxMsgSize);
    //     // TODO: Handle wait & mechanism since we do not check if message queue can be full
    //     if (recvLen == -1) {
    //         return res;
    //     }

    //     // Verify if message queue reuest is JSON-based
    //     cJSON *mqReq = cJSON_Parse(msgBuf);
    //     if (!mqReq) {
    //         *parseStatus = REQ_PARSE_JSON_ERROR;
    //         goto end;
    //     }
    //     if (!cJSON_IsObject(mqReq)) {
    //         *parseStatus = REQ_PARSE_JSON_ERROR;
    //         goto end;
    //     }

        // // Mandatory field "req"
        // cJSON *mqReqField = cJSON_GetObjectItemCaseSensitive(mqReq, "req");
        // if (!mqReqField || !cJSON_IsNumber(mqReqField)) {
        //     cJSON_Delete(mqReq);
        //     *parseStatus = REQ_PARSE_MISS_MANDATORY_ERROR;
        //     goto end;
        // }

        // Validation of req
        // if (mqReqField->valueint != 0) {
        //     *parseStatus = REQ_PARSE_MISS_MANDATORY_ERROR;
        //     goto end;
        // }

        // // Mandatory field "uniq"
        // cJSON *mqUniqField = cJSON_GetObjectItemCaseSensitive(mqReq, "uniq");
        // if (!mqUniqField || !cJSON_IsNumber(mqUniqField)) {
        //     cJSON_Delete(mqReq);
        //     *parseStatus = REQ_PARSE_MISS_MANDATORY_ERROR;
        //     goto end;
        // }

        // // Validation of unique
        // if (firstFrame) {
        //     firstUniq = mqUniqField->valueint;
        // } else if (firstUniq != mqUniqField->valueint) {
        //     cJSON_Delete(mqReq);
        //     *parseStatus = REQ_PARSE_MISS_MANDATORY_ERROR;
        //     goto end;
        // }

        // // Mandatory field "max"
        // cJSON *mqMaxField = cJSON_GetObjectItemCaseSensitive(mqReq, "max");
        // if (!mqMaxField || !cJSON_IsNumber(mqMaxField)) {
        //     cJSON_Delete(mqReq);
        //     *parseStatus = REQ_PARSE_MISS_MANDATORY_ERROR;
        //     goto end;
        // }

        // // Validation of max
        // if (firstFrame) {
        //     frameIdMax = mqMaxField->valueint;
        // } else if (frameIdMax != mqMaxField->valueint) {
        //     cJSON_Delete(mqReq);
        //     *parseStatus = REQ_PARSE_MISS_MANDATORY_ERROR;
        //     goto end;
        // }

        // // Mandatory field "id"
        // cJSON *mqIdField = cJSON_GetObjectItemCaseSensitive(mqReq, "id");
        // if (!mqIdField || !cJSON_IsNumber(mqIdField)) {
        //     cJSON_Delete(mqReq);
        //     *parseStatus = REQ_PARSE_MISS_MANDATORY_ERROR;
        //     goto end;
        // }


        // // Validation of id
        // if ((firstFrame) && (mqIdField->valueint != 1)) {
        //     cJSON_Delete(mqReq);
        //     *parseStatus = REQ_PARSE_MISS_MANDATORY_ERROR;
        //     goto end;
        // }

        
        // frameIdCurrent += 1;


        // } else if (frameIdCurrent != (mqIdField->valueint + 1)) {
        //     cJSON_Delete(mqReq);
        //     *parseStatus = REQ_PARSE_MISS_MANDATORY_ERROR;
        //     goto end;
        // }


        // // Mandatory field "data"
        // cJSON *mqDataField = cJSON_GetObjectItemCaseSensitive(mqReq, "data");
        // if (!mqDataField || !cJSON_IsString(mqDataField)) {
        //     cJSON_Delete(mqReq);
        //     *parseStatus = REQ_PARSE_MISS_MANDATORY_ERROR;
        //     goto end;
        // }



/*
 * Sends splitted responses, returns status of operation
 */
static bool
ipcSendMsgQResponse(mqd_t mqDes, size_t msgBufSize, cJSON *dataResp, int uniqReq) {
    char *data = cJSON_PrintUnformatted(dataResp);
    size_t dataLen = scope_strlen(data);

    bool res = FALSE;
    
    if (msgBufSize < METADATA_FRAME_SIZE) {
        return res;
    }
    size_t dataMaxSize = msgBufSize - METADATA_FRAME_SIZE;

    int totalFrames = dataLen/dataMaxSize;
    if (dataLen%dataMaxSize) {
        totalFrames += 1;
    }

    size_t msgOffset = 0;
    size_t respCounter = dataLen;

    char *tempDataBuf = scope_calloc(1, sizeof(char) * dataMaxSize);

    for (int i = 1; i <= totalFrames; ++i) {
        int retryCount = RETRY_COUNT;
        size_t dataSize;

        if (respCounter > dataMaxSize) {
            respCounter -= dataMaxSize;
            dataSize = dataMaxSize;
        } else {
            dataSize = respCounter;
        }

        cJSON *frame = cJSON_CreateObject();
        if (totalFrames == 1) {
            cJSON_AddNumberToObjLN(frame, "status", IPC_RESP_OK);
        } else {
            cJSON_AddNumberToObjLN(frame, "status", IPC_PARTIAL_CONTENT);
        }
        cJSON_AddNumberToObjLN(frame, "uniq", uniqReq);
        cJSON_AddNumberToObjLN(frame, "id", i);
        cJSON_AddNumberToObjLN(frame, "max", totalFrames);

        // Split the message
        scope_memcpy(tempDataBuf, data + msgOffset, dataSize);
        cJSON_AddStringToObjLN(frame, "data", tempDataBuf);
        msgOffset += dataSize;

        char *out = cJSON_PrintUnformatted(frame);
        size_t frameLen = scope_strlen(out);

        while (retryCount--) {
            int sendRes = ipcSend(mqDes, out, frameLen);
            if (sendRes == 0) {
                break;
            } else if(scope_errno == EAGAIN) {
                // ipcSend fails with EAGAIN when the queue is full
                // 50 us
                scope_nanosleep((const struct timespec[]){{0, 50000L}}, NULL);
            } else {
                cJSON_Delete(frame);
                return FALSE;
            }
        }

        cJSON_Delete(frame);

        if (!retryCount) {
            return FALSE;
        }
        scope_memset(tempDataBuf, 0, sizeof(char) * dataMaxSize);
    }
    return TRUE;
}

static req_parse_status_t
ipcParseScopeRequest(const char *scopeData, ipc_cmd_t* cmd) {
    req_parse_status_t status = REQ_PARSE_JSON_ERROR;
    ipc_cmd_t supportedCmd;

    // Verify if scope request is based on JSON-format
    cJSON *scopeReq = cJSON_Parse(scopeData);
    if (!scopeReq) {
        goto cleanJson;
    }

    if (!cJSON_IsObject(scopeReq)) {
        goto cleanJson;
    }

    cJSON *cmdReq = cJSON_GetObjectItemCaseSensitive(scopeReq, "req");
    if (!cmdReq || !cJSON_IsNumber(cmdReq)) {
        status = REQ_PARSE_MISS_MANDATORY_ERROR;
        goto cleanJson;
    }

    for (supportedCmd = 0; supportedCmd < IPC_CMD_UNKNOWN; ++supportedCmd) {
        if (cmdReq->valueint == supportedCmd) {
            break;
        }
    }

    status = REQ_PARSE_OK;
    *cmd = supportedCmd;

cleanJson:

    cJSON_Delete(scopeReq);

    return status;
}

// Create Message Queue response
// Input data
//  - scopeReq -> data
//  - uniqReq -> uniq field
//  - mqReqStatus 
bool
ipcResponseMQHandler(mqd_t mqDes, size_t msgBufSize, const char *scopeReq, req_parse_status_t mqReqStatus, int uniqReq) {
    bool res = FALSE;

    ipc_resp_status_t mqRespStatus = respStatusLookup(mqReqStatus);

    // Create dataResponse
    cJSON *scopeResp = cJSON_CreateObject();
    if (!scopeResp) {
        return res;
    }
    // Error case based on the wrong request
    if (mqRespStatus != IPC_RESP_OK) {
        // Response based on client request parsing error
        cJSON_AddNumberToObjLN(scopeResp, "status", mqRespStatus);
    } else {
        ipc_cmd_t supportedCmd = IPC_CMD_UNKNOWN;
        // Handling scope request individually
        req_parse_status_t scopeReqParseStatus = ipcParseScopeRequest(scopeReq, &supportedCmd);
        if (scopeReqParseStatus == REQ_PARSE_OK) {
            switch (supportedCmd) {
                case IPC_CMD_GET_SCOPE_STATUS:
                    cJSON_AddNumberToObjLN(scopeResp, "status", scopeReqParseStatus);
                    cJSON_AddBoolToObjLN(scopeResp, "scoped", (g_cfg.funcs_attached));
                    break;
                case IPC_CMD_UNKNOWN:
                    cJSON_AddNumberToObjLN(scopeResp, "status", IPC_RESP_NOT_IMPLEMENTED);
                    break;
                case IPC_CMD_GET_SCOPE_CFG:
                    // TODO: add real implementation
                    cJSON_AddNumberToObjLN(scopeResp, "status", scopeReqParseStatus);
                    cJSON_AddNumberToObjLN(scopeResp, "fieldTest1", IPC_RESP_NOT_IMPLEMENTED);
                    cJSON_AddNumberToObjLN(scopeResp, "fieldTest2", IPC_RESP_NOT_IMPLEMENTED);
                    cJSON_AddNumberToObjLN(scopeResp, "fieldTest3", IPC_RESP_NOT_IMPLEMENTED);
                    cJSON_AddNumberToObjLN(scopeResp, "fieldTest4", IPC_RESP_NOT_IMPLEMENTED);
                    cJSON_AddNumberToObjLN(scopeResp, "fieldTest5", IPC_RESP_NOT_IMPLEMENTED);
                    break;
                default:
                    DBG(NULL);
                    break;
            }
        } else {
            cJSON_AddNumberToObjLN(scopeResp, "status", respStatusLookup(scopeReqParseStatus));
        }
    }

    res = ipcSendMsgQResponse(mqDes, msgBufSize, scopeResp, uniqReq);

    cJSON_Delete(scopeResp);

    return res;
}
