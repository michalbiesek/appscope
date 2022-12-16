package cmd

import (
	"errors"
	"fmt"
	"strconv"

	"github.com/criblio/scope/internal"
	"github.com/criblio/scope/util"
	"github.com/spf13/cobra"
)

/* Args Matrix (X disallows)
 */

// infoCmd represents the info command
var infoCmd = &cobra.Command{
	Use:     "info",
	Short:   "Info about currently-running process",
	Long:    `Info a currently-running process identified by PID.`,
	Example: `scope info 1000`,

	Args: cobra.ExactArgs(1),
	Run: func(cmd *cobra.Command, args []string) {
		internal.InitConfig()
		// Nice message for non-adminstrators
		err := util.UserVerifyRootPerm()
		if errors.Is(err, util.ErrGetCurrentUser) {
			util.ErrAndExit("Unable to get current user: %v", err)
		}
		if errors.Is(err, util.ErrMissingAdmPriv) {
			fmt.Println("INFO: Run as root (or via sudo) to get info from all processes")
		}

		pid, err := strconv.Atoi(args[0])
		if err != nil {
			util.ErrAndExit("Convert fails: %v", err)
		}

		util.TestFrames(pid)
	},
}

func init() {
	RootCmd.AddCommand(infoCmd)
}
