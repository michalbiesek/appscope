package cmd

import (
	"fmt"

	"github.com/spf13/cobra"
)

// filterCmd represents the filter command
var filterCmd = &cobra.Command{
	Use:     "filter [flags]",
	Short:   "Filter a scoped process list",
	Long:    `Filter a scoped process list.`,
	Example: `scope filter`,
	Args:    cobra.NoArgs,
	Run: func(cmd *cobra.Command, args []string) {
		fmt.Printf("Filter command: Lorem ipsum")
	},
}

func init() {
	RootCmd.AddCommand(filterCmd)
}
