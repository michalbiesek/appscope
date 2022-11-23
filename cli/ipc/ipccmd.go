package ipc

// IPCCmd represents the command structure
type IPCCmd int64

const (
	CmdGetAttachStatus IPCCmd = iota
)

func (cmd IPCCmd) string() string {
	switch cmd {
	case CmdGetAttachStatus:
		return "getAttachStatus"
	}
	return "unknown"
}

func (cmd IPCCmd) byte() []byte {
	return []byte(cmd.string())
}
