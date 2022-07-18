package run

import (
	"fmt"
	"io"
	"os"
	"strconv"

	"github.com/criblio/scope/util"
	"gopkg.in/yaml.v2"
)

// FilterConfig represents whole filter configuration
type FilterConfig struct {
	AllowProc []AllowProcConfig `mapstructure:"allow_process,omitempty" json:"allow_process,omitempty" yaml:"allow_process"`
	DenyProc  []DenyProcConfig  `mapstructure:"deny_process,omitempty" json:"deny_process,omitempty" yaml:"deny_process"`
}

// AllowProcConfig represents a allowed process configuration
type AllowProcConfig struct {
	Name        string `mapstructure:"name" json:"name" yaml:"name"`
	Action_on   string `mapstructure:"action_on" json:"action_on" yaml:"action_on"`
	Arg         string `mapstructure:"arg,omitempty" json:"arg,omitempty" yaml:"arg,omitempty"`
	Config_file string `mapstructure:"config_file,omitempty" json:"config_file,omitempty" yaml:"config_file,omitempty"`
}

// AllowProcConfig represents a deny process configuration
type DenyProcConfig struct {
	Name string `mapstructure:"name" json:"name" yaml:"name"`
	Arg  string `mapstructure:"arg,omitempty" json:"arg,omitempty" yaml:"arg,omitempty"`
}

func (ac *AllowProcConfig) IsAttach() bool {
	return ac.Action_on == "attach"
}

func (rc *Config) Filter(fileName string) {
	//Parse a file
	if fileName == "" {
		fmt.Println("Filter file not defined. Using a default configuration")
		err := CreateDefaultFilter()
		util.CheckErrSprintf(err, "%v", err)
	} else {
		if !util.CheckFileExists(fileName) {
			util.ErrAndExit("%s not exist", fileName)
		}
		filterCfg, err := getProcFilterConfig(fileName)
		if err != nil {
			util.CheckErrSprintf(err, "%v", err)
		}

		for _, process := range filterCfg.AllowProc {
			fmt.Println("Start Scope Service process", process.Name)
			err := rc.Service(process.Name, "", true, true)
			if err == nil {
				fmt.Println("Scope Service", process.Name, "successfully performed")
			} else {
				fmt.Println("Scope Service", process.Name, "failed", err)
			}
			fmt.Println("End Scope Service process", process.Name)

			if process.IsAttach() {
				// Translate Name to all Process
				// TODO add error handling for function below
				fmt.Println("Start Scope Attach process", process.Name)
				attachProcesses, err := util.ProcessesByName(process.Name)
				if err != nil {
					continue
				}
				for _, attach_proc := range attachProcesses {
					fmt.Println("Start Scope Attach PID", attach_proc.Pid)
					rc.Subprocess = true
					err := rc.Attach([]string{strconv.Itoa(attach_proc.Pid)})
					if err == nil {
						fmt.Println("Scope Attach", process.Name, "PID", attach_proc.Pid, "successfully performed")
					} else {
						fmt.Println("Scope Attach", process.Name, "failed", err)
					}
				}
				fmt.Println("End Scope Attach process", process.Name)
			}
		}
		for _, process := range filterCfg.DenyProc {
			fmt.Println("Start parsing deny process", process.Name)
			// TODO: Detach
			// TODO: Deservice

		}

		err = CopyFilterFile(fileName)
		util.CheckErrSprintf(err, "%v", err)
	}
}

// Copy file to well known location for futher parsing
func CopyFilterFile(src string) error {
	// configFileLoc := filepath.Join("/tmp/", "scope_filter.yml")

	configFileLoc := "/tmp/scope_filter.yml"
	sourceFileStat, err := os.Stat(src)
	if err != nil {
		return err
	}

	if !sourceFileStat.Mode().IsRegular() {
		return err
	}

	source, err := os.Open(src)
	if err != nil {
		return err
	}
	defer source.Close()

	// Overwrite the config file ?
	dest, err := os.OpenFile(configFileLoc, os.O_RDWR|os.O_CREATE, 0755)
	if err != nil {
		return err
	}

	defer dest.Close()
	_, err = io.Copy(dest, source)
	return err
}

// Create default filter file
func CreateDefaultFilter() error {
	configFileLoc := "/tmp/scope_filter.yml"

	dest, err := os.OpenFile(configFileLoc, os.O_RDWR|os.O_CREATE, 0755)
	if err != nil {
		return err
	}

	defer dest.Close()

	return nil

}

// Get Proc Filter Config from yml file
func getProcFilterConfig(fileName string) (FilterConfig, error) {

	var filterCfg FilterConfig
	yamlFile, err := os.ReadFile(fileName)
	if err != nil {
		return filterCfg, err
	}
	if err = yaml.Unmarshal(yamlFile, &filterCfg); err != nil {
		return filterCfg, err
	}

	return filterCfg, nil
}
