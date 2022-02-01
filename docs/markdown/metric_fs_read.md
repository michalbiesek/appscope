# metric_fs_read.schema (`object`)

AppScope Metric

A structure described AppScope fs.read metric message

## Properties

### type _required_ (`string`)

type

metric

### body _required_ (`object`)

body

body

#### Properties

##### _metric _required_ (`string`)

fs.read

Source - File Read

##### _metric_type _required_ (`string`)

counter|histogram

counter|histogram

Possible values:

- `counter`
- `histogram`

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

##### file (`string`)

file

file

##### numops (`number`)

numops

numops

##### unit _required_ (`string`)

byte

Unit - byte

##### class (`string`)

summary

summary

##### _time _required_ (`number`)

_time

_time

###### Examples

`"1643662126.91777"`

