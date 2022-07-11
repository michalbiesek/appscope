package cmd

import (
	"bufio"
	"fmt"
	"io/ioutil"
	"os"
	"strings"
	"syscall"

	"github.com/criblio/scope/run"
	"github.com/criblio/scope/util"
	"github.com/spf13/cobra"
)

var forceFlag bool
var serviceUser string

// serviceCmd represents the service command
var serviceCmd = &cobra.Command{
	Use:     "service SERVICE [flags]",
	Short:   "Configure a systemd service to be scoped",
	Long:    `Configures the specified systemd service to be scoped upon starting.`,
	Example: `scope service cribl -c tls://in.my-instance.cribl.cloud:10090`,
	Args:    cobra.ExactArgs(1),
	Run: func(cmd *cobra.Command, args []string) {
		// must be root
		if os.Getuid() != 0 {
			util.ErrAndExit("error: must be run as root")
		}

		// service name is the first and only argument
		serviceName := args[0]

		// get uname pieces
		utsname := syscall.Utsname{}
		err := syscall.Uname(&utsname)
		util.CheckErrSprintf(err, "error: syscall.Uname failed; %v", err)
		buf := make([]byte, 0, 64)
		for _, v := range utsname.Machine[:] {
			if v == 0 {
				break
			}
			buf = append(buf, uint8(v))
		}
		unameMachine := string(buf)
		buf = make([]byte, 0, 64)
		for _, v := range utsname.Sysname[:] {
			if v == 0 {
				break
			}
			buf = append(buf, uint8(v))
		}
		unameSysname := strings.ToLower(string(buf))

		// TODO get libc name
		libcName := "gnu"

		if util.CheckDirExists("/etc/systemd") {
			// Systemd
			installSystemd(serviceName, unameMachine, unameSysname, libcName)
			os.Exit(0)
		} else if util.CheckDirExists("/etc/init.d") {
			// Init.d
			installInitd(serviceName, unameMachine, unameSysname, libcName)
			os.Exit(0)
		}

		// Unknown
		util.ErrAndExit("error: unknown boot system\n")
	},
}

func init() {
	metricAndEventDestFlags(serviceCmd, rc)
	serviceCmd.Flags().BoolVar(&forceFlag, "force", false, "Bypass confirmation prompt")
	serviceCmd.Flags().StringVarP(&serviceUser, "user", "u", "", "Specify owner username")
	RootCmd.AddCommand(serviceCmd)
}

func confirm(s string) bool {
	reader := bufio.NewReader(os.Stdin)
	for {
		fmt.Printf("%s [y/n]: ", s)
		resp, err := reader.ReadString('\n')
		util.CheckErrSprintf(err, "error: confirm failed; %v", err)
		resp = strings.ToLower(strings.TrimSpace(resp))
		if resp == "y" || resp == "yes" {
			return true
		} else if resp == "n" || resp == "no" {
			return false
		}
	}
}

func installScope(serviceName string, unameMachine string, unameSysname string, libcName string) string {
	// determine the library directory
	LibraryDirs := []string{fmt.Sprintf("/usr/lib/%s-%s-%s", unameMachine, unameSysname, libcName), "/usr/lib64", "/usr/lib"}
	libraryDir := ""
	for _, libDir := range LibraryDirs {
		if util.CheckDirExists(libDir) {
			libraryDir = libDir
			break
		}
	}

	if libraryDir == "" {
		util.ErrAndExit("error: failed to determine library directory")
	}

	// extract the library
	libraryDir = libraryDir + "/cribl"
	if !util.CheckDirExists(libraryDir) {
		err := os.Mkdir(libraryDir, 0755)
		util.CheckErrSprintf(err, "error: failed to create library directory; %v", err)
	}
	libraryPath := libraryDir + "/libscope.so"
	if !util.CheckFileExists(libraryPath) {
		asset, err := run.Asset("build/libscope.so")
		util.CheckErrSprintf(err, "error: failed to find libscope.so asset; %v", err)
		err = ioutil.WriteFile(libraryPath, asset, 0755)
		util.CheckErrSprintf(err, "error: failed to extract library; %v", err)
	}

	// patch the library
	rc.Patch(libraryPath)

	// create the config directory
	configBaseDir := "/etc/scope"
	if !util.CheckDirExists(configBaseDir) {
		err := os.Mkdir(configBaseDir, 0755)
		util.CheckErrSprintf(err, "error: failed to create config base directory; %v", err)
	}
	configDir := fmt.Sprintf("/etc/scope/%s", serviceName)
	if !util.CheckDirExists(configDir) {
		err := os.Mkdir(configDir, 0755)
		util.CheckErrSprintf(err, "error: failed to create config directory; %v", err)
	}

	// create the log directory
	logDir := "/var/log/scope"
	if !util.CheckDirExists(logDir) {
		err := os.Mkdir(logDir, 0755) // TODO chown/chgrp to who?
		util.CheckErrSprintf(err, "error: failed to create log directory; %v", err)
	}

	// create the run directory
	runDir := "/var/run/scope"
	if !util.CheckDirExists(runDir) {
		err := os.Mkdir(runDir, 0755) // TODO chown/chgrp to who?
		util.CheckErrSprintf(err, "error: failed to create run directory; %v", err)
	}

	// extract scope.yml
	configPath := fmt.Sprintf("/etc/scope/%s/scope.yml", serviceName)
	if !util.CheckFileExists(configPath) {
		asset, err := run.Asset("build/scope.yml")
		util.CheckErrSprintf(err, "error: failed to find scope.yml asset; %v", err)
		err = ioutil.WriteFile(configPath, asset, 0644)
		util.CheckErrSprintf(err, "error: failed to extract scope.yml; %v", err)
		if rc.MetricsDest != "" || rc.EventsDest != "" || rc.CriblDest != "" {
			examplePath := fmt.Sprintf("/etc/scope/%s/scope_example.yml", serviceName)
			err = os.Rename(configPath, examplePath)
			util.CheckErrSprintf(err, "error: failed to move scope.yml to scope_example.yml; %v", err)
			rc.WorkDir = configDir
			rc.SetDefault()
			sc := rc.GetScopeConfig()
			sc.Libscope.Log.Transport.Path = logDir + "/cribl.log"
			sc.Libscope.CommandDir = runDir
			err = rc.WriteScopeConfig(configPath, 0644)
			util.CheckErrSprintf(err, "error: failed to create scope.yml: %v", err)
		}
	}

	return libraryPath
}

func installSystemd(serviceName string, unameMachine string, unameSysname string, libcName string) {
	serviceDir := []string{"/etc/systemd/system/", "/lib/systemd/system/", "/run/systemd/system/", "/usr/lib/systemd/system/"}
	serviceFile := ""
	for _, v := range serviceDir {
		if util.CheckFileExists(v + serviceName + ".service") {
			serviceFile = v + serviceName + ".service"
			break
		}
	}

	if serviceFile == "" {
		util.ErrAndExit("error: didn't find service file; " + serviceName + ".service")
	}

	if !forceFlag {
		fmt.Printf("\nThis command will make the following changes if not found already:\n")
		fmt.Printf("  - install libscope.so into /usr/lib/%s-%s-%s/cribl/\n", unameMachine, unameSysname, libcName)
		fmt.Printf("  - create %s.d/env.conf override\n", serviceFile)
		fmt.Printf("  - create /etc/scope/%s/scope.yml\n", serviceName)
		fmt.Printf("  - create /var/log/scope/\n")
		fmt.Printf("  - create /var/run/scope/\n")
		if !confirm("Ready to proceed?") {
			util.ErrAndExit("info: canceled")
		}
	}

	libraryPath := installScope(serviceName, unameMachine, unameSysname, libcName)

	overrideDir := fmt.Sprintf("%s.d", serviceFile)
	if !util.CheckDirExists(overrideDir) {
		err := os.Mkdir(overrideDir, 0755) // TODO not ideal
		util.CheckErrSprintf(err, "error: failed to create override directory; %v", err)
	}
	envConfPath := fmt.Sprintf("%s.d/env.conf", serviceFile)
	if !util.CheckFileExists(envConfPath) {
		content := fmt.Sprintf("[Service]\nEnvironment=LD_PRELOAD=%s\nEnvironment=SCOPE_HOME=/etc/scope/%s\n", libraryPath, serviceName)
		err := ioutil.WriteFile(envConfPath, []byte(content), 0644)
		util.CheckErrSprintf(err, "error: failed to create override file; %v", err)
	} else {
		rawBytes, err := ioutil.ReadFile(envConfPath)
		util.CheckErrSprintf(err, "error: failed to read a override file %s; %v", envConfPath, err)
		fileContent := string(rawBytes)
		if !strings.Contains(fileContent, "Environment=SCOPE_HOME=/etc/scope") {
			content := fmt.Sprintf("[Service]\nEnvironment=LD_PRELOAD=%s\nEnvironment=SCOPE_HOME=/etc/scope/%s\n", libraryPath, serviceName)
			err := ioutil.WriteFile(envConfPath, []byte(content), 0644)
			util.CheckErrSprintf(err, "error: failed to update override file %s; %v", envConfPath, err)
		}
	}

	fmt.Printf("\nThe %s service has been updated to run with AppScope.\n", serviceName)
	fmt.Printf("\nRun `systemctl daemon-reload` to reload units.\n")
	fmt.Printf("\nPlease review the configs in /etc/scope/%s/scope.yml and check their\npermissions to ensure the scoped service can read them.\n", serviceName)
	fmt.Printf("\nAlso, please review permissions on /var/log/scope and /var/run/scope to ensure\nthe scoped service can write there.\n")
	fmt.Printf("\nRestart the service with `systemctl restart %s` so the changes take effect.\n", serviceName)
}

func installInitd(serviceName string, unameMachine string, unameSysname string, libcName string) {
	initScript := "/etc/init.d/" + serviceName
	if !util.CheckFileExists(initScript) {
		util.ErrAndExit("error: didn't find service script; " + initScript)
	}

	if !forceFlag {
		fmt.Printf("\nThis command will make the following changes if not found already:\n")
		fmt.Printf("  - install libscope.so into /usr/lib/%s-%s-%s/cribl/\n", unameMachine, unameSysname, libcName)
		fmt.Printf("  - create /etc/sysconfig/%s\n", serviceName)
		fmt.Printf("  - create /etc/scope/%s/scope.yml\n", serviceName)
		fmt.Printf("  - create /var/log/scope/\n")
		fmt.Printf("  - create /var/run/scope/\n")
		if !confirm("Ready to proceed?") {
			util.ErrAndExit("info: canceled")
		}
	}

	libraryPath := installScope(serviceName, unameMachine, unameSysname, libcName)

	if !util.CheckDirExists("/etc/sysconfig/") {
		err := os.Mkdir("/etc/sysconfig/", 0755)
		util.CheckErrSprintf(err, "error: failed to create override directory; %v", err)
	}

	sysconfigFile := "/etc/sysconfig/" + serviceName
	if !util.CheckFileExists(sysconfigFile) {
		content := fmt.Sprintf("LD_PRELOAD=%s\nSCOPE_HOME=/etc/scope/%s\n", libraryPath, serviceName)
		err := ioutil.WriteFile(sysconfigFile, []byte(content), 0644)
		util.CheckErrSprintf(err, "error: failed to create sysconfig file; %v", err)
	} else {
		rawBytes, err := ioutil.ReadFile(sysconfigFile)
		util.CheckErrSprintf(err, "error: failed to read a override file %s; %v", sysconfigFile, err)
		fileContent := string(rawBytes)
		if !strings.Contains(fileContent, "Environment=SCOPE_HOME=/etc/scope") {
			content := fmt.Sprintf("[Service]\nEnvironment=LD_PRELOAD=%s\nEnvironment=SCOPE_HOME=/etc/scope/%s\n", libraryPath, serviceName)
			err := ioutil.WriteFile(sysconfigFile, []byte(content), 0644)
			util.CheckErrSprintf(err, "error: failed to update sysconfig file %s; %v", sysconfigFile, err)
		}
	}

	fmt.Printf("\nThe %s service has been updated to run with AppScope.\n", serviceName)
	fmt.Printf("\nPlease review the configs in /etc/scope/%s/scope.yml and check their\npermissions to ensure the scoped service can read them.\n", serviceName)
	fmt.Printf("\nAlso, please review permissions on /var/log/scope and /var/run/scope to ensure\nthe scoped service can write there.\n")
	fmt.Printf("\nRestart the service with `service %s restart` so the changes take effect.\n", serviceName)
}
