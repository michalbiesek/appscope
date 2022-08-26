package util

import (
	"os"
	"os/user"
	"strings"
	"testing"

	"github.com/stretchr/testify/assert"
)

// TestProcessesByName
// Assertions:
// - The expected process array is returned
// - No error is returned
func TestProcessesByName(t *testing.T) {
	// Current process
	name := "util.test"
	result, err := ProcessesByName(name)
	user, _ := user.Current()
	exp := Processes{
		Process{
			ID:      1,
			Pid:     os.Getpid(),
			User:    user.Username,
			Command: strings.Join(os.Args[:], " "),
			Scoped:  false,
		},
	}
	assert.Equal(t, exp, result)
	assert.NoError(t, err)
}

// TestPidUser
// Assertions:
// - The expected user value is returned
// - No error is returned
func TestPidUser(t *testing.T) {
	/*
		 * Disabled since we run as "builder" for `make docker-build`.
		 *
		// Process 1 owner
		pid := 1
		result := PidUser(pid)
		assert.Equal(t, "root", result)
	*/

	// Current process owner
	currentUser, err := user.Current()
	if err != nil {
		t.Fatal(err)
	}
	username := currentUser.Username
	pid := os.Getpid()
	result, err := PidUser(pid)
	assert.Equal(t, username, result)
	assert.NoError(t, err)
}

// TestPidCommand
// Assertions:
// - The expected command value is returned
// - No error is returned
func TestPidCommand(t *testing.T) {
	// Current process command
	pid := os.Getpid()
	result, err := PidCommand(pid)
	assert.Equal(t, "util.test", result)
	assert.NoError(t, err)
}

// TestPidCmdline
// Assertions:
// - The expected cmdline value is returned
// - No error is returned
func TestPidCmdline(t *testing.T) {
	// Current process command
	pid := os.Getpid()
	result, err := PidCmdline(pid)
	assert.Equal(t, strings.Join(os.Args[:], " "), result)
	assert.NoError(t, err)
}

// TestPidExists
// Assertions:
// - The expected boolean value is returned
func TestPidExists(t *testing.T) {
	// Current process PID
	pid := os.Getpid()
	result := PidExists(pid)
	assert.Equal(t, true, result)

	// PID 0
	pid = 0
	result = PidExists(pid)
	assert.Equal(t, false, result)
}
