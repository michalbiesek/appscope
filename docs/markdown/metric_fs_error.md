# metric_fs_error.schema (`object`)

AppScope Metric

A structure described AppScope fs.error metric message

## Properties

### type _required_ (`string`)

type

metric

### body _required_ (`object`)

body

body

#### Properties

##### _metric _required_ (`string`)

fs.error

Source - File Error

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

##### op _required_ (`string`)

op

op

##### file _required_ (`string`)

file

file

##### class _required_ (`string`)

class fs.error

class fs.error

Possible values:

- `open_close`
- `read_write`
- `stat`

##### unit _required_ (`string`)

operation

Unit - operation

##### _time _required_ (`number`)

_time

_time

###### Examples

`"1643662126.91777"`

