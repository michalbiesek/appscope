package cmd

import (
	"github.com/criblio/scope/util"
	"github.com/spf13/cobra"
)

// excreteCmd represents the excrete command
var excreteCmd = &cobra.Command{
	Use:     "extract [flags] <dir>",
	Aliases: []string{"excrete", "expunge", "extricate", "exorcise"},
	Short:   "Output instrumentary library files to <dir>",
	Long: `Outputs ldscope, libscope.so, and scope.yml to the provided directory. You can configure these files to instrument any application, and to output the data to any existing tool using simple TCP protocols.

The --*dest flags accept file names like /tmp/scope.log or URLs like file:///tmp/scope.log. They may also
be set to sockets with unix:///var/run/mysock, tcp://hostname:port, udp://hostname:port, or tls://hostname:port.`,
	Example: `scope extract /opt/libscope
scope extract --metricdest tcp://some.host:8125 --eventdest tcp://other.host:10070 .
`,
	Args: cobra.ExactArgs(1),
	Run: func(cmd *cobra.Command, args []string) {
		outPath := "./"
		if len(args) > 0 {
			if !util.CheckDirExists(args[0]) {
				util.ErrAndExit("%s does not exist or is not a directory", args[0])
			}
			outPath = args[0]
		}
		rc.Extract(outPath)
	},
}

func init() {
	metricAndEventDestFlags(excreteCmd, rc)
	RootCmd.AddCommand(excreteCmd)
}
