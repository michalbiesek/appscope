# metric_http_request_content_length.schema (`object`)

AppScope Metric

A structure described AppScope http.request.content.length metric message

## Properties

### type _required_ (`string`)

type

metric

### body _required_ (`object`)

body

body

#### Properties

##### _metric _required_ (`string`)

http.request.content_length

Source - HTTP request content length

##### _metric_type _required_ (`string`)

counter

counter

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

byte

Unit - byte

##### _time _required_ (`number`)

_time

_time

###### Examples

`"1643662126.91777"`

