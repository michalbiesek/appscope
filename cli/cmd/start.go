package cmd

import (
	"bufio"
	"os"

	"github.com/spf13/cobra"
)

// getStartData reads data from Stdin
func getStartData() []byte {
	var startData []byte
	stdinFs, err := os.Stdin.Stat()

	if err != nil {
		return startData
	}

	// Verify size of data for mode other than a pipe
	if stdinFs.Size() == 0 && stdinFs.Mode()&os.ModeNamedPipe == 0 {
		return startData
	}

	stdInScanner := bufio.NewScanner(os.Stdin)
	for stdInScanner.Scan() {
		startData = append(startData, stdInScanner.Bytes()...)
	}
	return startData
}

// startCmd represents the start command
var startCmd = &cobra.Command{
	Use:   "start",
	Short: "Perform a scope start operation",
	Long: `Perform a scope start operation based on the filter input.

Following actions will be performed:
- extraction of the libscope.so to /tmp/libscope.so on the host and on the containers
- extraction of the filter inptu to /tmp/scope_fitler.yml on the host and on the containers
- setup etc/profile script to use LD_PRELOAD=/tmp/libscope.so on the host and on the containers
- setup the services which meet the allow list conditions on the host and on the containers
- attach to the processes which meet the allow list conditions on the host and on the containers`,
	Example: `
	scope start < example_filter.yml
	cat example_filter.json | scope start
	`,
	Args: cobra.NoArgs,
	RunE: func(cmd *cobra.Command, args []string) error {
		startData := getStartData()
		return rc.Start(startData)
	},
}

func init() {
	RootCmd.AddCommand(startCmd)
}
