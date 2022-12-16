package util

// This file describes the request logic in IPC communcation
// CLI -> Library

// requestCmd describes the command passed in the IPC request to library

// IMPORTANT NOTE:
// requestCmd must be inline with library: ipc_cmd_t
type requestCmd int

const (
	cmdReqFrameCjson requestCmd = iota
	cmdReqGetScopeStatus
	cmdReqGetScopeCfg
)

// Request which contains only cmd (without data)
type scopeRequestOnly struct {
	// Unique request command
	Req requestCmd `json:"req" jsonschema:"required,const"`
}
