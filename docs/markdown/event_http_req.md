# event_http_req.schema (`object`)

AppScope Event

A structure described AppScope http.req event message

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

http.req

Source - HTTP request

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

####### http_frame (`string`)

http_frame

http_frame

Possible values:

- `HEADERS`
- `PUSH_PROMISE`

####### http_target _required_ (`string`)

http_target

http_target

####### http_flavor _required_ (`string`)

http_flavor

http_flavor

####### http_stream (`integer`)

http_stream

http_stream

####### http_scheme _required_ (`string`)

http_scheme

http_scheme

Possible values:

- `http`
- `https`

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

