package loader

import (
	"os"
	"os/exec"
	"syscall"
)

// Loader represents ldscope object
type ScopeLoader struct {
	Path string
}

func (sL *ScopeLoader) Patch(libscopePath string) error {
	cmd := exec.Command(sL.Path, "-p", libscopePath)
	cmd.Env = os.Environ()
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	_ = cmd.Run()
	return nil
}

func (sL *ScopeLoader) Run(args []string, env []string) error {
	args = append([]string{"ldscope"}, args...)
	return syscall.Exec(sL.Path, args, env)
}

func (sL *ScopeLoader) RunSubProc(args []string, env []string) error {
	cmd := exec.Command(sL.Path, args...)
	cmd.Env = env
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	return cmd.Run()
}

func (sL *ScopeLoader) Attach(args []string, env []string) error {
	args = append([]string{"--attach"}, args...)
	return sL.Run(args, env)
}

func (sL *ScopeLoader) AttachSubProc(args []string, env []string) error {
	args = append([]string{"--attach"}, args...)
	return sL.RunSubProc(args, env)
}
