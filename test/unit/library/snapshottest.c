#define _GNU_SOURCE
#include "snapshot.h"
#include "test.h"
#include "scopestdlib.h"
#include <signal.h>

static void
snapshotSigSegvTest(void **state)
{
    struct sigaction act = { 0 };
    act.sa_handler = (void (*))graceSigAbrtHandler;
    act.sa_flags = SA_RESTART | SA_SIGINFO;
    sigaction(SIGABRT, &act, NULL);

    // install signal for SIGABRT
    pid_t pid = scope_getpid();
    siginfo_t info = {.si_signo = SIGSEGV, .si_code = SEGV_MAPERR};
    snapshotSignalHandler(-1, &info, NULL);
    // verify files
        sigaction(SIGABRT, &act, NULL);

}

int
main(int argc, char* argv[])
{
    printf("running %s\n", argv[0]);

    const struct CMUnitTest tests[] = {
        cmocka_unit_test(snapshotSigSegvTest),
        cmocka_unit_test(dbgHasNoUnexpectedFailures),
    };
    return cmocka_run_group_tests(tests, groupSetup, groupTeardown);
}
