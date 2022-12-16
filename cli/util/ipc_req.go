package util

// This file describes the request logic in IPC communcation
// CLI -> Library

type msgQCmd int

const (
	msgQCmdCjson msgQCmd = iota
)

// msgQ request - message which will be transferred in message queue
type msgQRequest struct {
	// Message queue request command
	Req msgQCmd `json:"req" jsonschema:"required,const"`
	// Request Sequence unique number
	Uniq int `json:"uniq" jsonschema:"required"`
	// Frame Id
	Id int `json:"id" jsonschema:"required"`
	// Frame max
	Max int `json:"max" jsonschema:"required"`
	// Frame data pass in request (scope request)
	Data string `json:"data" jsonschema:"required"`
}

// requestCmd describes the command passed in the IPC request to library

// IMPORTANT NOTE:
// requestCmd must be inline with library: ipc_cmd_t

// Real request placed in the data
type requestCmd int

const (
	cmdReqGetScopeStatus requestCmd = iota
	cmdReqGetScopeCfg
)

// Request which contains only cmd (without data)
type scopeRequestOnly struct {
	// Unique request command
	Req requestCmd `json:"req" jsonschema:"required,const"`
}
