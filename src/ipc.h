#ifndef __IPC_H__
#define __IPC_H__

#include "scopestdlib.h"
#include "scopetypes.h"

// Manage Inter-process connection
mqd_t ipcOpenReadConnection(const char *);
mqd_t ipcOpenWriteConnection(const char *);
int ipcCloseConnection(mqd_t);
long ipcInfoMsgCount(mqd_t);
bool ipcIsActive(mqd_t, size_t *);

// IPC Request Handler

// ipc_cmd_t describes the command retrieves from IPC communication
// IMPORTANT NOTE:
// ipc_cmd_t must be inline with client: requestCmd
typedef enum {
    IPC_CMD_CJSON_FRAMED,     // Framed request message - contains data when joined will be based on json
    IPC_CMD_GET_SCOPE_STATUS, // Retrieves scope status of application (enabled or disabled)
    IPC_CMD_GET_SCOPE_CFG,    // Retrieves the current configuration
    IPC_CMD_UNKNOWN,          // Should be last - points to unsupported message
} ipc_cmd_t;


// Internal status of parsing the request
typedef enum {
    IPC_REQ_OK,                     // Request was succesffully parsed
    IPC_REQ_INTERNAL_ERROR,         // Error: memory allocation fails or empty queue
    IPC_REQ_NOT_JSON_ERROR,         // Error: request it not based on JSON format
    IPC_REQ_MANDATORY_FIELD_ERROR,  // Error: request missed mandatory field
} ipc_req_status_t;


ipc_req_status_t ipcRequestMsgHandler(mqd_t, size_t, ipc_cmd_t *);

// IPC Response Handler

bool ipcResponseMsgHandler(mqd_t, size_t, ipc_cmd_t, ipc_req_status_t);

#endif // __IPC_H__
