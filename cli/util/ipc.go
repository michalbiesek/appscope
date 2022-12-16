package util

import (
	"encoding/json"
	"errors"
	"fmt"
	"os"
	"syscall"
	"time"
)

const IPC_SEND_TIMEOUT = 5 * time.Millisecond
const IPC_RECV_TIMEOUT = 5 * time.Millisecond
const METADATA_FRAME_SIZE = 35

// ipc structure representes Inter Process Communication object
type ipcObj struct {
	sender    *sendMessageQueue    // Message queue used to send messages
	receiver  *receiveMessageQueue // Message queue used to receive messages
	ipcSwitch bool                 // Indicator if IPC switch occured
}

var (
	errMissingProcMsgQueue = errors.New("missing message queue from PID")
	errRequest             = errors.New("error with sending request to PID")
	errFrameMismatchMax    = errors.New("frame error mismatch max value during transmission")
	errFrameMismatchId     = errors.New("frame error id greater than max value during transmission")
	errFrameInconsistentId = errors.New("frame error inconsistent id during transmission")
	errFrameMismatchStatus = errors.New("frame error status")
)

// ipcDispatcher dispatches request to the process specified by the pid.
// Returns the byte answer from scoped process endpoint.
func ipcDispatcher(request []byte, pid int) ([]byte, error) {
	var response []byte

	ipc, err := newIPC(pid)
	if err != nil {
		return response, err
	}
	defer ipc.destroyIPC()

	err = ipc.sendMsgQRequest(request)
	if err != nil {
		return response, fmt.Errorf("%v %v", errRequest, pid)
	}

	// Handle message queue response from application
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
	return ipc.receiver.receive(IPC_RECV_TIMEOUT)
}

// send sends the message to the process endpoint
func (ipc *ipcObj) send(msg []byte) error {
	return ipc.sender.send(msg, IPC_SEND_TIMEOUT)
}

// receives the framed message
// TODO: unify naming Framed/partial/message/packet
func receiveFramedMsg(msg []byte, expectedId int) ([]byte, int, error) {
	var frameResp msgQResponse

	err := json.Unmarshal(msg, &frameResp)
	if err != nil {
		return nil, -1, err
	}

	// Validate metadata
	// - status must be responsePartialData
	// - id must be equals expectedId
	// - id cannot be larger than max
	if frameResp.Status != responsePartialData {
		return nil, -1, errFrameMismatchStatus
	}

	if frameResp.Id != expectedId {
		return nil, -1, errFrameInconsistentId
	}

	if frameResp.Id > frameResp.Max {
		return nil, -1, errFrameMismatchId
	}

	return []byte(frameResp.Data), frameResp.Max, nil
}

// sendMsgQRequest puts the request in message queue (the message can be splited by the frames)
func (ipc *ipcObj) sendMsgQRequest(msg []byte) error {

	msgLen := len(msg)
	if ipc.sender.cap < METADATA_FRAME_SIZE {
		return errRequest
	}
	dataMaxSize := ipc.sender.cap - METADATA_FRAME_SIZE

	totalFrames := len(msg) / dataMaxSize
	if msgLen%dataMaxSize != 0 {
		totalFrames += 1
	}
	sendCounter := msgLen
	var msgOffset int

	for i := 1; i <= totalFrames; i++ {
		var dataSize int

		if sendCounter > dataMaxSize {
			sendCounter -= dataMaxSize
			dataSize = dataMaxSize
		} else if sendCounter == dataMaxSize {
			dataSize = sendCounter
		}

		frame := string(msg[msgOffset:dataSize])
		request, _ := json.Marshal(msgQRequest{Req: msgQCmdCjson, Id: i, Max: totalFrames, Data: frame})
		msgOffset += dataSize

		// Retry mechanism ?
		err := ipc.send(request)
		if err != nil {
			return err
		}

	}
	return nil
}

// receiveMultiple receive the message from the process endpoint
func (ipc *ipcObj) receiveMultiple(firstMsg []byte) ([]byte, error) {
	var response []byte

	// Get information about first frame
	data, maxFrameFirstMsg, err := receiveFramedMsg(firstMsg, 1)
	if err != nil {
		return nil, err
	}
	response = append(response, data...)
	for i := 2; i <= maxFrameFirstMsg; i++ {
		msg, err := ipc.receive()
		if err != nil {
			return response, err
		}
		data, maxFrame, err := receiveFramedMsg(msg, i)
		if err != nil {
			return nil, err
		}

		if maxFrame != maxFrameFirstMsg {
			return nil, errFrameMismatchMax
		}

		response = append(response, data...)
	}

	return response, nil
}
