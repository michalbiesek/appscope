package util

import "fmt"

const (
	cmdGetScopeStatus uint16 = iota
)

// type ipcRequest interface {
// 	instruction() uint16
// 	data() []byte
// 	msg() []byte
// }

// Inline with ipcRequest -> ipc.c
type DefaultRequest struct {
	ins   uint16
	param uint16
}

// NewScopeStatusRequest creates a new Default Request
func NewScopeStatusRequest() DefaultRequest {
	return DefaultRequest{
		ins:   cmdGetScopeStatus,
		param: 0,
	}
}

// msg serializes request
func (req DefaultRequest) msg() []byte {
	return []byte(fmt.Sprintf("%v", req))
}
