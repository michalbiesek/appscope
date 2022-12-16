#ifndef __IPC_H__
#define __IPC_H__

#include "scopestdlib.h"
#include "scopetypes.h"

// Manage Inter-process connection
mqd_t ipcOpenReadConnection(const char *);
mqd_t ipcOpenWriteConnection(const char *);
int ipcCloseConnection(mqd_t);
bool ipcIsActive(mqd_t, size_t *, long *);

/*  Request example:
 *
 *   Sender (Scope CLI)                        Receiver (Scoped Process)
 *
 *   _________________                         _________________
 *   | scope message |                         | scope message |
 *   |   format      |                         |   format      |
 *   |_______________|                         |_______________|
 *       |                                        /|\
 *       |                                         |
 *       | <packing>                               | <unpacking>
 *   ___\|/___________                         ____|____________ 
 *   | message queue |        <framing>        | message queue |
 *   |   format      |==========[IPC]==========|   format      |
 *   |_______________|                         |_______________|
 *      
 *   
 */

/*
 * Internal status of parsing the request, which can be:
 * - success after joining all the frames in the message
 * - error during parsing frames (processing will not wait for all frames)
 * - status is used both for scope_msg and mq_msg
 */
typedef enum {
    REQ_PARSE_GENERIC_ERROR,          // Error: generic error
    REQ_PARSE_INTERNAL_ERROR,         // Error: memory allocation fails or empty queue
    REQ_PARSE_FRAME_ERROR,            // Error: request frame error
    REQ_PARSE_JSON_ERROR,             // Error: request it not based on JSON format
    REQ_PARSE_MISS_MANDATORY_ERROR,   // Error: request missed one of mandatory fields
    REQ_PARSE_OK,                     // Request was succesfully parsed
} req_parse_status_t;


/* IPC Request Handler
 *
 * Handler for message queue request
 * IMPORTANT NOTE:
 * Message queue request MUST be inline with client code defintion: msgQRequest
 * 
 * - req - number - message queue request command type (currently only 0 is supported)
 * - uniq - number - unique identifier of message request
 * - id - number - current frame in the message request
 * - max - number - last frame in the message request
 * - data - string - data frame in the message request
 */
char *ipcMQRequestHandler(mqd_t, size_t, req_parse_status_t *, int *);

/* IPC Response Handler
 *
 * Handler for message queue response
 * IMPORTANT NOTE:
 * Message queue response MUST be inline with client code defintion: msgQResponse
 * 
 * msgQResponse
 * - status - number - message queue response status
 * - uniq - number - unique identifier of message request
 * - id - number - current frame in the message response
 * - max - number - last frame in the message response
 * - data - string - data frame  in the message response
 */
bool ipcResponseMQHandler(mqd_t, size_t, const char*, req_parse_status_t, int);

#endif // __IPC_H__
