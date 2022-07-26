package util

import (
	"errors"
	"fmt"
	"os/user"

	"github.com/syndtr/gocapability/capability"
)

var (
	ErrCurrentUser      = errors.New("unable to get current user")
	ErrNonRoot          = errors.New("missing administrator privileges")
	ErrCapGet           = errors.New("unable to get linux capabilities for current process")
	ErrCapLoad          = errors.New("unable to load linux capabilities")
	ErrCapPtraceMissing = errors.New("you must have CAP_SYS_PTRACE capabilities to attach to a process")
)

// Check if current user is a root
func PermIsUserRoot() error {
	user, err := user.Current()
	if err != nil {
		return fmt.Errorf("%v %w", ErrCurrentUser, err)
	}
	if user.Uid != "0" {
		return ErrNonRoot
	}

	return nil
}

// Check if current proccess got ptrace capabilities
func PermIsPtraceCapEnable() error {
	c, err := capability.NewPid2(0)
	if err != nil {
		return ErrCapGet
	}
	if err := c.Load(); err != nil {
		return ErrCapLoad
	}
	if !c.Get(capability.EFFECTIVE, capability.CAP_SYS_PTRACE) {
		return ErrCapPtraceMissing
	}

	return nil
}
