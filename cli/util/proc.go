package util

import (
	"bufio"
	"errors"
	"fmt"
	"os"
	"os/user"
	"strconv"
	"strings"

	linuxproc "github.com/michalbiesek/goprocinfo/linux"
)

// Process is a unix process
type Process struct {
	ID        int    `json:"id"`
	Pid       int    `json:"pid"`
	User      string `json:"user"`
	Scoped    bool   `json:"scoped"`
	Command   string `json:"command"`
	Namespace string `json:"namespace"`
}

// Processes is an array of Process
type Processes []Process

var (
	ErrRootIdMap         = errors.New("root id not found")
	ErrGetProcCmd        = errors.New("error getting process command")
	ErrOpenProc          = errors.New("cannot open proc directory")
	ErrOpenProcMountInfo = errors.New("cannot open proc mount info")
	ErrOpenProcCgroup    = errors.New("cannot open proc cgroup")
	ErrReadProc          = errors.New("cannot read from proc directory")
)

// ProcessesByName returns an array of processes that match a given name
func ProcessesByName(name string) (Processes, error) {
	processes := make([]Process, 0)

	procDir, err := os.Open("/proc")
	if err != nil {
		return processes, ErrOpenProc
	}
	defer procDir.Close()

	procs, err := procDir.Readdirnames(0)
	if err != nil {
		return processes, ErrReadProc
	}

	i := 1
	var userName, namespace string
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

		// Add process if there is a name match
		command, err := PidCommand(pid)
		if err != nil {
			continue
		}

		cmdLine, err := PidCmdline(pid)
		if err != nil {
			continue
		}

		nsPids, err := PidNamespacePids(pid)
		if err != nil {
			continue
		}

		if len(nsPids) == 1 {
			userName, err = PidUser(pid)
			if err != nil {
				continue
			}
			namespace = "Host"
		} else {
			userName = "-"
			namespace, _ = pidNamespaceId(pid)
		}

		if strings.Contains(command, name) {
			processes = append(processes, Process{
				ID:        i,
				Pid:       pid,
				User:      userName,
				Scoped:    PidScoped(pid),
				Namespace: namespace,
				Command:   cmdLine,
			})
			i++
		}
	}
	return processes, nil
}

// ProcessesScoped returns an array of processes that are currently being scoped
func ProcessesScoped() (Processes, error) {
	processes := make([]Process, 0)

	procDir, err := os.Open("/proc")
	if err != nil {
		return processes, ErrOpenProc
	}
	defer procDir.Close()

	procs, err := procDir.Readdirnames(0)
	if err != nil {
		return processes, ErrReadProc
	}

	i := 1
	var userName, namespace string
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

		nsPids, err := PidNamespacePids(pid)
		if err != nil {
			continue
		}

		if len(nsPids) == 1 {
			userName, err = PidUser(pid)
			if err != nil {
				continue
			}
			namespace = "Host"
		} else {
			userName = "-"
			namespace, _ = pidNamespaceId(pid)
		}

		// Add process if is is scoped
		scoped := PidScoped(pid)
		if scoped {
			processes = append(processes, Process{
				ID:        i,
				Pid:       pid,
				User:      userName,
				Scoped:    scoped,
				Namespace: namespace,
				Command:   cmdLine,
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
		return "", err
	}

	// Lookup username by uid
	u, err := user.LookupId(fmt.Sprint(pStat.RealUid))
	if err != nil {
		// This can happen if the process is inside LXC container with different
		// UID mappings
		return "-", nil
	}

	return u.Username, nil
}

// PidScoped checks if a process specified by PID is being scoped
func PidScoped(pid int) bool {

	pidMapFile, err := os.ReadFile(fmt.Sprintf("/proc/%v/maps", pid))
	if err != nil {
		return false
	}

	// Look for libscope in map
	pidMap := string(pidMapFile)
	if strings.Contains(pidMap, "libscope") {
		command, err := PidCommand(pid)
		if err != nil {
			return false
		}

		return command != "ldscopedyn"
	}

	return false
}

// Get the PID's list associated with each namespace
func PidNamespacePids(pid int) ([]int64, error) {
	// Get information about nspid from status
	pStat, err := linuxproc.ReadProcessStatus(fmt.Sprintf("/proc/%v/status", pid))
	if err != nil {
		return nil, fmt.Errorf("error getting nspid: %v", err)
	}
	return pStat.NSpid, nil
}

// PidCommand gets the command used to start the process specified by PID
func PidCommand(pid int) (string, error) {
	// Get command from status
	pStat, err := linuxproc.ReadProcessStatus(fmt.Sprintf("/proc/%v/status", pid))
	if err != nil {
		return "", fmt.Errorf("%v %w", ErrGetProcCmd, err)
	}
	return pStat.Name, nil
}

// GetParentGid retrieve parent root uid for specified PID
func GetParentGid(pid int) (int, error) {
	return parrentRootIdFromMaps(fmt.Sprintf("/proc/%v/gid_map", pid))
}

// GetParentUid retrieve parent root gid for specified PID
func GetParentUid(pid int) (int, error) {
	return parrentRootIdFromMaps(fmt.Sprintf("/proc/%v/uid_map", pid))
}

// rootIdFromMaps retrieve the root id from host namespace present in the mapFile
func parrentRootIdFromMaps(mapFile string) (int, error) {
	pidMapFile, err := os.Open(mapFile)
	if err != nil {
		return -1, err
	}
	defer pidMapFile.Close()

	scanner := bufio.NewScanner(pidMapFile)

	for scanner.Scan() {
		var idNamespace, idParentNamespace int
		fmt.Sscanf(scanner.Text(), "%d %d", &idNamespace, &idParentNamespace)
		if idNamespace == 0 {
			return idParentNamespace, nil
		}
	}
	return 0, ErrRootIdMap
}

// PidCmdline gets the cmdline used to start the process specified by PID
func PidCmdline(pid int) (string, error) {
	// Get cmdline
	cmdline, err := linuxproc.ReadProcessCmdline(fmt.Sprintf("/proc/%v/cmdline", pid))
	if err != nil {
		return "", fmt.Errorf("error getting process cmdline: %v", err)
	}

	return cmdline, nil
}

// PidExists checks if a PID is valid
func PidExists(pid int) bool {
	return CheckDirExists(fmt.Sprintf("/proc/%v", pid))
}

func pidNamespaceId(pid int) (string, error) {
	cgFile, err := os.Open(fmt.Sprintf("/proc/%v/cgroup", pid))
	if err != nil {
		return "", ErrOpenProcCgroup
	}
	scanner := bufio.NewScanner(cgFile)
	for scanner.Scan() {
		// hierarchy-ID:controller-list:cgroup-path
		// 0::/lxc.payload.ubuntuLcTest/system.slice/memcached.service
		// 0::/docker/1dfef7109b0c72dd229c6dbf49657177e3134e89321968f0404afdec2d66d7b8
		cgEntry := strings.Split(scanner.Text(), ":")
		if len(cgEntry) < 3 {
			continue
		}
		hId := cgEntry[0]
		if hId == "0" {
			cgPath := cgEntry[2]
			if strings.Contains(cgPath, "/docker/") {
				return "Docker container", nil
			}
			if strings.Contains(cgPath, "/lxc") {
				return "LXC container", nil
			}
		}

	}

	return "Unknown Container", nil
}
