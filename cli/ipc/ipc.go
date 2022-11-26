package ipc

import (
	"errors"
	"fmt"
	"time"
)

// IPC structure representes Inter Process Communication object
type IPC struct {
	sender   *sendMessageQueue    // Message queue sender
	receiver *receiveMessageQueue // Message queue receiver
}

var errMissingResponse = errors.New("missing response from PID")

// IPCDispatcher dispatches cmd to the process specified by the pid.
// Returns the string answer from process endpoint
func IPCDispatcher(cmd IPCCmd, pid int) (string, error) {
	ipc, err := newIPC(fmt.Sprintf("ScopeIPC.%d", pid))
	missResponse := true

	if err != nil {
		return "", err
	}
	defer ipc.destroyIPC()

	err = ipc.send(cmd.byte())
	if err != nil {
		return "", err
	}

	// TODO: Ugly hack but we need to wait for answer from process
	for i := 0; i < 5000; i++ {
		if !ipc.empty() {
			missResponse = false
			break
		}
		time.Sleep(time.Millisecond)
	}

	// Special case e.g. when the filter was applied the threading process is not running
	if !missResponse {
		return "", fmt.Errorf("%v %v", errMissingResponse, pid)
	}

	data, err := ipc.receive()
	return string(data), err
}

// newIPC creates an IPC structure
func newIPC(writerName string) (*IPC, error) {

	receiver, err := newNonBlockMsgQReader("ScopeCLI")
	if err != nil {
		return nil, err
	}

	sender, err := openMsgQWriter(writerName)
	if err != nil {
		return nil, err
	}

	return &IPC{sender: sender, receiver: receiver}, nil
}

// destroyIPC destroys an IPC structure
func (ipc *IPC) destroyIPC() {
	ipc.sender.close()
	ipc.receiver.close()
	ipc.receiver.unlink()
}

// receive receive the message from the process endpoint
func (ipc *IPC) receive() ([]byte, error) {
	return ipc.receiver.receive(0)
}

// empty check if receiver message queque is empty
func (ipc *IPC) empty() bool {
	atr, err := ipc.receiver.getAttributes()
	if err != nil {
		return true
	}
	return atr.CurrentMessages == 0
}

// send sends the message to the process endpoint
func (ipc *IPC) send(msg []byte) error {
	return ipc.sender.send(msg, 0)
}
