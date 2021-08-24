package cmd

import (
	"fmt"
	"os"
	"time"

	"github.com/criblio/scope/history"
	"github.com/criblio/scope/util"
	"github.com/kballard/go-shellquote"
	"github.com/spf13/cobra"
)

// historyCmd represents the history command
var historyCmd = &cobra.Command{
	Use:   "history [flags]",
	Short: "List scope session history",
	Long: `Each scope execution maintains a directory full of information collected by scope. History lists sessions and 
prints information on individual sessions. History displays information on what was executed, when they were 
started, how many events they output, etc.`,
	Example: `scope history                    # Displays session history
scope hist                       # Shortcut for scope history
scope hist -r                    # Displays running sessions
scope hist --id 2                # Displays detailed information for session 2
scope hist -n 50                 # Displays last 50 sessions
scope hist -d                    # Displays directory for the last session
cat $(scope hist -d)/args.json   # Outputs contents of args.json in the scope history directory for the current session`,
	Aliases: []string{"hist"},
	Args:    cobra.NoArgs,
	Run: func(cmd *cobra.Command, args []string) {
		all, _ := cmd.Flags().GetBool("all")
		last, _ := cmd.Flags().GetInt("last")
		running, _ := cmd.Flags().GetBool("running")
		id, _ := cmd.Flags().GetInt("id")
		onlydir, _ := cmd.Flags().GetBool("dir")
		sessions := history.GetSessions()
		if all {
			fmt.Println("Displaying all sessions")
		} else if id == -1 {
			if !onlydir && !running {
				fmt.Printf("Displaying last %d sessions\n", last)
			}
			sessions = sessions.Last(last)
			if onlydir {
				fmt.Printf("%s\n", sessions[len(sessions)-1].WorkDir)
				os.Exit(0)
			}
		} else {
			sessions = sessions.ID(id)
		}
		if running {
			fmt.Print("Displaying running sessions\n")
			sessions = sessions.Running()
		}
		sessions = sessions.Args()
		sessions = sessions.CountAndDuration()
		if id > -1 {
			if len(sessions) == 0 {
				util.ErrAndExit("session id %d not found", id)
			}
			session := sessions[0]
			if onlydir {
				fmt.Printf("%s\n", session.WorkDir)
				os.Exit(0)
			}
			util.PrintObj([]util.ObjField{
				{Name: "ID", Field: "id"},
				{Name: "Command", Field: "cmd"},
				{Name: "Cmdline", Field: "args", Transform: func(obj interface{}) string { return util.TruncWithEllipsis(shellquote.Join(obj.([]string)...), 25) }},
				{Name: "PID", Field: "pid"},
				{Name: "Timestamp", Field: "timestamp", Transform: func(obj interface{}) string { return time.Unix(int64(0), obj.(int64)).Format(time.Stamp) }},
				{Name: "End Timestamp", Field: "endtimestamp", Transform: func(obj interface{}) string { return time.Unix(int64(0), obj.(int64)).Format(time.Stamp) }},
				{Name: "Duration", Field: "duration", Transform: func(obj interface{}) string {
					if obj.(time.Duration) == -1 {
						return "NA"
					}
					return util.GetHumanDuration(obj.(time.Duration))
				}},
				{Name: "Total Events", Field: "eventCount", Transform: func(obj interface{}) string {
					if obj.(int) == -1 {
						return "NA"
					}
					return fmt.Sprintf("%d", obj)
				}},
				{Name: "WorkDir", Field: "workDir"},
				{Name: "ArgsPath", Field: "argspath"},
				{Name: "CmdDirPath", Field: "cmddirpath"},
				{Name: "EventsPath", Field: "eventspath"},
				{Name: "MetricsPath", Field: "metricspath"},
			}, session)
		} else {
			util.PrintObj([]util.ObjField{
				{Name: "ID", Field: "id"},
				{Name: "Command", Field: "cmd"},
				{Name: "Cmdline", Field: "args", Transform: func(obj interface{}) string { return util.TruncWithEllipsis(shellquote.Join(obj.([]string)...), 25) }},
				{Name: "PID", Field: "pid"},
				{Name: "Age", Field: "timestamp", Transform: func(obj interface{}) string { return util.GetHumanDuration(time.Since(time.Unix(0, obj.(int64)))) }},
				{Name: "Duration", Field: "duration", Transform: func(obj interface{}) string {
					if obj.(time.Duration) == -1 {
						return "NA"
					}
					return util.GetHumanDuration(obj.(time.Duration))
				}},
				{Name: "Total Events", Field: "eventCount", Transform: func(obj interface{}) string {
					if obj.(int) == -1 {
						return "NA"
					}
					return fmt.Sprintf("%d", obj)
				}},
			}, sessions)
		}
	},
}

func init() {
	historyCmd.Flags().BoolP("all", "a", false, "List all sessions")
	historyCmd.Flags().BoolP("running", "r", false, "List running sessions")
	historyCmd.Flags().IntP("last", "n", 20, "Show last <n> sessions")
	historyCmd.Flags().IntP("id", "i", -1, "Display info from specific from session ID")
	historyCmd.Flags().BoolP("dir", "d", false, "Output just directory (with -i)")
	RootCmd.AddCommand(historyCmd)
}
