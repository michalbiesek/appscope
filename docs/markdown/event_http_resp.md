# event_http_resp.schema (`object`)

AppScope Event

A structure described AppScope http.resp event message

## Properties

### type _required_ (`string`)

type

event

### id _required_ (`string`)

id

id

### _channel _required_ (`string`)

_channel

_channel

### body _required_ (`object`)

body

body

#### Properties

##### sourcetype _required_ (`string`)

http

Source type - http

##### _time _required_ (`number`)

_time

_time

###### Examples

`"1643662126.91777"`

##### source _required_ (`string`)

http.resp

Source - HTTP response

##### host _required_ (`string`)

host

host

##### proc _required_ (`string`)

proc

proc

##### cmd _required_ (`string`)

cmd

cmd

###### Examples

```json
"top"
```

##### pid _required_ (`integer`)

pid

pid

###### Examples

`1000`

##### data _required_ (`object`)

data

data

###### Properties

####### http_method _required_ (`string`)

http_method

http_method

####### http_target _required_ (`string`)

http_target

http_target

####### http_stream (`integer`)

http_stream

http_stream

####### http_scheme _required_ (`string`)

http_scheme

http_scheme

Possible values:

- `http`
- `https`

####### http_flavor _required_ (`string`)

http_flavor

http_flavor

####### http_status_code _required_ (`integer`)

http_status_code

http_status_code

Possible values:

- `100`
- `101`
- `102`
- `200`
- `201`
- `202`
- `203`
- `204`
- `205`
- `206`
- `207`
- `208`
- `226`
- `300`
- `301`
- `302`
- `303`
- `304`
- `305`
- `307`
- `400`
- `401`
- `402`
- `403`
- `404`
- `405`
- `406`
- `407`
- `408`
- `409`
- `410`
- `411`
- `412`
- `413`
- `414`
- `415`
- `416`
- `417`
- `418`
- `421`
- `422`
- `423`
- `424`
- `426`
- `428`
- `429`
- `431`
- `444`
- `451`
- `499`
- `500`
- `501`
- `502`
- `503`
- `504`
- `505`
- `506`
- `507`

####### http_status_text _required_ (`string`)

http_status_text

http_status_text

Possible values:

- `Continue`
- `Switching Protocols`
- `Processing`
- `OK`
- `Created`
- `Accepted`
- `Non-authoritaive Information`
- `No Content`
- `Reset Content`
- `Partial Content`
- `Multi-Status`
- `Already Reported`
- `IM Used`
- `Multiple Choices`
- `Moved Permanently`
- `Found`
- `See Other`
- `Not Modified`
- `Use Proxy`
- `Temporary Redirect`
- `Permanent Redirect`
- `Bad Request`
- `Unauthorized`
- `Payment Required`
- `Forbidden`
- `Not Found`
- `Method Not Allowed`
- `Not Acceptable`
- `Proxy Authentication Required`
- `Request Timeout`
- `Conflict`
- `Gone`
- `Length Required`
- `Precondition Failed`
- `Payload Too Large`
- `Request-URI Too Long`
- `Unsupported Media Type`
- `Requested Range Not Satisfiable`
- `Expectation Failed`
- `I'm a teapot`
- `Misdirected Request`
- `Unprocessable Entity`
- `Locked`
- `Failed Dependency`
- `Upgrade Required`
- `Precondition Required`
- `Too Many Requests`
- `Request Header Fields Too Large`
- `Connection Closed Without Response`
- `Unavailable For Legal Reasons`
- `Client Closed Request`
- `Internal Server Error`
- `Not Implemented`
- `Bad Gateway`
- `Service Unavailable`
- `Gateway Timeout`
- `HTTP Version Not Supported`
- `Variant Also Negotiates`
- `Insufficient Storage`

####### http_client_duration (`number`)

http_client_duration

http_client_duration

####### http_server_duration (`number`)

http_server_duration

http_server_duration

####### http_host _required_ (`string`)

http_host

http_host

####### http_user_agent _required_ (`string`)

http_user_agent

http_user_agent

####### net_transport _required_ (`string`)

net_transport

net_transport

Possible values:

- `IP.TCP`
- `IP.UDP`
- `IP.RAW`
- `IP.RDM`
- `IP.SEQPACKET`
- `Unix.TCP`
- `Unix.UDP`
- `Unix.RAW`
- `Unix.RDM`
- `Unix.SEQPACKET`

####### net_peer_ip _required_ (`string`)

net_peer_ip

net_peer_ip

####### net_peer_port _required_ (`integer`)

net_peer_port

net_peer_port

####### net_host_ip _required_ (`string`)

net_host_ip

net_host_ip

####### net_host_port _required_ (`integer`)

net_host_port

net_host_port

####### http_response_content_length _required_ (`number`)

http_response_content_length

http_response_content_length

