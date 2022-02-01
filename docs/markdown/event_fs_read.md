# event_fs_read.schema (`object`)

AppScope Event

A structure described AppScope fs.read event message

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

fs.read

Source - File Read

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

fs.read

Source - File Read

####### _metric_type _required_ (`string`)

histogram

histogram

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

op_fs_read

op_fs_read

Possible values:

- `go_read`
- `readdir`
- `pread64`
- `preadv`
- `preadv2`
- `preadv64v2`
- `__pread_chk`
- `__read_chk`
- `__fread_unlocked_chk`
- `read`
- `readv`
- `pread`
- `fread`
- `__fread_chk`
- `fread_unlocked`
- `fgets`
- `__fgets_chk`
- `fgets_unlocked`
- `__fgetws_chk`
- `fgetws`
- `fgetwc`
- `fgetc`
- `fscanf`
- `getline`
- `getdelim`
- `__getdelim`

####### file _required_ (`string`)

file

file

####### numops _required_ (`number`)

numops

numops

####### unit _required_ (`string`)

byte

Unit - byte

