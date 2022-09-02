package run

import (
	"io"
	"io/ioutil"
	"os"
	"strconv"

	"github.com/criblio/scope/libscope"
	"github.com/criblio/scope/loader"
	"github.com/criblio/scope/util"
	"github.com/rs/zerolog/log"
	"gopkg.in/yaml.v2"
)

// startConfig represents processed configuration
type startConfig struct {
	AllowProc []allowProcConfig `mapstructure:"allow,omitempty" json:"allow,omitempty" yaml:"allow"`
	DenyProc  []procConfig      `mapstructure:"deny,omitempty" json:"deny,omitempty" yaml:"deny"`
}

// procConfig represents a process configuration
type procConfig struct {
	Procname string `mapstructure:"procname,omitempty" json:"procname,omitempty" yaml:"procname,omitempty"`
	Arg      string `mapstructure:"arg,omitempty" json:"arg,omitempty" yaml:"arg,omitempty"`
	Hostname string `mapstructure:"hostname,omitempty" json:"hostname,omitempty" yaml:"hostname,omitempty"`
	Username string `mapstructure:"username,omitempty" json:"username,omitempty" yaml:"username,omitempty"`
	Env      string `mapstructure:"env,omitempty" json:"env,omitempty" yaml:"env,omitempty"`
	Ancestor string `mapstructure:"ancestor,omitempty" json:"ancestor,omitempty" yaml:"ancestor,omitempty"`
}

// allowProcConfig represents a allowed process configuration
type allowProcConfig struct {
	Procname string               `mapstructure:"procname,omitempty" json:"procname,omitempty" yaml:"procname,omitempty"`
	Arg      string               `mapstructure:"arg,omitempty" json:"arg,omitempty" yaml:"arg,omitempty"`
	Hostname string               `mapstructure:"hostname,omitempty" json:"hostname,omitempty" yaml:"hostname,omitempty"`
	Username string               `mapstructure:"username,omitempty" json:"username,omitempty" yaml:"username,omitempty"`
	Env      string               `mapstructure:"env,omitempty" json:"env,omitempty" yaml:"env,omitempty"`
	Ancestor string               `mapstructure:"ancestor,omitempty" json:"ancestor,omitempty" yaml:"ancestor,omitempty"`
	Config   libscope.ScopeConfig `mapstructure:"config" json:"config" yaml:"config"`
}

// return status from start operation
var startErr error

// Retrieve and unmarshall configuration passed in stdin.
// It returns the status of operation.
func getConfigFromStdin() (startConfig, []byte, error) {

	var startCfg startConfig

	data, err := io.ReadAll(os.Stdin)

	if err != nil {
		return startCfg, data, err
	}

	err = yaml.Unmarshal(data, &startCfg)

	return startCfg, data, err
}

// startAttachSingleProcess attach to the the specific process identifed by pid with configuration pass in cfgData.
// It returns the status of operation.
func startAttachSingleProcess(pid string, cfgData []byte) error {
	tmpFile, err := os.CreateTemp(".", pid)
	if err != nil {
		log.Error().
			Err(err).
			Str("pid", pid).
			Msg("Create Temp configuration failed.")
		startErr = err
		return err
	}
	defer os.Remove(tmpFile.Name())

	if _, err := tmpFile.Write(cfgData); err != nil {
		log.Error().
			Err(err).
			Str("pid", pid).
			Msg("Write Temp configuration failed.")
		startErr = err
		return err
	}

	env := append(os.Environ(), "SCOPE_CONF_PATH="+tmpFile.Name())
	ld := loader.ScopeLoader{Path: ldscopePath()}
	stdoutStderr, err := ld.AttachSubProc([]string{pid}, env)
	if err != nil {
		log.Error().
			Err(err).
			Str("pid", pid).
			Str("loaderDetails", stdoutStderr).
			Msg("Attach failed.")
		startErr = err
	} else {
		log.Info().
			Str("pid", pid).
			Msg("Attach success.")
	}
	os.Unsetenv("SCOPE_CONF_PATH")
	return err
}

func startAttach(alllowProc allowProcConfig) error {
	cfgSingleProc, err := yaml.Marshal(alllowProc.Config)
	if err != nil {
		log.Error().
			Err(err).
			Msg("Serialize configuration failed.")
		startErr = err
		return err
	}

	allProc, err := util.ProcessesByName(alllowProc.Procname)
	if err != nil {
		log.Error().
			Err(err).
			Str("process", alllowProc.Procname).
			Msg("Retrieve Process name failed.")
		startErr = err
		return err
	}

	for _, process := range allProc {
		if process.Scoped {
			log.Info().
				Str("pid", strconv.Itoa(process.Pid)).
				Msg("Process is already scoped.")
			continue
		}

		err := startAttachSingleProcess(strconv.Itoa(process.Pid), cfgSingleProc)
		if err != nil {
			log.Error().
				Err(err).
				Str("pid", strconv.Itoa(process.Pid)).
				Msg("Attach failed.")
			startErr = err
		} else {
			log.Info().
				Str("pid", strconv.Itoa(process.Pid)).
				Msg("Attach success.")
		}
	}

	return nil
}

// startConfigureHost setup the Host:
//
// - setup /etc/profile.d/scope.sh
//
// - extract filter into the filter file in /tmp/scope_filter.yml
//
// - extract libscope.so into the filter file in /tmp/libscope.so
//
// It returns the status of operation.
func startConfigureHost(filterData []byte) error {
	tempFile, err := ioutil.TempFile("/tmp", "temp_filter")
	if err != nil {
		return err
	} else {
		log.Info().
			Msg("Create test_filter succeded.")
	}
	defer os.Remove(tempFile.Name())

	if _, err := tempFile.Write(filterData); err != nil {
		return err
	} else {
		log.Info().
			Msg("Write test_filter succeded.")
	}

	ld := loader.ScopeLoader{Path: ldscopePath()}

	stdoutStderr, err := ld.ConfigureHost(tempFile.Name())
	if err != nil {
		log.Fatal().
			Err(err).
			Str("loaderDetails", stdoutStderr).
			Msg("Setup on Host failed.")
		return err
	} else {
		log.Info().
			Msg("Setup on Host success.")
	}

	return nil
}

// startServiceHost setup the Container:
//
// - setup service defined by the serviceName on the host.
//
// It returns the status of operation.
func startServiceHost(serviceName string) error {
	ld := loader.ScopeLoader{Path: ldscopePath()}
	stdoutStderr, err := ld.ServiceHost(serviceName)
	if err != nil {
		log.Error().
			Err(err).
			Str("service", serviceName).
			Str("loaderDetails", stdoutStderr).
			Msg("Service on Host failed.")
		startErr = err
	} else {
		log.Info().
			Str("service", serviceName).
			Msg("Service on Host success.")
	}
	return err
}

// startSetupContainer setup the Container:
//
// - setup /etc/profile.d/scope.sh in the container.
//
// - extract filter into the filter file in /tmp/scope_filter.yml in the container.
//
// - extract libscope.so into the filter file in /tmp/libscope.so in the container.
//
// - service services defined by the allowProcs process name list in the container.
//
// It returns the status of operation.
func startSetupContainer(allowProcs []allowProcConfig) error {
	// Iterate over all containers
	cPids, _ := util.GetDockerPids()
	for _, cPid := range cPids {
		ld := loader.ScopeLoader{Path: ldscopePath()}
		stdoutStderr, err := ld.ConfigureContainer("/tmp/scope_filter.yml", cPid)
		if err != nil {
			log.Error().
				Err(err).
				Str("pid", strconv.Itoa(cPid)).
				Str("loaderDetails", stdoutStderr).
				Msg("Setup on container failed.")
			startErr = err
		} else {
			log.Info().
				Str("pid", strconv.Itoa(cPid)).
				Msg("Setup on container success.")
		}

		// Iterate over all allowed processses
		for _, process := range allowProcs {
			// Setup service
			stdoutStderr, err := ld.ServiceContainer(process.Procname, cPid)
			if err != nil {
				log.Error().
					Err(err).
					Str("service", process.Procname).
					Str("pid", strconv.Itoa(cPid)).
					Str("loaderDetails", stdoutStderr).
					Msg("Service on container failed.")
				startErr = err
			} else {
				log.Info().
					Str("pid", strconv.Itoa(cPid)).
					Msg("Service on container success.")
			}
		}
	}
	return nil
}

// Run start command responsible for setup host and container env
// It returns the status of operation.
func (rc *Config) Start() error {
	rc.setupWorkDir([]string{"start"}, true)

	// Validate user has root permissions
	if err := util.UserVerifyRootPerm(); err != nil {
		log.Fatal().
			Err(err).
			Msg("Verfy root permission failed.")
		return err
	}

	startCfg, startcfgData, err := getConfigFromStdin()
	if err != nil {
		log.Fatal().
			Err(err).
			Msg("Read filter file failed.")
		return err
	}

	if err := createLdscope(); err != nil {
		log.Fatal().
			Err(err).
			Msg("Create ldscope failed.")
		return err
	}

	// Configure Host
	if err := startConfigureHost(startcfgData); err != nil {
		log.Error().
			Err(err).
			Msg("Setup host failed.")
		startErr = err
	} else {
		log.Info().
			Msg("Setup host success.")
	}

	// Setup Containers
	if err := startSetupContainer(startCfg.AllowProc); err != nil {
		log.Error().
			Err(err).
			Msg("Setup constainers failed.")
		startErr = err
	} else {
		log.Info().
			Msg("Setup constainers success.")
	}

	// Iterate over allowed process
	// Setup service on Host
	// Attach to services on Host and container
	for _, alllowProc := range startCfg.AllowProc {

		// Service the host chek error
		startServiceHost(alllowProc.Procname)

		// Attach to the process on the host and in the container
		startAttach(alllowProc)
	}

	// Deny list actions
	// Currently NOP
	// Detach ?
	// Deservice ?

	return startErr
}
