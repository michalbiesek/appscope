package util

import (
	"encoding/json"
	"fmt"
)

// ScopeStatus represents the process status in context of libscope.so
type ScopeStatus int

// Scope Status description
//
// When process started it will be in one of the following states:
// Disable
// Active
// Setup
//
// Possible transfer between states:
// Disable -> Active (attach required sudo - ptrace)
// Setup -> Active   (attach required sudo - ptrace)
// Latent -> Active
// Active -> Latent
// Setup -> Latent
//
// If the process is in Active status it can be changed only to Latent status

const (
	Disable ScopeStatus = iota // libscope.so is not loaded
	Setup                      // libscope.so is loaded, reporting thread is not present
	Active                     // libscope.so is loaded, reporting thread is present we are sending data
	Latent                     // libscope.so is loaded, reporting thread is present we are not emiting any data
)

func (state ScopeStatus) String() string {
	return []string{"Disable", "Setup", "Active", "Latent"}[state]
}

func TestFrames(pid int) ([]byte, error) {
	var testResp scopeTestResponse

	request, _ := json.Marshal(scopeRequestOnly{Req: cmdReqGetScopeCfg})

	response, err := ipcDispatcher(request, pid)

	err = json.Unmarshal(response, &testResp)
	if err != nil {
		fmt.Printf("Unmarshal fails: %v", err)
	} else {
		fmt.Printf("Received a message: %#v", testResp)
	}
}

// getScopeStatus retreives information about Scope status using IPC
func getScopeStatus(pid int) ScopeStatus {
	var scopeResp scopeStatusResponse

	request, _ := json.Marshal(scopeRequestOnly{Req: cmdReqGetScopeStatus})

	response, err := ipcDispatcher(request, pid)
	if err != nil {
		return Setup
	}
	json.Unmarshal(response, &scopeResp)

	if scopeResp.Status == responseOK {
		if scopeResp.Scoped {
			return Active
		}
		return Latent
	}
	return Disable
}
