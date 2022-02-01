# event_fs_error.schema (`object`)

AppScope Event

A structure described AppScope fs.error event message

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

fs.error

Source - File Error

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

fs.error

Source - File Error

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

op

op

####### file _required_ (`string`)

file

file

####### class _required_ (`string`)

class fs.error

class fs.error

Possible values:

- `open_close`
- `read_write`
- `stat`

####### unit _required_ (`string`)

operation

Unit - operation

