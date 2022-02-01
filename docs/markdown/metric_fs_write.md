# metric_fs_write.schema (`object`)

AppScope Metric

A structure described AppScope fs.write metric message

## Properties

### type _required_ (`string`)

type

metric

### body _required_ (`object`)

body

body

#### Properties

##### _metric _required_ (`string`)

fs.write

Source - File Write

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

op_fs_write

op_fs_write

Possible values:

- `go_write`
- `pwrite64`
- `pwritev`
- `pwritev64`
- `pwritev2`
- `pwritev64v2`
- `__overflow`
- `__write_libc`
- `__write_pthread`
- `fwrite_unlocked`
- `__stdio_write`
- `write`
- `pwrite`
- `writev`
- `fwrite`
- `puts`
- `putchar`
- `fputs`
- `fputs_unlocked`
- `fputc`
- `fputc_unlocked`
- `putwc`
- `fputwc`

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

