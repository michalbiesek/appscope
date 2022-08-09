package cmd

import (
	"github.com/criblio/scope/internal"
	"github.com/spf13/cobra"
)

var controlCmd = &cobra.Command{
	Use:     "control [flags] PID | <process_name>",
	Short:   "Lorem Ipsum",
	Long:    `Lorem Ipsum.`,
	Example: `scope control -c tcp://127.0.0.1:10091`,
	Args:    cobra.ExactArgs(1),
	Run: func(cmd *cobra.Command, args []string) {
		internal.InitConfig()

		rc.Control(args)
	},
}

func init() {
	runCmdFlags(controlCmd, rc)
	RootCmd.AddCommand(controlCmd)
}
