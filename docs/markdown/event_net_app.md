# event_net_app.schema (`object`)

AppScope Event

A structure described AppScope net.app event message

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

net.app

Source - Net App

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

####### host _required_ (`string`)

host

host

####### protocol _required_ (`string`)

protocol

protocol

Possible values:

- `HTTP`

