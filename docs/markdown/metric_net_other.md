# metric_net_other.schema (`object`)

AppScope Metric

A structure described AppScope net.other metric message

## Properties

### type _required_ (`string`)

type

metric

### body _required_ (`object`)

body

body

#### Properties

##### _metric _required_ (`string`)

net.other

Source - Net other

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

##### fd _required_ (`integer`)

fd

fd

###### Examples

`4`

##### host _required_ (`string`)

host

host

##### proto _required_ (`string`)

proto_other

proto_other

##### port _required_ (`number`)

port

port

##### unit _required_ (`string`)

connection

Unit - connection

##### _time _required_ (`number`)

_time

_time

###### Examples

`"1643662126.91777"`

