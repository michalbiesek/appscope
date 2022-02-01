# metric_net_dns.schema (`object`)

AppScope Metric

A structure described AppScope net.dns metric message

## Properties

### type _required_ (`string`)

type

metric

### body _required_ (`object`)

body

body

#### Properties

##### _metric _required_ (`string`)

net.dns

Source - Net DNS

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

##### domain (`string`)

domain

domain

##### duration (`number`)

duration

duration

##### unit _required_ (`string`)

request/operation

Unit - request, operation

Possible values:

- `request`
- `operation`

##### class (`string`)

summary

summary

##### _time _required_ (`number`)

_time

_time

###### Examples

`"1643662126.91777"`

