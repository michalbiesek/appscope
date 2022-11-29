package ipc

// IPCCmd represents the command structure
type IPCCmd int64

const (
	cmdGetScopeStatus IPCCmd = iota
)

func (cmd IPCCmd) string() string {
	switch cmd {
	case cmdGetScopeStatus:
		return "getScopeStatus"
	}
	return "unknown"
}

func (cmd IPCCmd) byte() []byte {
	return []byte(cmd.string())
}
