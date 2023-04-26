package bpf

import (
	"fmt"
	"runtime"
	"unsafe"

	"golang.org/x/sys/unix"
)

// Pointer wraps an unsafe.Pointer to be 64bit to
// conform to the syscall specification.
type Pointer struct {
	ptr unsafe.Pointer
}

// pidfd, process file descriptor
type bpfCmd int

const BPF_PROG_GET_FD_BY_ID bpfCmd = 13
const BPF_MAP_GET_FD_BY_ID bpfCmd = 14
const BPF_OBJ_GET_INFO_BY_FD bpfCmd = 15

// unixBbpf is wrapper to BPF syscall
func unixBbpf(cmd bpfCmd, attr unsafe.Pointer, size uintptr) (uintptr, error) {
	r1, _, errno := unix.Syscall(unix.SYS_BPF, uintptr(cmd), uintptr(attr), size)
	runtime.KeepAlive(attr)
	if errno != 0 {
		return r1, fmt.Errorf("SYS_BPF failed: %w", errno)
	}

	return r1, nil
}

type ObjGetInfoByFdAttr struct {
	BpfFd   uint32
	InfoLen uint32
	Info    Pointer
}

func objGetInfobyFd(attr *ObjGetInfoByFdAttr) error {
	_, err := unixBbpf(BPF_OBJ_GET_INFO_BY_FD, unsafe.Pointer(attr), unsafe.Sizeof(*attr))
	return err
}

type ProgGetFdByIdAttr struct {
	Id uint32
}

func objGetProgFdbyId(attr *ProgGetFdByIdAttr) (int, error) {
	fd, err := unixBbpf(BPF_PROG_GET_FD_BY_ID, unsafe.Pointer(attr), unsafe.Sizeof(*attr))
	return int(fd), err
}

type MapGetFdByIdAttr struct {
	Id uint32
}

func objGetMapFdbyId(attr *MapGetFdByIdAttr) (int, error) {
	fd, err := unixBbpf(BPF_MAP_GET_FD_BY_ID, unsafe.Pointer(attr), unsafe.Sizeof(*attr))
	return int(fd), err
}
