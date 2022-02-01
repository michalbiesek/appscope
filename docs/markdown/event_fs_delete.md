# event_fs_delete.schema (`object`)

AppScope Event

A structure described AppScope fs.delete event message

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

fs

Source type - fs

##### _time _required_ (`number`)

_time

_time

###### Examples

`"1643662126.91777"`

##### source _required_ (`string`)

fs.delete

Source - File Delete

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

####### proc _required_ (`string`)

proc

proc

####### pid _required_ (`integer`)

pid

pid

######## Examples

`1000`

####### host _required_ (`string`)

host

host

####### op _required_ (`string`)

op_fs_delete

op_fs_delete

Possible values:

- `unlink`
- `unlinkat`

####### file _required_ (`string`)

file

file

####### unit _required_ (`string`)

operation

Unit - operation

