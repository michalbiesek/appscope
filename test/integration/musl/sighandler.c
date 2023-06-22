// gcc -g -o sighandler sighandler.c

// This test app was created to handle a SIGUSR2 that the library
// also has an interest in. It can register SIGUSR2 in different ways:
// signal()/sigaction() or sigtimer().
//
// The goal is to test that our library can manage to call original
// test app signal handlers after the library's own handlers.


#define _GNU_SOURCE
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

const char *progname;

#define toStdout(str) (write(STDOUT_FILENO, str, strlen(str)))
#define failure(str) do { printf(str); exit(1); } while (0)


typedef enum {SIGNAL, SIGACTION} func_t;
static const char * const funcToName[] = {
    [SIGNAL]          = "signal",
    [SIGACTION]       = "sigaction",
};
#define FUNC_COUNT (sizeof(funcToName)/sizeof(funcToName[0]))


// a handler that just prints something to stdout to show it's been run
void
handleit(char *signame)
{
    // Ugly, but is a sig-safe alternative to:
    // printf("  Handling %s from %s\n", signame, progname);
    toStdout("  Handling ");
    toStdout(signame);
    toStdout(" from ");
    toStdout(progname);
    toStdout("\n");
    fsync(STDOUT_FILENO);
}
void handleUsr2(int signum) {handleit("SIGUSR2");}

typedef struct {
    int num;
    char *str;
    sighandler_t fn;
} sigList_t;

// These are the ones which libscope.so is interested in.
// So, these are ones we should test for interactions
sigList_t sigUsr2 = {.num = SIGUSR2,
                     .str = "SIGUSR2",
                     .fn = handleUsr2
                    };

// registers the handler, using the func specified
void
registerSigHandlers(func_t func)
{
    printf("  Executing %s using %s()\n", __func__, funcToName[func]);

    printf("    Registering %s\n", sigUsr2.str);
    if (func == SIGNAL) {
        if (signal(sigUsr2.num,sigUsr2.fn) == SIG_ERR) {
            failure("signal() call failed\n");
        }
    } else if (func == SIGACTION) {
        struct sigaction act = {.sa_handler = sigUsr2.fn,
                                .sa_mask = 0,
                                .sa_flags = 0};
        struct sigaction oldact;
        if (sigaction(sigUsr2.num, &act, &oldact) == -1) {
            failure("sigaction() call failed\n");
        }
    } else {
        failure("Unexpected function\n");
    }
}

int
main(int argc, char *argv[])
{
    progname = argv[0];

    // Read command line argument to determine which function to use
    // when registering our signal handler
    printf("Starting execution of %s\n", progname);
    if (argc != 2) {
        failure("expected one argument, either \"signal\" or \"sigaction\"\n");
    }

    func_t function;
    int i;
    for (i=0; i<FUNC_COUNT; i++) {
        if (!strcmp(argv[1], funcToName[i])) {
            function = i;
            break;
        }
    }
    if (i >= FUNC_COUNT) {
        failure("expected one argument, either \"signal\" or \"sigaction\"\n");
    }

    // register signal handler using the specified function
    printf("Registering to handle signals\n");
    registerSigHandlers(function);

    // wait a while for signal from the outside
    struct timespec time = {.tv_sec = 4, .tv_nsec = 0};
    nanosleep(&time, NULL);

    printf("Ending execution of %s\n", progname);
    return 0;
}
