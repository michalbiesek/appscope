package inspect

import (
	"fmt"

	"github.com/criblio/scope/ipc"
	"github.com/criblio/scope/util"
)

func Inspect() error {
	procs, err := util.ProcessesScoped()
	if err != nil {
		return err
	}
	if len(procs) == 0 {
		fmt.Println("No scope process were found.")
		return nil
	}

	for _, proc := range procs {
		status, err := ipc.IPCDispatcher(ipc.CmdGetAttachStatus, proc.Pid)
		if err != nil {
			fmt.Printf("\nError: %v", err)
		} else {
			fmt.Printf("\nResponse: %v", status)
		}
	}
	return err
}
