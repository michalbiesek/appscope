# metric_http_requests.schema (`object`)

AppScope Metric

A structure described AppScope http.requests metric message

## Properties

### type _required_ (`string`)

type

metric

### body _required_ (`object`)

body

body

#### Properties

##### _metric _required_ (`string`)

http.requests

Source - HTTP requests

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

##### http_status_code _required_ (`integer`)

http_status_code

http_status_code

Possible values:

- `100`
- `101`
- `102`
- `200`
- `201`
- `202`
- `203`
- `204`
- `205`
- `206`
- `207`
- `208`
- `226`
- `300`
- `301`
- `302`
- `303`
- `304`
- `305`
- `307`
- `400`
- `401`
- `402`
- `403`
- `404`
- `405`
- `406`
- `407`
- `408`
- `409`
- `410`
- `411`
- `412`
- `413`
- `414`
- `415`
- `416`
- `417`
- `418`
- `421`
- `422`
- `423`
- `424`
- `426`
- `428`
- `429`
- `431`
- `444`
- `451`
- `499`
- `500`
- `501`
- `502`
- `503`
- `504`
- `505`
- `506`
- `507`

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

request

Unit - request

##### _time _required_ (`number`)

_time

_time

###### Examples

`"1643662126.91777"`

