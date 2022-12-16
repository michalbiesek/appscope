package util

import (
	"bufio"
	"errors"
	"fmt"
	"os"
	"os/user"
	"strconv"
	"strings"

	linuxproc "github.com/c9s/goprocinfo/linux"
)

// Process is a unix process
type Process struct {
	ID      int    `json:"id"`
	Pid     int    `json:"pid"`
	User    string `json:"user"`
	Scoped  bool   `json:"scoped"`
	Command string `json:"command"`
}

// PidScopeMap is a map of Pid and Scope state
type PidScopeMapState map[int]bool

// Processes is an array of Process
type Processes []Process

var (
	errOpenProc        = errors.New("cannot open proc directory")
	errReadProc        = errors.New("cannot read from proc directory")
	errGetProcStatus   = errors.New("error getting process status")
	errGetProcCmdLine  = errors.New("error getting process command line")
	errGetProcTask     = errors.New("error getting process task")
	errGetProcChildren = errors.New("error getting process children")
	errGetNsPid        = errors.New("error getting namespace PID")
	errGetProcGidMap   = errors.New("error getting process gid map")
	errGetProcUidMap   = errors.New("error getting process uid map")
	errMissingUser     = errors.New("unable to find user")
)

// searchPidByProcName check if specified inputProcName fully match the pid's process name
func searchPidByProcName(pid int, inputProcName string) bool {

	procName, err := PidCommand(pid)
	if err != nil {
		return false
	}
	return inputProcName == procName
}

// searchPidByCmdLine check if specified inputArg submatch the pid's command line
func searchPidByCmdLine(pid int, inputArg string) bool {

	cmdLine, err := PidCmdline(pid)
	if err != nil {
		return false
	}
	return strings.Contains(cmdLine, inputArg)
}

type searchFunc func(int, string) bool

// pidScopeMapSearch returns an map of processes that met conditions in searchFunc
func pidScopeMapSearch(inputArg string, sF searchFunc) (PidScopeMapState, error) {
	pidMap := make(PidScopeMapState)

	procs, err := pidProcDirsNames()
	if err != nil {
		return pidMap, err
	}

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

		if sF(pid, inputArg) {
			status, err := PidScopeStatus(pid)
			if err != nil {
				continue
			}
			pidMap[pid] = (status == Active)
		}
	}

	return pidMap, nil
}

// PidScopeMapByProcessName returns an map of processes name that are found by process name match
func PidScopeMapByProcessName(procname string) (PidScopeMapState, error) {
	return pidScopeMapSearch(procname, searchPidByProcName)
}

// PidScopeMapByCmdLine returns an map of processes that are found by cmdLine submatch
func PidScopeMapByCmdLine(cmdLine string) (PidScopeMapState, error) {
	return pidScopeMapSearch(cmdLine, searchPidByCmdLine)
}

// pidProcDirsNames returns a list wiht process directory names
func pidProcDirsNames() ([]string, error) {
	procDir, err := os.Open("/proc")
	if err != nil {
		return nil, errOpenProc
	}
	defer procDir.Close()

	return procDir.Readdirnames(0)
}

// ProcessesByNameToDetach returns an array of processes to detach that match a given name
func ProcessesByNameToDetach(name string) (Processes, error) {
	return processesByName(name, true)
}

// ProcessesByNameToAttach returns an array of processes to attach that match a given name
func ProcessesByNameToAttach(name string) (Processes, error) {
	return processesByName(name, false)
}

// processesByName returns an array of processes that match a given name
func processesByName(name string, activeOnly bool) (Processes, error) {
	processes := make([]Process, 0)

	procs, err := pidProcDirsNames()
	if err != nil {
		return processes, err
	}

	i := 1
	for _, p := range procs {
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

		status, err := PidScopeStatus(pid)
		if err != nil {
			continue
		}

		// Add process if there is a name match
		if strings.Contains(command, name) {
			if !activeOnly || (activeOnly && status == Active) {
				processes = append(processes, Process{
					ID:      i,
					Pid:     pid,
					User:    userName,
					Scoped:  status == Active,
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

	procs, err := pidProcDirsNames()
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

		cmdLine, err := PidCmdline(pid)
		if err != nil {
			continue
		}

		// TODO in container namespace we cannot depend on following info
		userName, err := PidUser(pid)
		if err != nil && !errors.Is(err, errMissingUser) {
			continue
		}

		// Add process if is is scoped
		status, err := PidScopeStatus(pid)
		if err != nil {
			continue
		}
		if status == Active {
			processes = append(processes, Process{
				ID:      i,
				Pid:     pid,
				User:    userName,
				Scoped:  true,
				Command: cmdLine,
			})
			i++
		}
	}
	return processes, nil
}

// ProcessesToDetach returns an array of processes that can be detached
func ProcessesToDetach() (Processes, error) {
	processes := make([]Process, 0)

	procs, err := pidProcDirsNames()
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

		cmdLine, err := PidCmdline(pid)
		if err != nil {
			continue
		}

		// TODO in container namespace we cannot depend on following info
		userName, err := PidUser(pid)
		if err != nil && !errors.Is(err, errMissingUser) {
			continue
		}

		status, err := PidScopeStatus(pid)
		if err != nil && !errors.Is(err, errMissingUser) {
			continue
		}
		// Add process if is is actively scoped
		if status == Active {
			processes = append(processes, Process{
				ID:      i,
				Pid:     pid,
				User:    userName,
				Scoped:  true,
				Command: cmdLine,
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

// // PidScopeStatus checks a Scope Status if a process specified by PID.
func PidScopeStatus(pid int) (ScopeStatus, error) {
	pidMapFile, err := os.ReadFile(fmt.Sprintf("/proc/%v/maps", pid))
	if err != nil {
		// Process or do not exist or we do not have permissions to read a map file
		return Disable, err
	}

	pidMap := string(pidMapFile)
	if !strings.Contains(pidMap, "libscope") {
		// Process does not contain libscope library in maps
		return Disable, nil
	}

	// Ignore ldscope process
	command, err := PidCommand(pid)
	if err != nil {
		return Disable, nil
	}

	if command == "ldscopedyn" {
		return Disable, nil
	}

	// Check shmem does not exist (if scope_anon does not exist the proc is scoped)
	files, err := os.ReadDir(fmt.Sprintf("/proc/%v/fd", pid))
	if err != nil {
		// Process or do not exist or we do not have permissions to read a fd file
		return Disable, err
	}

	for _, file := range files {
		filePath := fmt.Sprintf("/proc/%v/fd/%s", pid, file.Name())
		resolvedFileName, err := os.Readlink(filePath)
		if err != nil {
			continue
		}
		if strings.Contains(resolvedFileName, "scope_anon") {
			return Setup, nil
		}
	}

	// Retrieve information from IPC
	return getScopeStatus(pid), nil
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

// PidInitContainer verify if specific PID is the init PID in the container
func PidInitContainer(pid int) (bool, error) {
	// TODO: goprocinfo does not support all the status parameters (NsPid)
	// handle procfs by ourselves ?
	file, err := os.Open(fmt.Sprintf("/proc/%v/status", pid))
	if err != nil {
		return false, errGetProcStatus
	}
	defer file.Close()
	scanner := bufio.NewScanner(file)
	scanner.Split(bufio.ScanLines)
	for scanner.Scan() {
		line := scanner.Text()
		if strings.HasPrefix(line, "NSpid:") {
			// Skip Nspid
			nsPidString := strings.Fields(line)[1:]
			// Check for nested PID namespace and the init PID in namespace (it should be equals 1)
			if (len(nsPidString) > 1) && (nsPidString[len(nsPidString)-1] == "1") {
				return true, nil
			}
		}
	}
	return false, nil
}

// PidChildren retrieves the children PID's for the main process specified by the PID
func PidChildren(pid int) ([]int, error) {
	file, err := os.Open(fmt.Sprintf("/proc/%v/task/%v/children", pid, pid))
	if err != nil {
		return nil, errGetProcChildren
	}
	defer file.Close()

	scanner := bufio.NewScanner(file)
	scanner.Split(bufio.ScanWords)
	var childrenPids []int
	for scanner.Scan() {
		childPid, err := strconv.Atoi(scanner.Text())
		if err != nil {
			return nil, errGetProcChildren
		}
		childrenPids = append(childrenPids, childPid)
	}
	return childrenPids, nil
}

// PidThreadsPids gets the all the thread PIDs specified by PID
func PidThreadsPids(pid int) ([]int, error) {
	files, err := os.ReadDir(fmt.Sprintf("/proc/%v/task", pid))
	if err != nil {
		return nil, errGetProcTask
	}

	threadPids := make([]int, len(files))

	for _, file := range files {
		tid, _ := strconv.Atoi(file.Name())
		threadPids = append(threadPids, tid)
	}

	return threadPids, nil
}

// PidExists checks if a PID is valid
func PidExists(pid int) bool {
	pidPath := fmt.Sprintf("/proc/%v", pid)
	return CheckDirExists(pidPath)
}

// pidLastNsPid process the NsPid file for specified PID.
// Returns status if the specified PID residents in nested PID namespace, last PID in namespace and status of operation.
func pidLastNsPid(pid int) (bool, int, error) {
	// TODO: goprocinfo does not support all the status parameters (NsPid)
	// handle procfs by ourselves ?
	file, err := os.Open(fmt.Sprintf("/proc/%v/status", pid))
	if err != nil {
		return false, -1, errGetProcStatus
	}
	defer file.Close()
	scanner := bufio.NewScanner(file)
	scanner.Split(bufio.ScanLines)
	for scanner.Scan() {
		line := scanner.Text()
		if strings.HasPrefix(line, "NSpid:") {
			var nsNestedStatus bool
			// Skip Nspid
			strPids := strings.Fields(line)[1:]

			strPidsSize := len(strPids)
			if strPidsSize > 1 {
				nsNestedStatus = true
			}
			nsLastPid, _ := strconv.Atoi(strPids[strPidsSize-1])

			return nsNestedStatus, nsLastPid, nil
		}
	}
	return false, -1, errGetProcStatus
}

// pidNsTranslateUid translate specified uid to the ID-outside-ns in specified pid.
// See https://man7.org/linux/man-pages/man7/user_namespaces.7.html for details
func pidNsTranslateUid(uid int, pid int) (int, error) {
	file, err := os.Open(fmt.Sprintf("/proc/%v/uid_map", pid))
	if err != nil {
		return -1, errGetProcUidMap
	}
	defer file.Close()
	scanner := bufio.NewScanner(file)
	scanner.Split(bufio.ScanLines)
	for scanner.Scan() {
		uidMapEntry := strings.Fields(scanner.Text())
		uidInsideNs, _ := strconv.Atoi(uidMapEntry[0])
		uidOutsideNs, _ := strconv.Atoi(uidMapEntry[1])
		length, _ := strconv.Atoi(uidMapEntry[2])
		if (uid >= uidInsideNs) && (uid < uidInsideNs+length) {
			return uidOutsideNs + uid, nil
		}
	}
	// unreachable
	return -1, errGetProcUidMap
}

// pidNsTranslateGid translate specified gid to the ID-outside-ns in specified pid.
// See https://man7.org/linux/man-pages/man7/user_namespaces.7.html for details
func pidNsTranslateGid(gid int, pid int) (int, error) {
	file, err := os.Open(fmt.Sprintf("/proc/%v/gid_map", pid))
	if err != nil {
		return -1, errGetProcGidMap
	}
	defer file.Close()
	scanner := bufio.NewScanner(file)
	scanner.Split(bufio.ScanLines)
	for scanner.Scan() {
		gidMapEntry := strings.Fields(scanner.Text())
		gidInsideNs, _ := strconv.Atoi(gidMapEntry[0])
		gidOutsideNs, _ := strconv.Atoi(gidMapEntry[1])
		length, _ := strconv.Atoi(gidMapEntry[2])
		if (gid >= gidInsideNs) && (gid < gidInsideNs+length) {
			return gidOutsideNs + gid, nil
		}
	}
	// unreachable
	return -1, errGetProcGidMap
}
