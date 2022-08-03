package util

import (
	"fmt"
	"os"
	"path/filepath"
	"syscall"

	"golang.org/x/sys/unix"
)

// PidSwitchNamespace switch namespace to the process specified by PID
func NamespaceSwitch(pid int) error {
	parentGid, err := GetParentGid(pid)
	if err != nil {
		return err
	}

	parentUid, err := GetParentUid(pid)
	if err != nil {
		return err
	}

	procNsPath := fmt.Sprintf("/proc/%v/ns/", pid)

	namespaces := []string{"pid", "user"}

	for _, ns := range namespaces {
		nmspcFd, err := os.Open(filepath.Join(procNsPath, ns))
		if err != nil {
			return err
		}

		if err := unix.Setns(int(nmspcFd.Fd()), 0); err != nil {
			return fmt.Errorf("%v namespace error: %w", ns, err)
		}
	}

	// The state of effective GID and UID is restored in restoreEffectiveIdStateInNamespace
	if err := syscall.Setegid(parentGid); err != nil {
		return err
	}

	if err := syscall.Seteuid(parentUid); err != nil {
		return err
	}

	return nil
}
