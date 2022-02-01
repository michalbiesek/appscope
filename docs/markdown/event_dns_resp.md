# event_dns_resp.schema (`object`)

AppScope Event

A structure described AppScope dns.resp event message

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

dns

Source type - dns

##### _time _required_ (`number`)

_time

_time

###### Examples

`"1643662126.91777"`

##### source _required_ (`string`)

dns.resp

Source - DNS response

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

####### duration _required_ (`number`)

duration

duration

####### domain _required_ (`string`)

domain

domain

####### addrs _required_ (`array`)

addrs

addrs

