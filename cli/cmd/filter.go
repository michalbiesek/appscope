package cmd

import (
	"github.com/spf13/cobra"
)

// filterCmd represents the filter command
var filterCmd = &cobra.Command{
	Use:     "filter [flags]",
	Short:   "Filter a scoped process list",
	Long:    `Filter a scoped process list.`,
	Example: `scope filter`,
	Args:    cobra.RangeArgs(0, 1),
	Run: func(cmd *cobra.Command, args []string) {
		fileName := args[0]
		rc.Filter(fileName)
	},
}

func init() {
	RootCmd.AddCommand(filterCmd)
}
