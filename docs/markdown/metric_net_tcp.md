# metric_net_tcp.schema (`object`)

AppScope Metric

A structure described AppScope net.tcp metric message

## Properties

### type _required_ (`string`)

type

metric

### body _required_ (`object`)

body

body

#### Properties

##### _metric _required_ (`string`)

net.tcp

Source - Net tcp

##### _metric_type _required_ (`string`)

gauge

gauge

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

##### proto (`string`)

proto_tcp

proto_tcp

##### port (`number`)

port

port

##### unit _required_ (`string`)

connection

Unit - connection

##### class (`string`)

summary

summary

##### _time _required_ (`number`)

_time

_time

###### Examples

`"1643662126.91777"`

