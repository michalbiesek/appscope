# metric_dns_duration.schema (`object`)

AppScope Metric

A structure described AppScope dns.duration metric message

## Properties

### type _required_ (`string`)

type

metric

### body _required_ (`object`)

body

body

#### Properties

##### _metric _required_ (`string`)

dns.duration

Source - DNS duration

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

##### host _required_ (`string`)

host

host

##### domain _required_ (`string`)

domain

domain

##### numops _required_ (`number`)

numops

numops

##### unit _required_ (`string`)

millisecond

Unit - millisecond

##### _time _required_ (`number`)

_time

_time

###### Examples

`"1643662126.91777"`

