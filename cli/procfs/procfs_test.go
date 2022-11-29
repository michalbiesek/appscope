package procfs

import (
	"os"
	"strings"
	"testing"

	"github.com/stretchr/testify/assert"
)

// TestPidScopeMapByProcessName
// Assertions:
// - The process map with single entry is returned
// - No error is returned
func TestPidScopeMapByProcessName(t *testing.T) {
	currentProcessName, err := PidCommand(os.Getpid())
	assert.NoError(t, err)
	pidMap, err := PidScopeMapByProcessName(currentProcessName)
	assert.NoError(t, err)
	assert.Len(t, pidMap, 1)
	assert.Contains(t, pidMap, os.Getpid())
	assert.Equal(t, pidMap[os.Getpid()], false)
}

// TestPidScopeMapByProcessName
// Assertions:
// - Empty Map is returned
// - No error is returned
func TestPidScopeMapByProcessNameCharShorter(t *testing.T) {
	currentProcName, err := PidCommand(os.Getpid())
	assert.NoError(t, err)
	modProcName := currentProcName[:len(currentProcName)-1]

	pidMap, err := PidScopeMapByProcessName(modProcName)
	assert.NoError(t, err)
	assert.Len(t, pidMap, 0)
}

// TestPidScopeMapByCmdLine
// Assertions:
// - The expected process map is returned
// - No error is returned
func TestPidScopeMapByCmdLine(t *testing.T) {
	currentProcCmdLine, err := PidCmdline(os.Getpid())
	assert.NoError(t, err)
	pidMap, err := PidScopeMapByCmdLine(currentProcCmdLine)
	assert.NoError(t, err)
	assert.Len(t, pidMap, 1)
	assert.Contains(t, pidMap, os.Getpid())
	assert.Equal(t, pidMap[os.Getpid()], false)
}

// TestPidScopeMapByCmdLineCharShorter
// Assertions:
// - The expected process map is returned
// - No error is returned
func TestPidScopeMapByCmdLineCharShorter(t *testing.T) {
	currentProcCmdLine, err := PidCmdline(os.Getpid())
	assert.NoError(t, err)
	modProcCmdLine := currentProcCmdLine[:len(currentProcCmdLine)-1]

	pidMap, err := PidScopeMapByCmdLine(modProcCmdLine)
	assert.NoError(t, err)
	assert.Len(t, pidMap, 1)
	assert.Contains(t, pidMap, os.Getpid())
	assert.Equal(t, pidMap[os.Getpid()], false)
}

// TestPidCommand
// Assertions:
// - The expected command value is returned
// - No error is returned
func TestPidCommand(t *testing.T) {
	// Current process command
	pid := os.Getpid()
	result, err := PidCommand(pid)
	assert.Equal(t, "procfs.test", result)
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

// TestPidThreadsPids
// Assertions:
// - Current process id is one of the element in thread
// - No error is returned
func TestPidThreadsPids(t *testing.T) {
	// Current process command
	pid := os.Getpid()
	threadPids, err := PidThreadsPids(pid)
	assert.Contains(t, threadPids, pid)
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
