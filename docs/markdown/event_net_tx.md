# event_net_tx.schema (`object`)

AppScope Event

A structure described AppScope net.tx event message

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

metric

Source type - metric

##### _time _required_ (`number`)

_time

_time

###### Examples

`"1643662126.91777"`

##### source _required_ (`string`)

net.tx

Source - Net TX

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

####### _metric _required_ (`string`)

net.tx

Source - Net TX

####### _metric_type _required_ (`string`)

counter

counter

####### _value _required_ (`number`)

_value

_value

######## Examples

`1`

####### proc _required_ (`string`)

proc

proc

####### pid _required_ (`integer`)

pid

pid

######## Examples

`1000`

####### fd _required_ (`integer`)

fd

fd

######## Examples

`4`

####### domain _required_ (`string`)

domain

domain

####### proto _required_ (`string`)

proto

proto

Possible values:

- `TCP`
- `UDP`
- `RAW`
- `RDM`
- `SEQPACKET`
- `OTHER`

####### localip (`string`)

localip

localip

####### localp (`number`)

localp

localp

####### localn (`number`)

localn

localn

####### remoteip (`string`)

remoteip

remoteip

####### remotep (`number`)

remotep

remotep

####### remoten (`number`)

remoten

remoten

####### data _required_ (`string`)

data

data

Possible values:

- `ssl`
- `clear`

####### numops _required_ (`number`)

numops

numops

####### unit _required_ (`string`)

byte

Unit - byte

