package util

// ScopeState represents the process state in context of libscope.so
type ScopeState int64

// Once the process is in Active state it can be changed only to Latent state
const (
	Disable ScopeState = iota // libscope.so is not loaded
	Setup                     // libscope.so is loaded, reporting thread is not present
	Active                    // libscope.so is loaded, reporting thread is present we are sending data
	Latent                    // libscope.so is loaded, reporting thread is present we are not emiting any data
)

func (state ScopeState) string() string {
	switch state {
	case Disable:
		return "Disable"
	case Setup:
		return "Setup"
	case Active:
		return "Active"
	case Latent:
		return "Latent"
	}
	// not reachable
	return "Unknown"
}
