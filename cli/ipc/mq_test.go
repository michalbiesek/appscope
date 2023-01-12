package ipc

import (
	"fmt"
	"golang.org/x/sys/unix"
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestSimpleMqReader(t *testing.T) {
	const mqName string = "goTestMqSimple"
	msgQUnlink(mqName)
	mqfd, err := msgQOpen(mqName, unix.O_CREAT|unix.O_RDONLY|unix.O_NONBLOCK|unix.O_EXCL)
	assert.NoError(t, err)
	for i := 0; i < 1000; i++ {
		size, err := msgQCurrentMessages(mqfd)
		assert.Zero(t, size)
		assert.NoError(t, err)
	}

	unix.Close(mqfd)
	// Always unlink even when closing fails
	msgQUnlink(mqName)
}

func TestNewMqReader(t *testing.T) {
	const mqName string = "goTestMq"

	msgQ, err := newMsgQReader(mqName)
	assert.NoError(t, err)
	assert.NotNil(t, msgQ)
	for i := 0; i < 1000; i++ {
		fmt.Println((i))
		size, err := msgQ.size()
		assert.Zero(t, size)
		assert.NoError(t, err)
	}
	err = msgQ.destroy()
	assert.NoError(t, err)
}

func TestNewMqReaderNew(t *testing.T) {
	const mqName string = "goTestMq"

	msgQ, err := CreateLinuxMQReader(mqName)
	assert.NoError(t, err)
	assert.NotNil(t, msgQ)
	for i := 0; i < 1000; i++ {
		fmt.Println((i))
		size := msgQ.Size()
		assert.Zero(t, size)
		// assert.NoError(t, err)
	}
	err = msgQ.Destroy()
	assert.NoError(t, err)
}

// func TestNewMqWriter(t *testing.T) {
// 	const mqName string = "goTestMq"

// 	msgQ, err := newMsgQWriter(mqName)
// 	assert.NoError(t, err)
// 	assert.NotNil(t, msgQ)
// 	size, err := msgQ.size()
// 	assert.Zero(t, size)
// 	assert.NoError(t, err)
// 	err = msgQ.destroy()
// 	assert.NoError(t, err)
// }

// func TestNoDuplicate(t *testing.T) {
// 	const mqNameR string = "goTestMqR"
// 	const mqNameW string = "goTestMqW"

// 	msgQW, err := newMsgQWriter(mqNameW)
// 	assert.NoError(t, err)
// 	assert.NotNil(t, msgQW)
// 	msgQWNew, err := newMsgQWriter(mqNameW)
// 	assert.Error(t, err)
// 	assert.Nil(t, msgQWNew)

// 	msgQR, err := newMsgQReader(mqNameR)
// 	assert.NoError(t, err)
// 	assert.NotNil(t, msgQR)
// 	msgQRNew, err := newMsgQReader(mqNameR)
// 	assert.Error(t, err)
// 	assert.Nil(t, msgQRNew)
// 	err = msgQR.destroy()
// 	assert.NoError(t, err)
// 	err = msgQW.destroy()
// 	assert.NoError(t, err)
// }

// func TestCommunicationMqReader(t *testing.T) {
// 	const mqName string = "goTestMqReader"
// 	byteTestMsg := []byte("Lorem Ipsum")

// 	msgQReader, err := newMsgQReader(mqName)
// 	assert.NoError(t, err)
// 	assert.NotNil(t, msgQReader)

// 	msgQWriteFd, err := msgQOpen(mqName, unix.O_WRONLY)
// 	assert.NoError(t, err)
// 	assert.NotEqual(t, msgQWriteFd, -1)

// 	// Try to read from empty queue
// 	data, err := msgQReader.receive()
// 	assert.Nil(t, data)
// 	assert.Error(t, err)

// 	// Empty message send to w
// 	err = msgQSend(msgQWriteFd, []byte(""))
// 	assert.Error(t, err)
// 	size, err := msgQReader.size()
// 	assert.NoError(t, err)
// 	assert.Zero(t, size)
// 	attr := new(messageQueueAttributes)
// 	size, err = msgQCurrentMessages(msgQWriteFd, attr)
// 	assert.NoError(t, err)
// 	assert.Zero(t, size)

// 	// Normal message send
// 	err = msgQSend(msgQWriteFd, byteTestMsg)
// 	assert.NoError(t, err)
// 	size, err = msgQReader.size()
// 	assert.NoError(t, err)
// 	assert.Equal(t, size, 1)
// 	size, err = msgQCurrentMessages(msgQWriteFd, attr)
// 	assert.NoError(t, err)
// 	assert.Equal(t, size, 1)

// 	data, err = msgQReader.receive()
// 	assert.Equal(t, data, byteTestMsg)
// 	assert.NotNil(t, data)
// 	assert.NoError(t, err)
// 	size, err = msgQReader.size()
// 	assert.NoError(t, err)
// 	assert.Zero(t, size)
// 	size, err = msgQCurrentMessages(msgQWriteFd, attr)
// 	assert.NoError(t, err)
// 	assert.Zero(t, size)

// 	err = unix.Close(msgQWriteFd)
// 	assert.NoError(t, err)

// 	err = msgQReader.destroy()
// 	assert.NoError(t, err)
// }

// func TestCommunicationMqWriter(t *testing.T) {
// 	const mqName string = "goTestMqReader"
// 	byteTestMsg := []byte("Lorem Ipsum")

// 	msgQWriter, err := newMsgQWriter(mqName)

// 	assert.NoError(t, err)
// 	assert.NotNil(t, msgQWriter)

// 	msgQReaderFd, err := msgQOpen(mqName, unix.O_RDONLY)
// 	assert.NoError(t, err)
// 	assert.NotEqual(t, msgQReaderFd, -1)

// 	// Empty message send to writer message queue
// 	err = msgQWriter.send([]byte(""))
// 	assert.Error(t, err)
// 	size, err := msgQWriter.size()
// 	assert.NoError(t, err)
// 	assert.Zero(t, size)
// 	attr := new(messageQueueAttributes)
// 	size, err = msgQCurrentMessages(msgQReaderFd, attr)
// 	assert.NoError(t, err)
// 	assert.Zero(t, size)

// 	// Normal message send
// 	err = msgQWriter.send(byteTestMsg)
// 	assert.NoError(t, err)
// 	size, err = msgQWriter.size()
// 	assert.NoError(t, err)
// 	assert.Equal(t, size, 1)
// 	size, err = msgQCurrentMessages(msgQReaderFd, attr)
// 	assert.NoError(t, err)
// 	assert.Equal(t, size, 1)

// 	data, err := msgQReceive(msgQReaderFd, msgQWriter.cap)
// 	assert.NoError(t, err)
// 	assert.NotNil(t, data)
// 	assert.Equal(t, data, byteTestMsg)
// 	size, err = msgQWriter.size()
// 	assert.NoError(t, err)
// 	assert.Zero(t, size)
// 	size, err = msgQCurrentMessages(msgQReaderFd, attr)
// 	assert.NoError(t, err)
// 	assert.Zero(t, size)

// 	err = unix.Close(msgQReaderFd)
// 	assert.NoError(t, err)

// 	err = msgQWriter.destroy()
// 	assert.NoError(t, err)
// }
