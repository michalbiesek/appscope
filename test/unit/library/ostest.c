#define _GNU_SOURCE

#include "fn.h"
#include "os.h"
#include "scopestdlib.h"
#include "test.h"

static void
dummyHandler(int sig, siginfo_t *info, void *secret) {
    assert_int_equal(sig, SIGUSR2);
    assert_int_equal(info->si_signo, SIGUSR2);
    assert_int_equal(info->si_code, SI_TIMER);
    // copy pasta from SIG_ORIGIN_SCOPE_ID
    assert_int_equal(info->si_int, 123456789);
}

static void
osTestTimerStopNotInit(void **state) {
    bool res = osTimerStop();
    assert_false(res);
}

static void
osTestTimerBasic(void **state) {
    bool res = osThreadInit(dummyHandler, 1);
    assert_true(res);
    scope_sleep(2);
    res = osTimerStop();
    assert_true(res);
}

static void
osWritePermSuccess(void **state) {
    int perm = PROT_READ | PROT_EXEC;
    size_t len = 4096;
    void *addr = scope_mmap(NULL, len, perm, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    assert_ptr_not_equal(addr, MAP_FAILED);
    bool res = osMemPermAllow(addr, len, perm, PROT_WRITE);
    assert_true(res);
    res = osMemPermRestore(addr, len, perm);
    assert_true(res);
    scope_munmap(addr ,len);
}

static void
osWritePermFailure(void **state) {
    int perm = PROT_READ;
    size_t len = 4096;

    // Open file as read only
    int fd = scope_open("/etc/passwd", O_RDONLY);
    void *addr = scope_mmap(NULL, len, perm, MAP_SHARED, fd, 0);
    assert_ptr_not_equal(addr, MAP_FAILED);
    bool res = osMemPermAllow(addr, len, perm, PROT_WRITE);
    assert_false(res);
    scope_munmap(addr ,len);
    scope_close(fd);
}

int
main(int argc, char* argv[]) {
    printf("running %s\n", argv[0]);

    initFn();

    const struct CMUnitTest tests[] = {
        cmocka_unit_test(osTestTimerStopNotInit),
        cmocka_unit_test(osTestTimerBasic),
        cmocka_unit_test(osWritePermSuccess),
        cmocka_unit_test(osWritePermFailure),
        cmocka_unit_test(dbgHasNoUnexpectedFailures),
    };
    return cmocka_run_group_tests(tests, groupSetup, groupTeardown);
}
