package run

import (
	"bufio"
	"errors"
	"fmt"
	"os"
	"path"
	"strings"
	"syscall"

	"github.com/criblio/scope/loader"
	"github.com/criblio/scope/util"
)

var (
	errRootPerm       = errors.New("error: must be run as root")
	errUname          = errors.New("error: syscall.Uname failed")
	errMissingService = errors.New("error: didn't find service file")
)

func getLibraryDir() (string, error) {
	// get uname pieces
	utsname := syscall.Utsname{}
	if err := syscall.Uname(&utsname); err != nil {
		return "", errUname
	}
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
	return fmt.Sprintf("/usr/lib/%s-%s-gnu/cribl", unameMachine, unameSysname), nil
}

func (rc *Config) Service(name string, user string, force bool, quiet bool) error {
	// TODO: move it to separate package
	// must be root
	if os.Getuid() != 0 {
		return errRootPerm
	}

	if util.CheckFileExists("/etc/rc.conf") {
		// OpenRC
		return rc.installOpenRc(name, user, force, quiet)
	} else if util.CheckDirExists("/etc/systemd") {
		// Systemd
		return rc.installSystemd(name, user, force, quiet)
	} else if util.CheckDirExists("/etc/init.d") {
		// Initd
		return rc.installInitd(name, user, force, quiet)
	}
	return errors.New("error: unknown boot system")
}

func confirm(s string) (bool, error) {
	reader := bufio.NewReader(os.Stdin)
	for {
		fmt.Printf("%s [y/n]: ", s)
		resp, err := reader.ReadString('\n')
		if err != nil {
			return false, fmt.Errorf("error: confirm failed; %v", err)
		}
		resp = strings.ToLower(strings.TrimSpace(resp))
		if resp == "y" || resp == "yes" {
			return true, nil
		} else if resp == "n" || resp == "no" {
			return false, nil
		}
	}
}

func (rc *Config) installScope(serviceName string, libraryDir string) (string, error) {
	// determine the library directory
	var libraryPath string
	if !util.CheckDirExists(libraryDir) {
		if err := os.MkdirAll(libraryDir, 0755); err != nil {
			return libraryPath, fmt.Errorf("error: failed to create library directory; %v", err)
		}
	}

	libraryPath = libraryDir + "/libscope.so"
	if !util.CheckFileExists(libraryPath) {
		asset, err := Asset("build/libscope.so")
		if err != nil {
			return libraryPath, fmt.Errorf("error: failed to find libscope.so asset; %v", err)
		}
		if err := os.WriteFile(libraryPath, asset, 0755); err != nil {
			return libraryPath, fmt.Errorf("error: failed to extract library; %v", err)
		}
	}

	// patch the library
	asset, err := Asset("build/ldscope")
	if err != nil {
		return libraryPath, fmt.Errorf("error: failed to find ldscope asset; %v", err)
	}
	loaderPath := path.Join(libraryDir, "ldscope")
	if err := os.WriteFile(loaderPath, asset, 0755); err != nil {
		return libraryPath, fmt.Errorf("error: failed to extract loader; %v", err)
	}

	ld := loader.ScopeLoader{Path: loaderPath}
	ld.Patch(path.Join(libraryDir, "libscope.so"))
	if err := os.Remove(loaderPath); err != nil {
		return libraryPath, fmt.Errorf("error: failed to remove loader; %v", err)
	}
	// create the config directory
	configDir := fmt.Sprintf("/etc/scope/%s", serviceName)
	if !util.CheckDirExists(configDir) {
		if err := os.MkdirAll(configDir, 0755); err != nil {
			return libraryPath, fmt.Errorf("error: failed to create config directory; %v", err)
		}
	}

	// create the log directory
	logDir := "/var/log/scope"
	if !util.CheckDirExists(logDir) {
		// TODO chown/chgrp to who?
		if err := os.Mkdir(logDir, 0755); err != nil {
			return libraryPath, fmt.Errorf("error: failed to create log directory; %v", err)
		}
	}

	// create the run directory
	runDir := "/var/run/scope"
	if !util.CheckDirExists(runDir) {
		// TODO chown/chgrp to who?
		if err := os.Mkdir(runDir, 0755); err != nil {
			return libraryPath, fmt.Errorf("error: failed to create run directory; %v", err)
		}
	}

	// extract scope.yml
	configPath := fmt.Sprintf("/etc/scope/%s/scope.yml", serviceName)
	asset, err = Asset("build/scope.yml")
	if err != nil {
		return libraryPath, fmt.Errorf("error: failed to find scope.yml asset; %v", err)
	}

	if os.WriteFile(configPath, asset, 0644); err != nil {
		return libraryPath, fmt.Errorf("error: failed to extract scope.yml; %v", err)
	}
	examplePath := fmt.Sprintf("/etc/scope/%s/scope_example.yml", serviceName)

	if err = os.Rename(configPath, examplePath); err != nil {
		return libraryPath, fmt.Errorf("error: failed to move scope.yml to scope_example.yml; %v", err)
	}
	rc.WorkDir = configDir
	rc.CommandDir = runDir
	rc.LogDest = "file://" + logDir + "/cribl.log"

	if err := rc.WriteScopeConfig(configPath, 0644); err != nil {
		return libraryPath, fmt.Errorf("error: failed to create scope.yml: %v", err)
	}

	return libraryPath, nil
}

func locateService(serviceName string) bool {
	servicePrefixDirs := []string{"/etc/systemd/system/", "/lib/systemd/system/", "/run/systemd/system/", "/usr/lib/systemd/system/"}
	for _, prefixDir := range servicePrefixDirs {
		if util.CheckFileExists(prefixDir + serviceName + ".service") {
			return true
		}
	}
	return false
}

func (rc *Config) installSystemd(name string, user string, force bool, quiet bool) error {
	libraryDir, err := getLibraryDir()
	if err != nil {
		return err
	}

	serviceDir := "/etc/systemd/system/" + name + ".service.d"
	if !locateService(name) {
		return errMissingService
	}
	if !util.CheckDirExists(serviceDir) {
		if err := os.MkdirAll(serviceDir, 0655); err != nil {
			return fmt.Errorf("error: failed to create serviceDir directory; %v", err)
		}
	}

	serviceEnvPath := serviceDir + "/env.conf"

	if !force {
		fmt.Printf("\nThis command will make the following changes if not found already:\n")
		fmt.Printf("  - install libscope.so into %s\n", libraryDir)
		fmt.Printf("  - create/update %s override\n", serviceEnvPath)
		fmt.Printf("  - create /etc/scope/%s/scope.yml\n", name)
		fmt.Printf("  - create /var/log/scope/\n")
		fmt.Printf("  - create /var/run/scope/\n")
		status, err := confirm("Ready to proceed?")
		if err != nil {
			return err
		} else if !status {
			return errors.New("info: canceled")
		}
	}

	libraryPath, err := rc.installScope(name, libraryDir)
	if err != nil {
		return fmt.Errorf("error: failed to install Scope; %v", err)
	}

	if !util.CheckFileExists(serviceEnvPath) {
		// Create env.conf
		content := fmt.Sprintf("[Service]\nEnvironment=LD_PRELOAD=%s\nEnvironment=SCOPE_HOME=/etc/scope/%s\n", libraryPath, name)
		if err := os.WriteFile(serviceEnvPath, []byte(content), 0644); err != nil {
			return fmt.Errorf("error: failed to create override file; %v", err)
		}
	} else {
		// Modify env.conf
		data, err := os.ReadFile(serviceEnvPath)
		if err != nil {
			return fmt.Errorf("error: failed to read override file; %v", err)
		}
		strData := string(data)
		if !strings.Contains(strData, "[Service]\n") {
			// Create Service section
			content := fmt.Sprintf("[Service]\nEnvironment=LD_PRELOAD=%s\nEnvironment=SCOPE_HOME=/etc/scope/%s\n", libraryPath, name)
			f, err := os.OpenFile(serviceEnvPath, os.O_APPEND|os.O_WRONLY, 0644)
			if err != nil {
				return fmt.Errorf("error: failed to open sysconfig file; %v", err)
			}
			defer f.Close()
			if _, err := f.WriteString(content); err != nil {
				return fmt.Errorf("error: failed to update override file; %v", err)
			}
		} else {
			// Update Service section
			oldData := "[Service]\n"
			if !strings.Contains(strData, "Environment=LD_PRELOAD=") {
				newData := oldData + fmt.Sprintf("Environment=LD_PRELOAD=%s\n", libraryPath)
				strData = strings.Replace(strData, oldData, newData, 1)
				oldData = newData
			}
			if !strings.Contains(strData, "Environment=SCOPE_HOME=") {
				newData := oldData + fmt.Sprintf("Environment=SCOPE_HOME=/etc/scope/%s\n", name)
				strData = strings.Replace(strData, oldData, newData, 1)
			}
			err := os.WriteFile(serviceEnvPath, []byte(strData), 0644)
			if err != nil {
				return fmt.Errorf("error: failed to create override file; %v", err)
			}
		}
	}
	if !quiet {
		fmt.Printf("\nThe %s service has been updated to run with AppScope.\n", name)
		fmt.Printf("\nRun `systemctl daemon-reload` to reload units.\n")
		fmt.Printf("\nPlease review the configs in /etc/scope/%s/scope.yml and check their\npermissions to ensure the scoped service can read them.\n", name)
		fmt.Printf("\nAlso, please review permissions on /var/log/scope and /var/run/scope to ensure\nthe scoped service can write there.\n")
		fmt.Printf("\nRestart the service with `systemctl restart %s` so the changes take effect.\n", name)
	}
	return nil
}

func (rc *Config) installInitd(name string, user string, force bool, quiet bool) error {
	libraryDir, err := getLibraryDir()
	if err != nil {
		return err
	}

	initScript := "/etc/init.d/" + name
	if !util.CheckFileExists(initScript) {
		return errMissingService
	}
	sysconfigFile := "/etc/sysconfig/" + name

	if !force {
		fmt.Printf("\nThis command will make the following changes if not found already:\n")
		fmt.Printf("  - install libscope.so into %s\n", libraryDir)
		fmt.Printf("  - create/update %s\n", sysconfigFile)
		fmt.Printf("  - create /etc/scope/%s/scope.yml\n", name)
		fmt.Printf("  - create /var/log/scope/\n")
		fmt.Printf("  - create /var/run/scope/\n")
		status, err := confirm("Ready to proceed?")
		if err != nil {
			return err
		} else if !status {
			return errors.New("info: canceled")
		}
	}

	libraryPath, err := rc.installScope(name, libraryDir)
	if err != nil {
		return fmt.Errorf("error: failed to install Scope; %v", err)
	}

	if !util.CheckFileExists(sysconfigFile) {
		content := fmt.Sprintf("LD_PRELOAD=%s\nSCOPE_HOME=/etc/scope/%s\n", libraryPath, name)
		err := os.WriteFile(sysconfigFile, []byte(content), 0644)
		if err != nil {
			return fmt.Errorf("error: failed to create sysconfig file; %v", err)
		}
	} else {
		data, err := os.ReadFile(sysconfigFile)
		if err != nil {
			return fmt.Errorf("error: failed to read sysconfig file; %v", err)
		}
		strData := string(data)
		if !strings.Contains(strData, "LD_PRELOAD=") {
			content := fmt.Sprintf("LD_PRELOAD=%s\n", libraryPath)
			f, err := os.OpenFile(sysconfigFile, os.O_APPEND|os.O_WRONLY, 0644)
			if err != nil {
				return fmt.Errorf("error: failed to open sysconfig file; %v", err)
			}
			defer f.Close()
			if _, err = f.WriteString(content); err != nil {
				return fmt.Errorf("error: failed to update sysconfig file; %v", err)
			}
		}
		if !strings.Contains(strData, "SCOPE_HOME=") {
			content := fmt.Sprintf("SCOPE_HOME=/etc/scope/%s\n", name)
			f, err := os.OpenFile(sysconfigFile, os.O_APPEND|os.O_WRONLY, 0644)
			if err != nil {
				return fmt.Errorf("error: failed to open sysconfig file; %v", err)
			}
			defer f.Close()
			if _, err = f.WriteString(content); err != nil {
				return fmt.Errorf("error: failed to update sysconfig file; %v", err)
			}
		}
	}
	if !quiet {
		fmt.Printf("\nThe %s service has been updated to run with AppScope.\n", name)
		fmt.Printf("\nPlease review the configs in /etc/scope/%s/scope.yml and check their\npermissions to ensure the scoped service can read them.\n", name)
		fmt.Printf("\nAlso, please review permissions on /var/log/scope and /var/run/scope to ensure\nthe scoped service can write there.\n")
		fmt.Printf("\nRestart the service with `service %s restart` so the changes take effect.\n", name)
	}
	return nil
}

func (rc *Config) installOpenRc(name string, user string, force bool, quiet bool) error {
	libraryDir, err := getLibraryDir()
	if err != nil {
		return err
	}

	initScript := "/etc/init.d/" + name
	if !util.CheckFileExists(initScript) {
		return errMissingService
	}

	initConfigScript := "/etc/conf.d/" + name

	if !force {
		fmt.Printf("\nThis command will make the following changes if not found already:\n")
		fmt.Printf("  - install libscope.so into %s\n", libraryDir)
		fmt.Printf("  - create/update %s\n", initConfigScript)
		fmt.Printf("  - create /etc/scope/%s/scope.yml\n", name)
		fmt.Printf("  - create /var/log/scope/\n")
		fmt.Printf("  - create /var/run/scope/\n")
		status, err := confirm("Ready to proceed?")
		if err != nil {
			return err
		} else if !status {
			return errors.New("info: canceled")
		}
	}

	libraryPath, err := rc.installScope(name, libraryDir)
	if err != nil {
		return fmt.Errorf("error: failed to install Scope; %v", err)
	}

	if !util.CheckFileExists(initConfigScript) {
		fmt.Printf("Write to a file %s", initConfigScript)
		content := fmt.Sprintf("export LD_PRELOAD=%s\nexport SCOPE_HOME=/etc/scope/%s\n", libraryPath, name)
		err := os.WriteFile(initConfigScript, []byte(content), 0644)
		if err != nil {
			return fmt.Errorf("error: failed to update sysconfig file; %v", err)
		}
	} else {
		data, err := os.ReadFile(initConfigScript)
		if err != nil {
			return fmt.Errorf("error: failed to read sysconfig file; %v", err)
		}
		strData := string(data)
		if !strings.Contains(strData, "export LD_PRELOAD=") {
			content := fmt.Sprintf("export LD_PRELOAD=%s\n", libraryPath)
			f, err := os.OpenFile(initConfigScript, os.O_APPEND|os.O_WRONLY, 0644)
			if err != nil {
				return fmt.Errorf("error: failed to open sysconfig file; %v", err)
			}
			defer f.Close()
			if _, err = f.WriteString(content); err != nil {
				return fmt.Errorf("error: failed to update sysconfig file; %v", err)
			}
		}
		if !strings.Contains(strData, "export SCOPE_HOME=") {
			content := fmt.Sprintf("export SCOPE_HOME=/etc/scope/%s\n", name)
			f, err := os.OpenFile(initConfigScript, os.O_APPEND|os.O_WRONLY, 0644)
			if err != nil {
				return fmt.Errorf("error: failed to open sysconfig file; %v", err)
			}
			defer f.Close()
			if _, err = f.WriteString(content); err != nil {
				return fmt.Errorf("error: failed to update sysconfig file; %v", err)
			}
		}
	}
	if !quiet {
		fmt.Printf("\nThe %s service has been updated to run with AppScope.\n", name)
		fmt.Printf("\nPlease review the configs in /etc/scope/%s/scope.yml and check their\npermissions to ensure the scoped service can read them.\n", name)
		fmt.Printf("\nAlso, please review permissions on /var/log/scope and /var/run/scope to ensure\nthe scoped service can write there.\n")
		fmt.Printf("\nRestart the service with `rc-service %s restart` so the changes take effect.\n", name)
	}
	return nil
}
