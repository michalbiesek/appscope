# metric_http_server_duration.schema (`object`)

AppScope Metric

A structure described AppScope http.server.duration metric message

## Properties

### type _required_ (`string`)

type

metric

### body _required_ (`object`)

body

body

#### Properties

##### _metric _required_ (`string`)

http.server.duration

Source - HTTP server duration

##### _metric_type _required_ (`string`)

timer

timer

##### _value _required_ (`number`)

_value

_value

###### Examples

`1`

##### http_target _required_ (`string`)

http_target

http_target

##### numops _required_ (`number`)

numops

numops

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

##### unit _required_ (`string`)

millisecond

Unit - millisecond

##### _time _required_ (`number`)

_time

_time

###### Examples

`"1643662126.91777"`

