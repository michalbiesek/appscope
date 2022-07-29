package run

import (
	"fmt"
	"io"
	"os"
	"strconv"

	"github.com/criblio/scope/libscope"
	"github.com/criblio/scope/util"
	"gopkg.in/yaml.v2"
)

// FilterConfig represents whole filter configuration
type FilterConfig struct {
	DenyProc  []DenyProcConfig  `mapstructure:"deny,omitempty" json:"deny,omitempty" yaml:"deny"`
	AllowProc []AllowProcConfig `mapstructure:"allow,omitempty" json:"allow,omitempty" yaml:"allow"`
}

// AllowProcConfig represents a allowed process configuration
type AllowProcConfig struct {
	Name   string               `mapstructure:"filter" json:"filter" yaml:"filter"`
	Config libscope.ScopeConfig `mapstructure:"config" json:"config" yaml:"config"`
}

// DenyProcConfig represents a deny process configuration
type DenyProcConfig struct {
	Name string `mapstructure:"filter" json:"filter" yaml:"filter"`
}

type Filter struct {
	fileName string
	data     []byte
}

func newFilter(fileName string) *Filter {
	ft := &Filter{}
	if fileName != "" {
		ft.fileName = fileName
	}
	return ft
}

func (fc *Filter) getConfig() (FilterConfig, error) {

	var filterCfg FilterConfig
	var err error

	if fc.fileName != "" {
		fc.data, err = os.ReadFile(fc.fileName)
	} else {
		fc.data, err = io.ReadAll(os.Stdin)
	}

	if err != nil {
		return filterCfg, err
	}

	err = yaml.Unmarshal(fc.data, &filterCfg)

	return filterCfg, err
}

func (fc *Filter) copyConfig() error {

	// Overwrite the config file ?
	// Check default location
	dest, err := os.OpenFile("/tmp/scope_filter.yml", os.O_RDWR|os.O_CREATE, 0755)
	if err != nil {
		return err
	}

	if _, err := dest.Write(fc.data); err != nil {
		return err
	}

	return nil
}

func (ac *AllowProcConfig) isAttachInConfig() bool {
	// return ac.ActionOn == "attach"
	return false
}

func (fc *Filter) handleInteractiveProcess(libscopePath string) error {

	// string profileDir = "/etc/profile.d/scope.sh"

	// if util
	// /etc/profile.d/
	// create file if not exist
	// fill the values there
	// bash profile
	return nil
}

func (rc *Config) Filter(fileName string) error {
	fc := newFilter(fileName)

	filterCfg, err := fc.getConfig()
	if err != nil {
		return err
	}

	for _, allowProcess := range filterCfg.AllowProc {
		fmt.Println("Start allowed process", allowProcess.Name)
		if err := rc.Service(allowProcess.Name, "", true, true); err == nil {
			fmt.Println("Scope Service", allowProcess.Name, "successfully performed")
		} else {
			fmt.Println("Scope Service", allowProcess.Name, "failed", err)
		}
		if allowProcess.isAttachInConfig() {
			if err := checkPermissionToAttach(); err != nil {
				// Translate Name to all Process
				attachProcesses, err := util.ProcessesByName(allowProcess.Name)
				if err != nil {
					continue
				}
				for _, attachProc := range attachProcesses {
					rc.Subprocess = true
					// TODO set the configuration
					// if allowProcess.config != nil {
					// 	rc.UserConfig = allowProcess.config
					// }
					if err := rc.Attach([]string{strconv.Itoa(attachProc.Pid)}); err == nil {
						fmt.Println("Scope Attach", allowProcess.Name, "PID", attachProc.Pid, "successfully performed")
					} else {
						fmt.Println("Scope Attach", allowProcess.Name, "failed", err)
					}
				}
			} else {
				fmt.Println("Attach for process", allowProcess.Name, "skipped:", err)
			}
		}
	}

	// for _, process := range filterCfg.denyProc {
	// 	TODO Detach
	// 	TODO Deservice
	// }

	// path, err := rc.getServiceLibraryDir()
	// if err != nil {
	// 	return err
	// }

	// TODO Use same scope as in service ?
	// libscopePath := filepath.Join(path, "libscope.so")
	// if !util.CheckFileExists(libscopePath) {
	// 	return errors.New("missing libscope.so")
	// }
	// for _, process := range filterCfg.denyProc {
	// 	TODO Detach
	// 	TODO Deservice
	// }

	// if err := fc.handleInteractiveProcess(libscopePath); err != nil {
	// 	return err
	// }

	if err := fc.copyConfig(); err != nil {
		return err
	}

	return nil
}
