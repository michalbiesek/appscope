package run

import (
	"errors"
	"fmt"
	"os"
	"path/filepath"
	"runtime"
	"strconv"

	"github.com/criblio/scope/loader"
	"github.com/criblio/scope/util"
	"github.com/rs/zerolog/log"
)

var (
	ErrPrepareNamespaceHost      = errors.New("setup namespace failed on the host")
	ErrPrepareNamespaceContainer = errors.New("setup namespace failed on the container")
	ErrSwitchingNamespace        = errors.New("switching namespace failed")
	ErrSetupWorkdir              = errors.New("setup workdir failed")
	ErrPermission                = errors.New("permission issue")
	ErrAlreadyScoped             = errors.New("process is already being scoped")
	ErrInvalidPidChoice          = errors.New("invalid PID choice")
)

func checkPermissionToAttach() error {
	err := util.PermIsUserRoot()
	if err != nil {
		return err
	}
	return util.PermIsPtraceCapEnable()
}

// Attach scopes an existing PID
func (rc *Config) Attach(args []string) error {
	err := checkPermissionToAttach()
	if err != nil {
		return fmt.Errorf("attach error %v %w", ErrPermission, err)
	}

	// Get PID by name if non-numeric, otherwise validate/use args[0]
	var pid int
	if !util.IsNumeric(args[0]) {
		procs, _ := util.ProcessesByName(args[0])
		if len(procs) == 1 {
			pid = procs[0].Pid
		} else if len(procs) > 1 {
			fmt.Println("Found multiple processes matching that name...")
			pid, err = choosePid(procs)
			if err != nil {
				return fmt.Errorf("invalid Selection %w", err)
			}
		} else {
			return errors.New("no process found matching that name")
		}
		args[0] = fmt.Sprint(pid)
	} else {
		pid, err = strconv.Atoi(args[0])
		if err != nil {
			return fmt.Errorf("Invalid PID: \"%v\"", err)
		}
	}
	// Check PID exists
	if !util.PidExists(pid) {
		return fmt.Errorf("PID does not exist: \"%v\"", pid)
	}
	// Check PID is not already being scoped
	if util.PidScoped(pid) {
		return fmt.Errorf("attach error PID: %v %w ", pid, ErrAlreadyScoped)
	}

	nsPids, err := util.PidNamespacePids(pid)
	if err != nil {
		return err
	}
	env := os.Environ()
	attachMode := AttachEnableOnHost
	// Check if the process exists in the different Namespace
	namespaceNo := len(nsPids)
	if namespaceNo == 2 {
		fmt.Println("WARNING: Session history will be stored in different namespace")
		// Limit used CPU's when switching namespace
		runtime.GOMAXPROCS(1)

		// We need to read file from the host
		if rc.UserConfig != "" {
			rc.ConfigFromFile()
		}
		// After this point we are in the namespace
		if err := util.NamespaceSwitch(pid); err != nil {
			return fmt.Errorf("attach error %v %w", ErrSwitchingNamespace, err)
		}
		// Last PID
		args[0] = strconv.FormatInt(nsPids[namespaceNo-1], 10)
		rc.Subprocess = true
		attachMode = AttachEnableOnContainer
		env = append(env, "SCOPE_EXEC_PATH=/root/.scope/ldscope")
	} else if namespaceNo > 2 {
		return fmt.Errorf("attach error PID: %v AppScope currently don't support nested namespace", pid)
	}

	// Create ldscope
	if err := createLdscope(); err != nil {
		return fmt.Errorf("error creating ldscope: %v", err)
	}

	// Normal operational, not passthrough, create directory for this run
	// Directory contains scope.yml which is configured to output to that
	// directory and has a command directory configured in that directory.

	if rc.NoBreaker {
		env = append(env, "SCOPE_CRIBL_NO_BREAKER=true")
	}
	if !rc.Passthrough {
		if err := rc.setupWorkDir(args, attachMode); err != nil {
			return fmt.Errorf("Attach failed. Setup workdir failed %v", err)
		}
		env = append(env, "SCOPE_CONF_PATH="+filepath.Join(rc.WorkDir, "scope.yml"))
		log.Info().Bool("passthrough", rc.Passthrough).Strs("args", args).Msg("calling syscall.Exec")
	}
	if len(rc.LibraryPath) > 0 {
		// Validate path exists
		if !util.CheckDirExists(rc.LibraryPath) {
			return fmt.Errorf("library Path does not exist: \"%s\"", rc.LibraryPath)
		}
		// Prepend "-f" [PATH] to args
		args = append([]string{"-f", rc.LibraryPath}, args...)
	}

	ld := loader.ScopeLoader{Path: ldscopePath()}
	if !rc.Subprocess {
		return ld.Attach(args, env)
	}
	return ld.AttachSubProc(args, env)
}

// choosePid presents a user interface for selecting a PID
func choosePid(procs util.Processes) (int, error) {
	util.PrintObj([]util.ObjField{
		{Name: "ID", Field: "id"},
		{Name: "Pid", Field: "pid"},
		{Name: "User", Field: "user"},
		{Name: "Scoped", Field: "scoped"},
		{Name: "Location", Field: "location"},
		{Name: "Command", Field: "command"},
	}, procs)
	fmt.Println("Select an ID from the list:")
	var selection string
	fmt.Scanln(&selection)
	i, err := strconv.Atoi(selection)
	i--
	if err != nil || i < 0 || i >= len(procs) {
		return -1, ErrInvalidPidChoice
	}
	return procs[i].Pid, nil
}
