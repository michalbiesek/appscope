package bpf

import (
	"runtime"
	"unsafe"

	"golang.org/x/sys/unix"
)

type LinkType uint32

const (
	BPF_LINK_TYPE_UNSPEC         LinkType = 0
	BPF_LINK_TYPE_RAW_TRACEPOINT LinkType = 1
	BPF_LINK_TYPE_TRACING        LinkType = 2
	BPF_LINK_TYPE_CGROUP         LinkType = 3
	BPF_LINK_TYPE_ITER           LinkType = 4
	BPF_LINK_TYPE_NETNS          LinkType = 5
	BPF_LINK_TYPE_XDP            LinkType = 6
	BPF_LINK_TYPE_PERF_EVENT     LinkType = 7
	BPF_LINK_TYPE_KPROBE_MULTI   LinkType = 8
	BPF_LINK_TYPE_STRUCT_OPS     LinkType = 9
	MAX_BPF_LINK_TYPE            LinkType = 10
)

// linkId uniquely identifies a bpf_link.
type linkId uint32

type linkInfo struct {
	Type   LinkType
	Id     linkId
	ProgId uint32
	_      [4]byte
	Extra  [16]uint8
}

type ObjName [unix.BPF_OBJ_NAME_LEN]byte

// BTFID uniquely identifies a BTF blob loaded into the kernel.
type BTFID uint32

type ProgInfo struct {
	Type                 uint32
	Id                   uint32
	Tag                  [8]uint8
	JitedProgLen         uint32
	XlatedProgLen        uint32
	JitedProgInsns       uint64
	XlatedProgInsns      Pointer
	LoadTime             uint64
	CreatedByUid         uint32
	NrMapIds             uint32
	MapIds               Pointer
	Name                 ObjName
	Ifindex              uint32
	_                    [4]byte /* unsupported bitfield */
	NetnsDev             uint64
	NetnsIno             uint64
	NrJitedKsyms         uint32
	NrJitedFuncLens      uint32
	JitedKsyms           uint64
	JitedFuncLens        uint64
	BtfId                BTFID
	FuncInfoRecSize      uint32
	FuncInfo             uint64
	NrFuncInfo           uint32
	NrLineInfo           uint32
	LineInfo             uint64
	JitedLineInfo        uint64
	NrJitedLineInfo      uint32
	LineInfoRecSize      uint32
	JitedLineInfoRecSize uint32
	NrProgTags           uint32
	ProgTags             uint64
	RunTimeNs            uint64
	RunCnt               uint64
	RecursionMisses      uint64
	VerifiedInsns        uint32
	_                    [4]byte
}

// NewPointer creates a 64-bit pointer from an unsafe Pointer.
func NewPointer(ptr unsafe.Pointer) Pointer {
	return Pointer{ptr: ptr}
}

type Info interface {
	info() (unsafe.Pointer, uint32)
}

var _ Info = (*linkInfo)(nil)

func (i *linkInfo) info() (unsafe.Pointer, uint32) {
	return unsafe.Pointer(i), uint32(unsafe.Sizeof(*i))
}

var _ Info = (*ProgInfo)(nil)

func (i *ProgInfo) info() (unsafe.Pointer, uint32) {
	return unsafe.Pointer(i), uint32(unsafe.Sizeof(*i))
}

func objInfo(fd int, info Info) error {
	ptr, len := info.info()
	err := objGetInfobyFd(&ObjGetInfoByFdAttr{
		BpfFd:   uint32(fd),
		InfoLen: len,
		Info:    NewPointer(ptr),
	})
	runtime.KeepAlive(fd)
	return err
}

type LinkStatus struct {
	Type   int
	Id     int
	ProgId int
}

func LinkGetInfo(fd int) (LinkStatus, error) {
	var info linkInfo
	var status LinkStatus
	if err := objInfo(fd, &info); err != nil {
		return status, err
	}
	status.Type = int(info.Type)
	status.Id = int(info.Id)
	status.ProgId = int(info.ProgId)

	return status, nil
}

type ProgStatus struct {
	Name string
}

func ProgGetInfo(fd int) (ProgStatus, error) {
	var info ProgInfo
	var status ProgStatus
	if err := objInfo(fd, &info); err != nil {
		return status, err
	}
	status.Name = unix.ByteSliceToString(info.Name[:])
	return status, nil
}

func ProgFdFromId(id int) (int, error) {
	return objGetProgFdbyId(&ProgGetFdByIdAttr{Id: uint32(id)})
}

func MapFdFromId(id int) (int, error) {
	return objGetMapFdbyId(&MapGetFdByIdAttr{Id: uint32(id)})
}
