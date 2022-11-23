package cmd

import (
	"github.com/criblio/scope/inspect"
	"github.com/spf13/cobra"
)

// inspectCmd represents the inspect command
var inspectCmd = &cobra.Command{
	Use:     "inspect [flags]",
	Short:   "Display scope inspect",
	Long:    `Outputs inspect info.`,
	Example: `scope inspect`,
	Args:    cobra.NoArgs,
	Run: func(cmd *cobra.Command, args []string) {
		inspect.Inspect()
	},
}

func init() {
	RootCmd.AddCommand(inspectCmd)
}
