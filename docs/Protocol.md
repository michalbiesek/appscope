# Protocol

## Note: Request/Response can consists of multpile messages (concatenation)

### Client Request

| Field name   | Length (bytes) | Description                                         |
|--------------|----------------|-----------------------------------------------------|
| INS          | 2              | IPC command (e.g getScopeStatus)                    |
| REQ_ID       | 2              | Unique ID of the request (randomize by client)      |
| MSG_NO       | 2              | Current Id of the message                           |
| TOTAL_MSG_NO | 2              | Total number of messages within this request        |
| RFC_FIELD    | 2              | For future use                                      |
| REQ_DATA     | n              | Request data, for example `cfg` in case of `setCfg` |

10 bytes metadata per each request.

### Server Response

| Field name | Length (bytes) | Description                                           |
|------------|----------------|-------------------------------------------------------|
| REQ_ID     | 2              | ID of the response equals to unique ID of the request |
| STATUS     | 2              | Overall status of request                             |
|            |                | - examples success, command not found, command failed |
| MSG_NO     | 2              | Current Id of the message                             |
| MAX_ID     | 2              | Total number of messages within this response         |
| RFC_FIELD  | 2              | For future use                                      |
| RESP_DATA  | n              | Response data for example `cfg` in case of `getCfg`   |

10 bytes metadata per each response.

## Example communication `get scope status`

### Request (Get Scope Status messsage):

-> `0x00 0x01 0xEF 0xCD 0x00 0x01 0x00 0x01 0x00 0x00`

`INS` - `0x00 0x01` - GetScopeStatus

`REQ_ID` - `0xEF 0xCD` - Unique Id of this request

`MSG_NO` - `0x00 0x01` - Message no 1

`TOTAL_MSG_NO` - `0x00 0x01` - Total message of this request is 1

`RFC_FIELD` - `0x00 0x00` - Unused

`REQ_DATA` - not present

### Response (Respones for get Scope Status messsage -> scope is Active):

<- `0xCD 0x00 0x01 0x01 0x01`

`REQ_ID` - `0xCD` - GetScopeStatus

`STATUS` - `0x00` - OK (Success)

`MSG_NO` - `0x01` - message no 1

`TOTAL_MSG_NO` - `0x01` - total message of this request is 1

`RESP_DATA` - `0x01` - (`0x01` - Active state, `0x00` - Latent state)
