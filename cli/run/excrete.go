package run

import (
	"fmt"
	"os"
	"path"
	"path/filepath"

	"github.com/criblio/scope/loader"
	"github.com/criblio/scope/util"
)

func (rc *Config) Extract(outPath string) {
	newPath, err := filepath.Abs(outPath)
	util.CheckErrSprintf(err, "cannot resolve absolute path of %s: %v", outPath, err)
	outPath = newPath

	err = CreateAll(outPath)
	util.CheckErrSprintf(err, "error excreting files: %v", err)
	if rc.MetricsDest != "" || rc.EventsDest != "" || rc.CriblDest != "" {
		err = os.Rename(path.Join(outPath, "scope.yml"), path.Join(outPath, "scope_example.yml"))
		util.CheckErrSprintf(err, "error renaming scope.yml: %v", err)
		rc.WorkDir = outPath
		err = rc.WriteScopeConfig(path.Join(outPath, "scope.yml"), 0644)
		util.CheckErrSprintf(err, "error writing scope.yml: %v", err)
	}
	ld := loader.ScopeLoader{Path: path.Join(outPath, "ldscope")}
	ld.Patch(path.Join(outPath, "libscope.so"))
	fmt.Printf("Successfully extracted to %s.\n", outPath)
}
