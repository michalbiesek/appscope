# metric_proc_mem.schema (`object`)

AppScope Metric

A structure described AppScope proc.mem metric message

## Properties

### type _required_ (`string`)

type

metric

### body _required_ (`object`)

body

body

#### Properties

##### _metric _required_ (`string`)

proc.mem

Source - proc memory

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

##### host _required_ (`string`)

host

host

##### unit _required_ (`string`)

kibibyte

Unit - kibibyte

##### _time _required_ (`number`)

_time

_time

###### Examples

`"1643662126.91777"`

