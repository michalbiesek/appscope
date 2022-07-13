package cmd

import (
	"fmt"

	"github.com/criblio/scope/util"
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
		if len(args) == 0 {
			err := rc.SetDefaultFilter()
			util.CheckErrSprintf(err, "%v", err)
		} else {
			// parse configuration file
			fileName := args[0]
			if !util.CheckFileExists(fileName) {
				util.ErrAndExit("%s not exist", fileName)
			}

			rc.FilterConfigFromFile(fileName)
		}

		// open file

		// run scope service
		// Build or load config
		// if  == "" {
		// 	err := rc.configFromRunOpts()
		// 	util.CheckErrSprintf(err, "%v", err)
		// } else {
		// 	err := rc.FilterConfigFromFile()
		// 	util.CheckErrSprintf(err, "%v", err)
		// }
		// var c FilterConfig
		// c.getConf()
		// file handling check if exists etc

		fmt.Println("Lorem ipsum")
		// run scope service with force
	},
}

func init() {
	RootCmd.AddCommand(filterCmd)
}
