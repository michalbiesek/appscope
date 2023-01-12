package inspect

import (
	"encoding/json"
	"errors"
	"fmt"

	"github.com/criblio/scope/ipc"
)

var errInspectCfg = errors.New("error inspect cfg")

// InspectScopeCfg returns the configuration of scoped process
func InspectScopeCfg(pidCtx ipc.IpcPidCtx) (string, error) {

	cmd := ipc.CmdGetScopeCfg{}
	resp, err := cmd.Request(pidCtx)
	if err != nil {
		return "", err
	}

	err = cmd.UnmarshalResp(resp.ResponseScopeMsgData)
	if err != nil {
		return "", err
	}
	if resp.MetaMsgStatus != ipc.ResponseOK || *cmd.Response.Status != ipc.ResponseOK {
		return "", errInspectCfg
	}
	marshalToPrint, err := json.MarshalIndent(cmd.Response.Cfg.Current, "", "   ")
	if err != nil {
		return "", err
	}

	// TODO merge the both response in JSON

	cmdTr := ipc.CmdGetTransportStatus{}
	respTr, err := cmd.Request(pidCtx)
	if err != nil {
		return "", err
	}

	err = cmd.UnmarshalResp(respTr.ResponseScopeMsgData)
	if err != nil {
		return "", err
	}

	if respTr.MetaMsgStatus != ipc.ResponseOK || *cmdTr.Response.Status != ipc.ResponseOK {
		return "", errInspectCfg
	}

	marshalToPrintTransport, err := json.MarshalIndent(cmdTr.Response.Data, "", "   ")
	if err != nil {
		return "", err
	}
	fmt.Println(marshalToPrintTransport)

	return string(marshalToPrint), nil
}
