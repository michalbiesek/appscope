# metric_fs_duration.schema (`object`)

AppScope Metric

A structure described AppScope fs.duration metric message

## Properties

### type _required_ (`string`)

type

metric

### body _required_ (`object`)

body

body

#### Properties

##### _metric _required_ (`string`)

fs.duration

Source - File Duration

##### _metric_type _required_ (`string`)

histogram

histogram

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

##### op (`string`)

op

op

##### file (`string`)

file

file

##### numops (`number`)

numops

numops

##### unit _required_ (`string`)

microsecond

Unit - microsecond

##### class (`string`)

summary

summary

##### _time _required_ (`number`)

_time

_time

###### Examples

`"1643662126.91777"`

