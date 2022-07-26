package util

import (
	"golang.org/x/sys/unix"
)

// PidSwitchNamespace switch namespace to the process specified by PID
func NamespaceSwitch(pid int) error {
	pidFd, err := unix.PidfdOpen(pid, 0)
	if err != nil {
		return err
	}
	return unix.Setns(pidFd, unix.CLONE_NEWPID|unix.CLONE_NEWNS)
}
