package cmd

import (
	"errors"

	"github.com/spf13/cobra"
)

// startCmd represents the start command
var startCmd = &cobra.Command{
	Use:   "start [flags]",
	Short: "Start a scoped process list",
	Long:  `start a scoped process list.`,
	Example: `
	scope start "Lorem ipsum"
	scope start < example_filter.yml
	scope start --file example_filter.yml
	`,
	Args: func(cmd *cobra.Command, args []string) error {
		fileFlag, _ := cmd.Flags().GetBool("file")
		if fileFlag && len(args) != 1 {
			return errors.New("requires a file argument")
		}
		return nil
	},
	RunE: func(cmd *cobra.Command, args []string) error {
		fileFlag, _ := cmd.Flags().GetBool("file")
		fileName := ""
		if fileFlag {
			fileName = args[0]
		}
		return rc.Start(fileName)

	},
}

func init() {
	startCmd.Flags().Bool("file", false, "Interpret the data as a file")
	RootCmd.AddCommand(startCmd)
}
