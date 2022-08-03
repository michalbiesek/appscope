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

	pidFd, err := os.Open(filepath.Join(procNsPath, "pid"))
	if err != nil {
		return err
	}

	mntFd, err := os.Open(filepath.Join(procNsPath, "mnt"))
	if err != nil {
		return err
	}

	if err := unix.Setns(int(pidFd.Fd()), unix.CLONE_NEWPID); err != nil {
		return fmt.Errorf("namespace PID error %w", err)
	}

	if err := unix.Setns(int(mntFd.Fd()), unix.CLONE_NEWNS); err != nil {
		return fmt.Errorf("namespace mount error %w", err)
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
