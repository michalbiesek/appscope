# Protocol

## Note: Request/Response can consists of multpile messages (concatenation)

### Client Request

| Field name   | Length (bytes) | Description                                         |
|--------------|----------------|-----------------------------------------------------|
| INS          | 2              | IPC command (e.g getScopeStatus)                    |
| REQ_ID       | 2              | Unique ID of the request (randomize by client)      |
| PARARMETER   | 2              | Parameter                                           |
| MSG_NO       | 2              | Current Id of the frame                             |
| TOTAL_MSG_NO | 2              | Total number of frame within this request           |
| RFC_1        | 2              |                                                     |
| RFC_2        | 2              |                                                     |
| REQ_DATA     | n              | Request data, for example `cfg` in case of `setCfg` |

4 bytes metadata per each request.

### Server Response

| Field name | Length (bytes) | Description                                              |
|------------|----------------|----------------------------------------------------------|
| REQ_ID        | 1              | ID of the response equals to unique ID of the request |
| STATUS        | 1              | Overall status of request                             |
|               |                | - examples success, command not found, command failed |
| MSG_NO        | 1              | Current Id of the message                             |
| TOTAL_MSG_NO  | 1              | Total number of messages within this response         |
| RFC_1         | 2              | Unused                                                |
| RFC_2         | 2              | Unused                                                |
| RESP_DATA     | n              | Response data for example `cfg` in case of `getCfg`   |

4 bytes reserved

4 bytes metadata per each response.

## Example communication `get scope status`

### Request (Get Scope Status messsage):

-> `0x01 0xCD 0x01 0x01`

`INS` - `0x01` - GetScopeStatus

`REQ_ID` - `0xCD` - Unique Id of this request

`MSG_NO` - `0x01` - Message no 1

`TOTAL_MSG_NO` - `0x01` - Total message of this request is 1

`REQ_DATA` - not present

### Response (Respones for get Scope Status messsage -> scope is Active):

<- `0xCD 0x00 0x01 0x01 0x01`

`REQ_ID` - `0xCD` - GetScopeStatus

`STATUS` - `0x00` - OK (Success)

`MSG_NO` - `0x01` - message no 1

`TOTAL_MSG_NO` - `0x01` - total message of this request is 1

`RESP_DATA` - `0x01` - (`0x01` - Active state, `0x00` - Latent state)

## Questions:

- `INS`, `REQ_ID`, `MSG_NO`, `TOTAL_MSG_NO` - is one byte sufficient ?
- Do we need to know version of the client ?
- `RESP_DATA`, `REQ_DATA` interpreation for each commmand should be immutable once defined
