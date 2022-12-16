package util

import (
	"net/http"
)

// This file describes the response logic in IPC communication
// library -> CLI

// respStatVal describes the status of request operation (received from library)
// IMPORTANT NOTE:
// responseStatus must be inline with library: ipc_resp_status_t
type respStatVal int

const (
	responseOK             respStatVal = http.StatusOK
	responsePartialData                = http.StatusPartialContent
	responseBadRequest                 = http.StatusBadRequest
	responseServerError                = http.StatusInternalServerError
	responseNotImplemented             = http.StatusNotImplemented
)

// msgQResponse describes message which will be transferred in response message queue (from library to CLI)
type msgQResponse struct {
	// Response status
	Status respStatVal `json:"status" jsonschema:"required"`
	// Request Unique Identifter
	Uniq int `json:"uniq" jsonschema:"required"`
	// Sequence frame Id
	Id int `json:"id" jsonschema:"required"`
	// Sequence frame max
	Max int `json:"max" jsonschema:"required"`
	// Frame data pass in response (scope response)
	Data string `json:"data" jsonschema:"required"`
}

// Todo describe where must be inline in c code
type scopeStatusResponse struct {
	// Response status
	Status respStatVal `json:"status" jsonschema:"required"`
	// Scoped status
	Scoped bool `json:"scoped"`
}

// Todo remove this mockup
type scopeTestResponse struct {
	// Response status
	Status respStatVal `json:"status" jsonschema:"required"`
	// FieldTest1
	FieldTest1 int `json:"fieldTest1" jsonschema:"required"`
	// FieldTest2
	FieldTest2 int `json:"fieldTest2" jsonschema:"required"`
	// FieldTest3
	FieldTest3 int `json:"fieldTest3" jsonschema:"required"`
	// FieldTest4
	FieldTest4 int `json:"fieldTest4" jsonschema:"required"`
	// FieldTest5
	FieldTest5 int `json:"fieldTest5" jsonschema:"required"`
}
