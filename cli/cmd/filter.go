package cmd

import (
	"errors"

	"github.com/spf13/cobra"
)

// filterCmd represents the filter command
var filterCmd = &cobra.Command{
	Use:   "filter [flags]",
	Short: "Filter a scoped process list",
	Long:  `Filter a scoped process list.`,
	Example: `
	scope filter "Lorem ipsum"
	scope filter < example_filter.yml
	scope filter --file example_filter.yml
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
		return rc.Filter(fileName)

	},
}

func init() {
	filterCmd.Flags().Bool("file", false, "Interpret the data as a file")
	RootCmd.AddCommand(filterCmd)
}
