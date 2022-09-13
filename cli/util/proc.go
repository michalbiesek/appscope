package util

import (
	"errors"
	"fmt"
	"io/ioutil"
	"os"
	"os/user"
	"strconv"
	"strings"

	linuxproc "github.com/c9s/goprocinfo/linux"
)

type ScopeState int

const (
	Unscoped ScopeState = iota
	Scoped
	Loaded
	Unknown
)

func (ss ScopeState) String() string {
	return []string{"Unscoped", "Scoped", "Loaded", "Unknown"}[ss]
}

// Process is a unix process
type Process struct {
	ID          int        `json:"id"`
	Pid         int        `json:"pid"`
	User        string     `json:"user"`
	ScopeStatus ScopeState `json:"scoped"`
	Command     string     `json:"command"`
}

// Processes is an array of Process
type Processes []Process

var (
	errOpenProc       = errors.New("cannot open proc directory")
	errReadProc       = errors.New("cannot read from proc directory")
	errGetProcStatus  = errors.New("error getting process status")
	errGetProcCmdLine = errors.New("error getting process command line")
	errMissingUser    = errors.New("unable to find user")
)

// ProcessesByName returns an array of processes that match a given name
func ProcessesByName(name string, partialMatch bool) (Processes, error) {
	processes := make([]Process, 0)

	procDir, err := os.Open("/proc")
	if err != nil {
		return processes, errOpenProc
	}
	defer procDir.Close()

	procs, err := procDir.Readdirnames(0)
	if err != nil {
		return processes, errReadProc
	}

	i := 1
	for _, p := range procs {
		// Skip non-integers as they are not PIDs
		if !IsNumeric(p) {
			continue
		}

		// Convert directory name to integer
		pid, err := strconv.Atoi(p)
		if err != nil {
			continue
		}

		command, err := PidCommand(pid)
		if err != nil {
			continue
		}

		cmdLine, err := PidCmdline(pid)
		if err != nil {
			continue
		}

		// TODO in container namespace we cannot depend on following info
		userName, err := PidUser(pid)
		if err != nil && !errors.Is(err, errMissingUser) {
			continue
		}
		if partialMatch {
			if strings.Contains(command, name) {
				processes = append(processes, Process{
					ID:      i,
					Pid:     pid,
					User:    userName,
					Scoped:  PidScoped(pid),
					Command: cmdLine,
				})
				i++
			}
		} else {
			if command == name {
				processes = append(processes, Process{
					ID:      i,
					Pid:     pid,
					User:    userName,
					Scoped:  PidScoped(pid),
					Command: cmdLine,
				})
				i++
			}
		}
	}
	return processes, nil
}

// ProcessesScoped returns an array of processes that are currently being scoped
func ProcessesScoped() (Processes, error) {
	processes := make([]Process, 0)

	procDir, err := os.Open("/proc")
	if err != nil {
		return processes, errOpenProc
	}
	defer procDir.Close()

	procs, err := procDir.Readdirnames(0)
	if err != nil {
		return processes, errReadProc
	}

	i := 1
	for _, p := range procs {
		// Skip non-integers as they are not PIDs
		if !IsNumeric(p) {
			continue
		}

		// Convert directory name to integer
		pid, err := strconv.Atoi(p)
		if err != nil {
			continue
		}

		cmdLine, err := PidCmdline(pid)
		if err != nil {
			continue
		}

		// TODO in container namespace we cannot depend on following info
		userName, err := PidUser(pid)
		if err != nil && !errors.Is(err, errMissingUser) {
			continue
		}

		// Add process if is is scoped or loaded
		scopeState := PidScopeState(pid)
		if scopeState == Scoped || scopeState == Loaded {
			processes = append(processes, Process{
				ID:          i,
				Pid:         pid,
				User:        userName,
				ScopeStatus: scopeState,
				Command:     cmdLine,
			})
			i++
		}
	}
	return processes, nil
}

// PidUser returns the user owning the process specified by PID
func PidUser(pid int) (string, error) {

	// Get uid from status
	pStat, err := linuxproc.ReadProcessStatus(fmt.Sprintf("/proc/%v/status", pid))
	if err != nil {
		return "", errGetProcStatus
	}

	// Lookup username by uid
	user, err := user.LookupId(fmt.Sprint(pStat.RealUid))
	if err != nil {
		return "", errMissingUser
	}

	return user.Username, nil
}

// PidScopeState retrieve Scope state of a process specified by PID
func PidScopeState(pid int) ScopeState {
	pidMapFile, err := ioutil.ReadFile(fmt.Sprintf("/proc/%v/maps", pid))
	if err != nil {
		return Unknown
	}

	// Look for libscope in map
	pidMap := string(pidMapFile)
	if strings.Contains(pidMap, "libscope") {
		command, err := PidCommand(pid)
		if err != nil {
			return Unknown
		}
		if command == "ldscopedyn" {
			return Unscoped
		}
		if strings.Contains(pidMap, "scope_loaded") {
			return Loaded
		}
		return Scoped
	}

	return Unscoped
}

// PidCommand gets the command used to start the process specified by PID
func PidCommand(pid int) (string, error) {
	// Get command from status
	pStat, err := linuxproc.ReadProcessStatus(fmt.Sprintf("/proc/%v/status", pid))
	if err != nil {
		return "", errGetProcStatus
	}

	return pStat.Name, nil
}

// PidCmdline gets the cmdline used to start the process specified by PID
func PidCmdline(pid int) (string, error) {
	// Get cmdline
	cmdline, err := linuxproc.ReadProcessCmdline(fmt.Sprintf("/proc/%v/cmdline", pid))
	if err != nil {
		return "", errGetProcCmdLine
	}

	return cmdline, nil
}

// PidExists checks if a PID is valid
func PidExists(pid int) bool {
	pidPath := fmt.Sprintf("/proc/%v", pid)
	if CheckDirExists(pidPath) {
		return true
	}
	return false
}
