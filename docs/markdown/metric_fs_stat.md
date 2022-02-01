# metric_fs_stat.schema (`object`)

AppScope Metric

A structure described AppScope fs.stat metric message

## Properties

### type _required_ (`string`)

type

metric

### body _required_ (`object`)

body

body

#### Properties

##### _metric _required_ (`string`)

fs.stat

Source - File Stat

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

##### host _required_ (`string`)

host

host

##### op (`string`)

op

op

##### file (`string`)

file

file

##### unit _required_ (`string`)

operation

Unit - operation

##### class (`string`)

summary

summary

##### _time _required_ (`number`)

_time

_time

###### Examples

`"1643662126.91777"`

