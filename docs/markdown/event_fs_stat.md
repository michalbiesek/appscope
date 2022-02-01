# event_fs_stat.schema (`object`)

AppScope Event

A structure described AppScope fs.stat event message

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

fs.stat

Source - File Stat

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

fs.stat

Source - File Stat

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

####### op _required_ (`string`)

op_fs_stat

op_fs_stat

Possible values:

- `statfs64`
- `__xstat`
- `__xstat64`
- `__lxstat`
- `__lxstat64`
- `__fxstat`
- `__fxstatat`
- `__fxstatat64`
- `statx`
- `statfs`
- `statvfs`
- `statvfs64`
- `access`
- `faccessat`
- `stat`
- `lstat`
- `fstatfs64`
- `__fxstat`
- `__fxstat64`
- `fstatfs`
- `fstatvfs`
- `fstatvfs64`
- `fstat`
- `fstatat`

####### file _required_ (`string`)

file

file

####### unit _required_ (`string`)

operation

Unit - operation

