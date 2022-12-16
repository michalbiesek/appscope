package util

import (
	"encoding/json"
	"errors"
	"fmt"
	"os"
	"syscall"
	"time"
)

// ipc structure representes Inter Process Communication object
type ipcObj struct {
	sender    *sendMessageQueue    // Message queue used to send messages
	receiver  *receiveMessageQueue // Message queue used to receive messages
	ipcSwitch bool                 // Indicator if IPC switch occured
}

var (
	errMissingProcMsgQueue = errors.New("missing message queue from PID")
	errRequest             = errors.New("error with sending request to PID")
	errFrame               = errors.New("frame error")
)

// ipcDispatcher dispatches request to the process specified by the pid.
// Returns the byte answer from scoped process endpoint.
func ipcDispatcher(request []byte, pid int) ([]byte, error) {
	var response []byte
	var genResp scopeGenericResponse

	ipc, err := newIPC(pid)
	if err != nil {
		return response, err
	}
	defer ipc.destroyIPC()

	err = ipc.send(request)
	if err != nil {
		return response, fmt.Errorf("%v %v", errRequest, pid)
	}

	response, err = ipc.receive()
	if err != nil {
		return response, err
	}

	// Peek out first message to figured out if the response is based on single frame
	err = json.Unmarshal(response, &genResp)
	if err != nil {
		return response, err
	}

	// If this is not partial response we are done
	if genResp.Status != responsePartialData {
		return response, err
	}

	return ipc.receiveMultiple(response)
}

// nsnewMsgQWriter creates an IPC writer structure with switching effective uid and gid
func nsnewMsgQWriter(name string, nsUid int, nsGid int, restoreUid int, restoreGid int) (*sendMessageQueue, error) {

	if err := syscall.Setegid(nsGid); err != nil {
		return nil, err
	}
	if err := syscall.Seteuid(nsUid); err != nil {
		return nil, err
	}

	sender, err := newMsgQWriter(name)
	if err != nil {
		return nil, err
	}

	if err := syscall.Seteuid(restoreUid); err != nil {
		return nil, err
	}

	if err := syscall.Setegid(restoreGid); err != nil {
		return nil, err
	}

	return sender, err
}

// nsnewMsgQReader creates an IPC reader structure with switching effective uid and gid
func nsnewMsgQReader(name string, nsUid int, nsGid int, restoreUid int, restoreGid int) (*receiveMessageQueue, error) {

	if err := syscall.Setegid(nsGid); err != nil {
		return nil, err
	}
	if err := syscall.Seteuid(nsUid); err != nil {
		return nil, err
	}

	receiver, err := newMsgQReader(name)
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
func newIPC(pid int) (*ipcObj, error) {
	restoreGid := os.Getegid()
	restoreUid := os.Geteuid()

	ipcSame, err := namespaceSameIpc(pid)
	if err != nil {
		return nil, err
	}

	// Retrieve information about user nad group id
	nsUid, err := pidNsTranslateUid(restoreUid, pid)
	if err != nil {
		return nil, err
	}
	nsGid, err := pidNsTranslateGid(restoreGid, pid)
	if err != nil {
		return nil, err
	}

	// Retrieve information about process namespace PID
	_, ipcPid, err := pidLastNsPid(pid)
	if err != nil {
		return nil, err
	}

	// Switch IPC if neeeded
	if !ipcSame {
		if err := namespaceSwitchIPC(pid); err != nil {
			return nil, err
		}
	}

	//  Create message queue to Write into it
	sender, err := nsnewMsgQWriter(fmt.Sprintf("ScopeIPCIn.%d", ipcPid), nsUid, nsGid, restoreUid, restoreGid)
	if err != nil {
		namespaceRestoreIPC()
		return nil, errMissingProcMsgQueue
	}

	// Create message queue to Read from it
	receiver, err := nsnewMsgQReader(fmt.Sprintf("ScopeIPCOut.%d", ipcPid), nsUid, nsGid, restoreUid, restoreGid)
	if err != nil {
		sender.destroy()
		namespaceRestoreIPC()
		return nil, err
	}

	return &ipcObj{sender: sender, receiver: receiver, ipcSwitch: ipcSame}, nil
}

// destroyIPC destroys an IPC object
func (ipc *ipcObj) destroyIPC() {
	ipc.sender.destroy()
	ipc.receiver.destroy()
	if ipc.ipcSwitch {
		namespaceRestoreIPC()
	}
}

// receive receive the message from the process endpoint
func (ipc *ipcObj) receive() ([]byte, error) {
	return ipc.receiver.receive(5 * time.Millisecond)
}

// send sends the message to the process endpoint
func (ipc *ipcObj) send(msg []byte) error {
	return ipc.sender.send(msg, 5*time.Millisecond)
}

// sends the message to the process endpoint
func handleFramedMsg(msg []byte, expectedId int) ([]byte, int, error) {
	var frameResp scopeDataResponseFrame

	err := json.Unmarshal(msg, &frameResp)
	if err != nil {
		return nil, -1, err
	}

	// Validate metadata
	// - status must be responsePartialData
	// - id must be equals expectedId
	// - id cannot be larger than max
	if frameResp.Status != responsePartialData {
		return nil, -1, errFrame
	}

	if frameResp.Id != expectedId {
		return nil, -1, errFrame
	}

	if frameResp.Id > frameResp.Max {
		return nil, -1, errFrame
	}

	return []byte(frameResp.Data), frameResp.Max, nil
}

// receiveMultiple receive the message from the process endpoint
func (ipc *ipcObj) receiveMultiple(firstMsg []byte) ([]byte, error) {
	var response []byte

	// Get information about first frame
	data, maxFrames, err := handleFramedMsg(firstMsg, 1)
	if err != nil {
		return nil, err
	}
	response = append(response, data...)
	for i := 2; i <= maxFrames; i++ {
		msg, err := ipc.receive()
		if err != nil {
			return response, err
		}
		data, _, err := handleFramedMsg(msg, i)
		if err != nil {
			return nil, err
		}
		response = append(response, data...)
	}

	return response, nil
}
