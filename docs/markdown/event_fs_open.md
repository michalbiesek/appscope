# event_fs_open.schema (`object`)

AppScope Event

A structure described AppScope fs.open event message

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

fs.open

Source - File open

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

fs.open

Source - File open

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

####### op _required_ (`string`)

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

####### file _required_ (`string`)

file

file

####### unit _required_ (`string`)

operation

Unit - operation

