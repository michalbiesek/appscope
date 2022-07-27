package util

import (
	"syscall"

	"golang.org/x/sys/unix"
)

// PidSwitchNamespace switch namespace to the process specified by PID
func NamespaceSwitch(pid int) error {
	pidFd, err := unix.PidfdOpen(pid, 0)
	if err != nil {
		return err
	}
	parentGid, err := GetParentGid(pid)
	if err != nil {
		return err
	}

	parentUid, err := GetParentUid(pid)
	if err != nil {
		return err
	}

	if err := unix.Setns(pidFd, unix.CLONE_NEWPID|unix.CLONE_NEWNS); err != nil {
		return err
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
