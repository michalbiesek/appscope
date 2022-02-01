# event_net_conn_open.schema (`object`)

AppScope Event

A structure described AppScope net.conn.open event message

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

net

Source type - net

##### _time _required_ (`number`)

_time

_time

###### Examples

`"1643662126.91777"`

##### source _required_ (`string`)

net.conn.open

Source - Net Open

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

####### net_peer_ip (`string`)

net_peer_ip

net_peer_ip

####### net_peer_port (`integer`)

net_peer_port

net_peer_port

####### net_host_ip (`string`)

net_host_ip

net_host_ip

####### net_host_port (`integer`)

net_host_port

net_host_port

####### unix_peer_inode (`number`)

unix_peer_inode

unix_peer_inode

####### unix_local_inode (`number`)

unix_local_inode

unix_local_inode

####### net_protocol (`string`)

net_protocol

net_protocol

