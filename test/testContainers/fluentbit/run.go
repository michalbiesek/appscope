package main

import (
	"log"
	"os"
	"os/exec"
)

func main() {

	cmd := exec.Command("docker", "build", "appscope/", "-t", "test-appscope")
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	err := cmd.Run()
	if err != nil {
		log.Fatalf("cmd.run() failed with %s\n", err)
	}

	cmd = exec.Command("docker", "run", "--rm", "test-appscope", "scope", "run", "ps")
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	err = cmd.Run()
	if err != nil {
		log.Fatalf("cmd.run() failed with %s\n", err)
	}

}
