package util

import (
	"errors"
	"fmt"
	"os"
	"os/user"
	"strconv"
	"strings"

	"github.com/criblio/scope/procfs"
)

// Process is a unix process
type Process struct {
	ID      int         `json:"id"`
	Pid     int         `json:"pid"`
	User    string      `json:"user"`
	Scoped  bool        `json:"scoped"`
	Command string      `json:"command"`
	Status  ScopeStatus `json:"status"`
}

// Processes is an array of Process
type Processes []Process

var (
	errMissingUser = errors.New("unable to find user")
)

// For detach process must be Active
func ProcessesByNameToDetach(name string) (Processes, error) {
	return processesByName(name, false)
}

// For attach process can be in all states
func ProcessesByNameToAttach(name string) (Processes, error) {
	return processesByName(name, true)
}

// processesByName returns an array of processes that match a given name
func processesByName(name string, toAttach bool) (Processes, error) {
	processes := make([]Process, 0)

	procs, err := procfs.PidProcDirs()
	if err != nil {
		return processes, err
	}

	i := 1
	for _, p := range procs {
		var status ScopeStatus = Disable
		// Skip non-integers as they are not PIDs
		if !IsNumeric(p) {
			continue
		}

		// Skip if no permission to read the fd directory
		procFdDir, err := os.Open("/proc/" + p + "/fd")
		if err != nil {
			continue
		}
		procFdDir.Close()

		// Convert directory name to integer
		pid, err := strconv.Atoi(p)
		if err != nil {
			continue
		}

		command, err := procfs.PidCommand(pid)
		if err != nil {
			continue
		}

		cmdLine, err := procfs.PidCmdline(pid)
		if err != nil {
			continue
		}

		// TODO in container namespace we cannot depend on following info
		userName, err := pidUser(pid)
		if err != nil && !errors.Is(err, errMissingUser) {
			continue
		}

		scoped := procfs.PidScoped(pid)

		if scoped {
			status = getScopeStatus(pid)
		}

		// Add process if there is a name match
		if strings.Contains(command, name) {
			if toAttach || (!toAttach && status == Active) {
				processes = append(processes, Process{
					ID:      i,
					Pid:     pid,
					User:    userName,
					Scoped:  scoped,
					Command: cmdLine,
					Status:  status,
				})
			}
			i++
		}
	}
	return processes, nil
}

// ProcessesToDetach returns an array of processes that can be detached
func ProcessesToDetach() (Processes, error) {
	processes := make([]Process, 0)

	procs, err := procfs.PidProcDirs()
	if err != nil {
		return processes, err
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

		cmdLine, err := procfs.PidCmdline(pid)
		if err != nil {
			continue
		}

		// TODO in container namespace we cannot depend on following info
		userName, err := pidUser(pid)
		if err != nil && !errors.Is(err, errMissingUser) {
			continue
		}

		if procfs.PidScoped(pid) && getScopeStatus(pid) == Active {
			processes = append(processes, Process{
				ID:      i,
				Pid:     pid,
				User:    userName,
				Scoped:  true,
				Command: cmdLine,
				Status:  Active,
			})
			i++
		}
	}
	return processes, nil

}

// ProcessesScoped returns an array of processes that are currently being scoped
func ProcessesScoped() (Processes, error) {
	processes := make([]Process, 0)

	procs, err := procfs.PidProcDirs()
	if err != nil {
		return processes, err
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

		cmdLine, err := procfs.PidCmdline(pid)
		if err != nil {
			continue
		}

		// TODO in container namespace we cannot depend on following info
		userName, err := pidUser(pid)
		if err != nil && !errors.Is(err, errMissingUser) {
			continue
		}

		// Add process if is is scoped
		scoped := procfs.PidScoped(pid)
		if scoped {
			status := getScopeStatus(pid)
			// Retrieve the status
			processes = append(processes, Process{
				ID:      i,
				Pid:     pid,
				User:    userName,
				Scoped:  scoped,
				Command: cmdLine,
				Status:  status,
			})
			i++
		}
	}
	return processes, nil
}

// pidUser returns the user owning the process specified by PID
func pidUser(pid int) (string, error) {

	// Get uid from status
	realUid, err := procfs.PidRealUid(pid)
	if err != nil {
		return "", err
	}
	// Lookup username by uid
	user, err := user.LookupId(fmt.Sprint(realUid))
	if err != nil {
		return "", errMissingUser
	}

	return user.Username, nil
}
