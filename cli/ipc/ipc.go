package ipc

import (
	"errors"
	"fmt"
	"os"
	"syscall"
	"time"

	"github.com/criblio/scope/util"
)

// IPC structure representes Inter Process Communication object
type IPC struct {
	sender    *sendMessageQueue    // Message queue used to send messages
	receiver  *receiveMessageQueue // Message queue used to receive messages
	// pid       int                  // PID of scoped process from host
	ipcSwitch bool                 // Indicator if IPC switch occured
}

var (
	errMissingProcMsgQueue = errors.New("missing message queue from PID")
	errMissingResponse     = errors.New("missing response from PID")
)

// iPCDispatcher dispatches cmd to the process specified by the pid.
// Returns the byte answer from scoped process endpoint.
func iPCDispatcher(cmd IPCCmd, pid int) ([]byte, error) {
	var answer []byte
	var responseReceived bool

	ipc, err := newIPC(pid)
	if err != nil {
		return answer, err
	}
	defer ipc.destroyIPC()

	if err := ipc.send(cmd.byte()); err != nil {
		return answer, err
	}

	// TODO: Ugly hack but we need to wait for answer from process
	for i := 0; i < 5000; i++ {
		if !ipc.empty() {
			responseReceived = true
			break
		}
		time.Sleep(time.Millisecond)
	}

	// Missing response
	// The message queue on the application side exists but we are unable to receive
	// an answer from it
	if !responseReceived {
		return answer, fmt.Errorf("%v %v", errMissingResponse, pid)
	}

	return ipc.receive()
}

// nsnewNonBlockMsgQReader creates an IPC structure with switching effective uid and gid
func nsnewNonBlockMsgQReader(name string, nsUid int, nsGid int, restoreUid int, restoreGid int) (*receiveMessageQueue, error) {

	if err := syscall.Setegid(nsGid); err != nil {
		return nil, err
	}
	if err := syscall.Seteuid(nsUid); err != nil {
		return nil, err
	}

	receiver, err := newNonBlockMsgQReader(name)
	if err != nil {
		return nil, err
	}

	if err := syscall.Seteuid(restoreUid); err != nil {
		return nil, err
	}

	if err := syscall.Setegid(restoreGid); err != nil {
		return nil, err
	}

	return receiver, err
}

// newIPC creates an IPC object designated for communication with specific PID
func newIPC(pid int) (*IPC, error) {
	restoreGid := os.Getegid()
	restoreUid := os.Geteuid()

	ipcSwitch, err := util.NamespaceSelfCompareIPC(pid)
	if err != nil {
		return nil, err
	}

	// Retrieve information about user nad group id
	nsUid, err := util.PidNsTranslateUid(restoreUid, pid)
	if err != nil {
		return nil, err
	}
	nsGid, err := util.PidNsTranslateGid(restoreGid, pid)
	if err != nil {
		return nil, err
	}

	// Retrieve information about process namespace PID
	_, ipcPid, err := util.PidLastNsPid(pid)
	if err != nil {
		return nil, err
	}

	// Switch IPC if neeeded
	if ipcSwitch {
		if err := util.NamespaceSwitchIPC(pid); err != nil {
			return nil, err
		}
	}

	// Try to open proc message queue
	sender, err := openMsgQWriter(fmt.Sprintf("ScopeIPCIn.%d", ipcPid))
	if err != nil {
		util.NamespaceRestoreIPC()
		return nil, errMissingProcMsgQueue
	}

	// Try to create own message queue
	receiver, err := nsnewNonBlockMsgQReader(fmt.Sprintf("ScopeIPCOut.%d", ipcPid), nsUid, nsGid, restoreUid, restoreGid)
	if err != nil {
		sender.close()
		util.NamespaceRestoreIPC()
		return nil, err
	}

	return &IPC{sender: sender, receiver: receiver, ipcSwitch: ipcSwitch}, nil
}

// destroyIPC destroys an IPC object
func (ipc *IPC) destroyIPC() {
	ipc.sender.close()
	ipc.receiver.close()
	ipc.receiver.unlink()
	if ipc.ipcSwitch {
		util.NamespaceRestoreIPC()
	}
}

// receive receive the message from the process endpoint
func (ipc *IPC) receive() ([]byte, error) {
	return ipc.receiver.receive(0)
}

// empty checks if receiver message queque is empty
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
