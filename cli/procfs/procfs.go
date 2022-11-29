package procfs

import (
	"bufio"
	"errors"
	"fmt"
	"os"
	"strconv"
	"strings"

	linuxproc "github.com/c9s/goprocinfo/linux"
)

var (
	errOpenProc        = errors.New("cannot open proc directory")
	errReadProc        = errors.New("cannot read from proc directory")
	errGetProcGidMap   = errors.New("error getting process gid map")
	errGetProcUidMap   = errors.New("error getting process uid map")
	errGetProcStatus   = errors.New("error getting process status")
	errGetProcCmdLine  = errors.New("error getting process command line")
	errGetProcTask     = errors.New("error getting process task")
	errGetProcChildren = errors.New("error getting process children")
)

// PidScopeMap is a map of Pid and Scope state
type PidScopeMapState map[int]bool

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

	procs, err := PidProcDirs()
	if err != nil {
		return pidMap, errReadProc
	}

	for _, p := range procs {
		// Convert directory name to integer
		pid, err := strconv.Atoi(p)
		if err != nil {
			continue
		}

		if sF(pid, inputArg) {
			pidMap[pid] = PidScoped(pid)
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

// PidProcDirs returns a list wiht process directory names
func PidProcDirs() ([]string, error) {
	procDir, err := os.Open("/proc")
	if err != nil {
		return nil, errOpenProc
	}
	defer procDir.Close()

	return procDir.Readdirnames(0)
}

// PidRealUid returns the realUid specified by PID
func PidRealUid(pid int) (uint64, error) {

	// Get uid from status
	pStat, err := linuxproc.ReadProcessStatus(fmt.Sprintf("/proc/%v/status", pid))
	if err != nil {
		return 0, errGetProcStatus
	}
	return pStat.RealUid, nil
}

// PidScoped checks if a process specified by PID contains libscope
func PidScoped(pid int) bool {

	// Look for libscope in /proc maps
	pidMapFile, err := os.ReadFile(fmt.Sprintf("/proc/%v/maps", pid))
	if err != nil {
		return false
	}
	pidMap := string(pidMapFile)
	if !strings.Contains(pidMap, "libscope") {
		return false
	}

	// Ignore ldscope process
	command, err := PidCommand(pid)
	if err != nil {
		return false
	}
	if command == "ldscopedyn" {
		return false
	}

	return true

}

// pidIsAnonPresent checks if scope_anon shared memory segment exists
func pidIsAnonPresent(pid int) bool {
	files, err := os.ReadDir(fmt.Sprintf("/proc/%v/fd", pid))
	if err != nil {
		return false
	}

	for _, file := range files {
		filePath := fmt.Sprintf("/proc/%v/fd/%s", pid, file.Name())
		resolvedFileName, err := os.Readlink(filePath)
		if err != nil {
			continue
		}
		if strings.Contains(resolvedFileName, "scope_anon") {
			return false
		}
	}
	return true
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

// PidNsTranslateUid translate specified uid to the ID-outside-ns in specified pid.
// See https://man7.org/linux/man-pages/man7/user_namespaces.7.html for details
func PidNsTranslateUid(uid int, pid int) (int, error) {
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

// PidNsTranslateGid translate specified gid to the ID-outside-ns in specified pid.
// See https://man7.org/linux/man-pages/man7/user_namespaces.7.html for details
func PidNsTranslateGid(gid int, pid int) (int, error) {
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

// PidLastNsPid process the NsPid file for specified PID.
// Returns status if the specified PID residents in nested PID namespace, last PID in namespace and status of operation.
func PidLastNsPid(pid int) (bool, int, error) {
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

// PidInitContainer verify if specific PID is the init PID in the container
func PidInitContainer(pid int) (bool, error) {

	nsNestedStatus, nsLastPid, err := PidLastNsPid(pid)
	if err != nil {
		return false, errGetProcStatus
	}

	// Check for nested PID namespace and the init PID in namespace (it should be equals 1)
	if (nsNestedStatus) && (nsLastPid == 1) {
		return true, nil
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
	stat, err := os.Stat((fmt.Sprintf("/proc/%v", pid)))
	if err != nil {
		return false
	}
	if !stat.IsDir() {
		return false
	}
	return true
}
