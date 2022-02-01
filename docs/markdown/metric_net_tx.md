# metric_net_tx.schema (`object`)

AppScope Metric

A structure described AppScope net.tx metric message

## Properties

### type _required_ (`string`)

type

metric

### body _required_ (`object`)

body

body

#### Properties

##### _metric _required_ (`string`)

net.tx

Source - Net TX

##### _metric_type _required_ (`string`)

counter

counter

##### _value _required_ (`number`)

_value

_value

###### Examples

`1`

##### proc _required_ (`string`)

proc

proc

##### pid _required_ (`integer`)

pid

pid

###### Examples

`1000`

##### fd (`integer`)

fd

fd

###### Examples

`4`

##### host _required_ (`string`)

host

host

##### domain (`string`)

domain

domain

##### proto (`string`)

proto

proto

Possible values:

- `TCP`
- `UDP`
- `RAW`
- `RDM`
- `SEQPACKET`
- `OTHER`

##### localn (`number`)

localn

localn

##### localip (`string`)

localip

localip

##### localp (`number`)

localp

localp

##### remoten (`number`)

remoten

remoten

##### remoteip (`string`)

remoteip

remoteip

##### remotep (`number`)

remotep

remotep

##### data (`string`)

data

data

##### numops (`number`)

numops

numops

##### unit _required_ (`string`)

byte

Unit - byte

##### class (`string`)

class net.rxtx

class net.rxrx

Possible values:

- `inet_tcp`
- `inet_udp`
- `unix_tcp`
- `unix_udp`
- `other`

##### _time _required_ (`number`)

_time

_time

###### Examples

`"1643662126.91777"`

