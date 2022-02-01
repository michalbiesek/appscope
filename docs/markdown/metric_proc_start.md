# metric_proc_start.schema (`object`)

AppScope Metric

A structure described AppScope proc.start metric message

## Properties

### type _required_ (`string`)

type

metric

### body _required_ (`object`)

body

body

#### Properties

##### _metric _required_ (`string`)

proc.start

Source - proc start

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

##### gid _required_ (`integer`)

gid

gid

###### Examples

`0`

##### groupname _required_ (`string`)

groupname

groupname

###### Examples

```json
"root"
```

##### uid _required_ (`integer`)

uid

uid

###### Examples

`0`

##### username _required_ (`string`)

username

username

###### Examples

```json
"root"
```

##### host _required_ (`string`)

host

host

##### args _required_ (`string`)

args

args

##### unit _required_ (`string`)

process

Unit - process

##### _time _required_ (`number`)

_time

_time

###### Examples

`"1643662126.91777"`

