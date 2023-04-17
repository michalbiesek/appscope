package main

/*
#if defined(__aarch64__)
#cgo LDFLAGS: -L../lib/linux/aarch64 -lloader
#elif defined(__riscv)
#cgo LDFLAGS: -L../lib/linux/riscv64 -lloader
#elif defined(__x86_64__)
#cgo LDFLAGS: -L../lib/linux/x86_64 -lloader
#else
#error Bad arch defined
#endif

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <fcntl.h>

#include "../src/loader/libdir.h"
#include "../src/loader/loader.h"
#include "../src/loader/patch.h"
#include "../src/loader/ns.h"

// Example Usage:
// scope [OPTIONS] --ldattach PID
// scope [OPTIONS] --lddetach PID
// scope [OPTIONS] --configure FILTER_PATH --namespace PID
// scope [OPTIONS] --service SERVICE --namespace PID
// scope [OPTIONS] --passthrough EXECUTABLE [ARGS...]
// scope [OPTIONS] --patch SO_FILE
//
// Options:
// -l, --libbasedir DIR              specify parent for the library directory (default: /tmp)
// -f DIR                            alias for \"-l DIR\" for backward compatibility
// -a, --ldattach PID                attach to the specified process ID
// -d, --lddetach PID                detach from the specified process ID
// -c, --configure FILTER_PATH       configure scope environment with FILTER_PATH
// -w, --unconfigure                 unconfigure scope environment
// -g, --getfile SRC_FILE,DEST_FILE  get a file from SRC_FILE and put it in DEST_FILE
// -s, --service SERVICE             setup specified service NAME
// -v, --unservice                   remove scope from all service configurations
// -n  --namespace PID               perform operation on specified container PID
// -p, --patch SO_FILE               patch specified libscope.so
// -r, --starthost                   execute the scope start command in a host context (must be run in the container)
// -x, --stophost                    execute the scope stop command in a host context (must be run in the container)

// Long aliases for short options
// NOTE: Be sure to align these with the options listed in the call to getopt_long
// NOTE: These must not conflict with the cli options specified in the cmd/ package
static struct option opts[] = {
    { "ldattach",    required_argument, 0, 'a' },
    { "lddetach",    required_argument, 0, 'd' },
    { "namespace",   required_argument, 0, 'n' },
    { "configure",   required_argument, 0, 'c' },
    { "unconfigure", no_argument,       0, 'w' },
    { "getfile",     required_argument, 0, 'g' },
    { "service",     required_argument, 0, 's' },
    { "unservice",   no_argument,       0, 'v' },
    { "libbasedir",  required_argument, 0, 'l' },
    { "patch",       required_argument, 0, 'p' },
    { "starthost",   no_argument,       0, 'r' },
    { "stophost",    no_argument,       0, 'x' },
    { "passthrough", no_argument,       0, 'z' },
    { 0, 0, 0, 0 }
};

unsigned long g_libscopesz;
unsigned long g_scopedynsz;

// This is the constructor for the Go application.
// Code here executes before the Go Runtime starts.
// We execute loader-specific commands here, because we can perform namespace switches while meeting
// the OS requirement of being in a single-threaded process to do so.
// Note: We cannot rely on argc/argv/environ being present,
// i.e. the musl library does not define them as constructor args.
__attribute__((constructor)) void cli_constructor() {
    FILE *f;
    int c;
	char fname[PATH_MAX];
	int scope_pid = getpid();
	int arg_max = 1024; // Is this reasonable?
	int arg_c = 0;
	size_t size = 0;
	char **arg_v;
	int index;
	pid_t nspid = -1;
	pid_t pid = -1;
	uid_t eUid = geteuid();
	int cmdArgc;
	char **cmdArgv;

	bool opt_ldattach = false;
	bool opt_lddetach = false;
	bool opt_namespace = false;
	bool opt_configure = false;
	bool opt_unconfigure = false;
	bool opt_getfile = false;
	bool opt_service = false;
	bool opt_unservice = false;
	bool opt_libbasedir = false;
	bool opt_patch = false;
	bool opt_starthost = false;
	bool opt_stophost = false;
	bool opt_passthrough = false;

	char *arg_ldattach;
	char *arg_lddetach;
	char *arg_configure;
	char *arg_getfile;
	char *arg_service;
	char *arg_namespace;
	char *arg_libbasedir;
	char *arg_patch;

    if ((g_libscopesz = strtoul(LIBSCOPE_SO_SIZE, NULL, 10)) == ULONG_MAX) {
       perror("strtoul");
       exit(EXIT_FAILURE);
    }

    if ((g_scopedynsz = strtoul(SCOPEDYN_SIZE, NULL, 10)) == ULONG_MAX) {
       perror("strtoul");
       exit(EXIT_FAILURE);
    }

	// Open /proc/pid/cmdline to get args
	snprintf(fname, sizeof fname, "/proc/%d/cmdline", scope_pid);
    f = fopen(fname, "r");
    if (!f) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

	// Read args delimited by null
	arg_v = malloc(sizeof(char *) * arg_max);
	if (!arg_v) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	while(getdelim(&arg_v[arg_c], &size, 0, f) != -1)
	{
		arg_c++;
	}
	arg_v[arg_c] = NULL;

    fclose(f);

    for (;;) {
        index = 0;
        int opt = getopt_long(arg_c, arg_v, "+:a:d:n:l:p:c:g:s:rxvwz", opts, &index);
        if (opt == -1) {
            break;
        }
        switch (opt) {
		case 'a':
			opt_ldattach = true;
			arg_ldattach = optarg;
			break;
		case 'd':
			opt_lddetach = true;
			arg_lddetach = optarg;
			break;
		case 'n':
			opt_namespace = true;
			arg_namespace = optarg;
			break;
		case 'c':
			opt_configure = true;
			arg_configure = optarg;
			break;
		case 'w':
			opt_unconfigure = true;
			break;
		case 'g':
			opt_getfile = true;
			arg_getfile = optarg;
			break;
		case 's':
			opt_service = true;
			arg_service = optarg;
			break;
		case 'v':
			opt_unservice = true;
			break;
		case 'f':
		case 'l':
			opt_libbasedir = true;
			arg_libbasedir = optarg;
			break;
		case 'p':
			opt_patch = true;
			arg_patch = optarg;
			break;
		case 'r':
			opt_starthost = true;
			break;
		case 'x':
			opt_stophost = true;
			break;
		case 'z':
			opt_passthrough = true;
			break;
		case ':': // Handle options missing their arg value
			switch (optopt) {
			default:
				fprintf(stderr, "error: missing required value for -%c option\n", optopt);
				exit(EXIT_FAILURE);
			}
			break;
		default:
			break;
        }
    }

	// Handle potential argument conflicts
	if (opt_ldattach && opt_lddetach) {
        fprintf(stderr, "error: --ldattach and --lddetach cannot be used together\n");
        exit(EXIT_FAILURE);
	}
	if (opt_service && opt_unservice) {
        fprintf(stderr, "error: --service and --unservice cannot be used together\n");
        exit(EXIT_FAILURE);
	}
	if (opt_configure && opt_unconfigure) {
        fprintf(stderr, "error: --configure and --unconfigure cannot be used together\n");
        exit(EXIT_FAILURE);
	}
    if ((opt_ldattach || opt_lddetach) && (opt_service || opt_unservice)) {
        fprintf(stderr, "error: --ldattach/--lddetach and --service/--unservice cannot be used together\n");
        exit(EXIT_FAILURE);
    }
    if ((opt_ldattach || opt_lddetach) && (opt_configure || opt_unconfigure)) {
        fprintf(stderr, "error: --ldattach/--lddetach and --configure/--unconfigure cannot be used together\n");
        exit(EXIT_FAILURE);
    }
    if ((opt_configure || opt_unconfigure) && (opt_service || opt_unservice)) {
        fprintf(stderr, "error: --configure/--unconfigure and --service/--unservice cannot be used together\n");
        exit(EXIT_FAILURE);
    }
    if (opt_namespace && (!opt_configure && !opt_unconfigure && !opt_service && !opt_unservice && !opt_getfile)) {
        fprintf(stderr, "error: --namespace option requires --configure/--unconfigure or --service/--unservice or --getfile option\n");
        exit(EXIT_FAILURE);
    }
	if (opt_passthrough && (opt_ldattach || opt_lddetach || opt_namespace ||
		opt_service || opt_unservice || opt_configure || opt_unconfigure || opt_getfile)) {
        fprintf(stderr, "error: --passthrough cannot be used with --ldattach/--lddetach or --namespace or --service/--unservice or --configure/--unconfigure or --getfile\n");
        exit(EXIT_FAILURE);
	}

	// Handle potential permissions issues
	if (eUid && (opt_configure || opt_unconfigure || opt_service || opt_unservice || opt_getfile)) {
        fprintf(stderr, "error: command requires root\n");
        exit(EXIT_FAILURE);
	}

	// Process options for use with commands
	if (opt_libbasedir) {
		if (libdirSetLibraryBase(arg_libbasedir)) {
			exit(EXIT_FAILURE);
		}
	}
	if (opt_namespace) {
		nspid = atoi(arg_namespace);
		if (nspid < 1) {
			fprintf(stderr, "error: invalid --namespace PID: %s\n", arg_namespace);
			exit(EXIT_FAILURE);
		}
	}
	if (opt_ldattach) {
		pid = atoi(arg_ldattach);
		if (pid < 1) {
			fprintf(stderr, "error: invalid --ldattach PID: %s\n", arg_ldattach);
			exit(EXIT_FAILURE);
		}
	}
	if (opt_lddetach) {
		pid = atoi(arg_lddetach);
		if (pid < 1) {
			fprintf(stderr, "error: invalid --lddetach PID: %s\n", arg_lddetach);
			exit(EXIT_FAILURE);
		}
	}

	// Execute commands
	cmdArgc = arg_c-optind; // argc of the program we want to scope
	cmdArgv = &arg_v[optind]; // argv of the program we want to scope

	if (opt_ldattach) exit(cmdAttach(true, pid));
	if (opt_lddetach) exit(cmdAttach(false, pid));
	if (opt_configure) exit(cmdConfigure(arg_configure, nspid));
	if (opt_unconfigure) exit(cmdUnconfigure(nspid));
	if (opt_getfile) exit(cmdGetFile(arg_getfile, nspid));
	if (opt_service) exit(cmdService(arg_service, nspid));
	if (opt_unservice) exit(cmdUnservice(nspid));
	if (opt_patch) exit(patchLibrary(arg_patch) == PATCH_FAILED);
	if (opt_starthost) exit(nsHostStart());
	if (opt_stophost) exit(nsHostStop());
	if (opt_passthrough) exit(cmdRun(false, false, pid, nspid, cmdArgc, cmdArgv));

	// No constructor command executed.
	// Continue to regular CLI usage.
	if (arg_v) free(arg_v);
	return;
}
*/
import "C"
import (
	"github.com/criblio/scope/cmd"
	"github.com/criblio/scope/internal"
)

// GitSummary is produced by govvv and stores tag, commit and branch status
var GitSummary string

// BuildDate is set by govvv and stores when we were built
var BuildDate string

// Version is set by govvv and stores the version at build time
var Version string

func main() {
	internal.SetVersionInfo(GitSummary, BuildDate, Version)
	cmd.Execute()
}
