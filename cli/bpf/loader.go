package bpf

import (
	"errors"
	"fmt"
	"os"
	"path"
	"path/filepath"
	"strconv"
	"strings"

	"github.com/cilium/ebpf"
	"github.com/criblio/scope/pidfd"
	"golang.org/x/sys/unix"
)

type BpfLoader struct {
	pid   int
	pidFd pidfd.PidFd
}

var (
	errReadProc      = errors.New("failed to read /proc")
	errMissingLoader = errors.New("failed to find ebpf loader")
)

// Create Bpf Loader object
func NewLoader(loaderName string) (BpfLoader, error) {
	var bl BpfLoader
	// Read the list of directories in /proc
	procDirs, err := os.ReadDir("/proc")
	if err != nil {
		return bl, errReadProc
	}

	// Iterate through the directories and check cmdline
	for _, dir := range procDirs {
		if dir.IsDir() {
			pid, err := strconv.Atoi(dir.Name())
			if err == nil {
				// Read the cmdline file for the process
				cmdline, err := os.ReadFile(fmt.Sprintf("/proc/%d/cmdline", pid))
				if err == nil {
					// Convert cmdline to string and remove null terminator
					cmdlineStr := strings.TrimSuffix(string(cmdline), "\x00")
					if strings.Contains(cmdlineStr, loaderName) {
						// This is dummy implementation there should be logic to handle multiple programs
						// Ensure that bpf_link is there
						_, err := findBpfObj(pid, "bpf_link")
						if err != nil {
							continue
						}
						_, err = findBpfObj(pid, "bpf-map")
						if err != nil {
							continue
						}
						pidFd, err := pidfd.Open(pid)
						if err != nil {
							continue
						}
						bl.pid = pid
						bl.pidFd = pidFd
						return bl, nil
					}
				}
			}
		}
	}

	return bl, errMissingLoader
}

// Terminate Bpf Loader
func (bl BpfLoader) Terminate() error {
	defer bl.pidFd.Close()
	return bl.pidFd.SendSignal(unix.SIGUSR1)
}

func (bl BpfLoader) NewSigdel() (SigDel, error) {
	var sd SigDel
	// Find the file descriptors
	linkFd, err := findBpfObj(bl.pid, "bpf_link")
	if err != nil {
		return sd, err
	}

	// TODO: handle this via bpf_link
	mapFd, err := findBpfObj(bl.pid, "bpf-map")
	if err != nil {
		return sd, err
	}

	// Copy the file descriptors
	dupLinkFd, err := bl.pidFd.GetFd(linkFd)
	if err != nil {
		return sd, err
	}

	dupMapFd, err := bl.pidFd.GetFd(mapFd)
	if err != nil {
		return sd, err
	}

	newMap, err := ebpf.NewMapFromFD(dupMapFd)
	if err != nil {
		return sd, err
	}

	sd.bpfLinkFd = dupLinkFd
	sd.bpfMap = newMap

	return sd, err
}

func findBpfObj(targetPid int, objName string) (int, error) {
	fdDir := path.Join("/proc", strconv.Itoa(targetPid), "fd")
	files, err := os.ReadDir(fdDir)
	if err != nil {
		return -1, fmt.Errorf("failed to read %s: %v", fdDir, err)
	}
	// iterate over file
	for _, file := range files {
		resolvedFileName, err := os.Readlink(filepath.Join(fdDir, file.Name()))
		if err != nil {
			fmt.Println("Readlink failed ", err)
			continue
		}
		// fmt.Println("Resolved name ", resolvedFileName)
		if strings.Contains(resolvedFileName, objName) {
			fd, err := strconv.Atoi(file.Name())
			if err != nil {
				return fd, err
			}
			return fd, nil
		}
	}
	return -1, fmt.Errorf("failed to find %v file descriptor in loader", objName)
}
