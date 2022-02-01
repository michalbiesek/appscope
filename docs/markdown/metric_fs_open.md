# metric_fs_open.schema (`object`)

AppScope Metric

A structure described AppScope fs.open metric message

## Properties

### type _required_ (`string`)

type

metric

### body _required_ (`object`)

body

body

#### Properties

##### _metric _required_ (`string`)

fs.open

Source - File open

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

##### op (`string`)

op_fs_open

op_fs_open

Possible values:

- `open`
- `openat`
- `opendir`
- `creat`
- `fopen`
- `freopen`
- `open64`
- `openat64`
- `__open_2`
- `__openat_2`
- `creat64`
- `fopen64`
- `freopen64`
- `recmsg`
- `console output`
- `console input`

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

