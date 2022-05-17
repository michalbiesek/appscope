#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test.h"
#include "scopestdlib.h"

static int std_callback(struct dl_phdr_info *info, size_t size, void *data) {
    fprintf(stderr, "std library called %s\n", info->dlpi_name);
    return 0;
}

static int callback(struct dl_phdr_info *info, size_t size, void *data) {
    scope_fprintf(scope_stderr, "scope library called %s\n", info->dlpi_name);
    return 0;
}

static void
vdso_functions_before_init(void **state)
{
    struct timespec ts;
    scope_clock_gettime(CLOCK_MONOTONIC, &ts);
    scope_sched_getcpu();
}

static void
vdso_functions_after_init(void **state)
{
    scope_init_aux_val();
    struct timespec ts;
    scope_clock_gettime(CLOCK_MONOTONIC, &ts);
    scope_sched_getcpu();
    dl_iterate_phdr(std_callback, NULL);
    scope_dl_iterate_phdr(callback, NULL);
}

int
main(int argc, char* argv[])
{
    printf("running %s\n", argv[0]);

    const struct CMUnitTest tests[] = {
        cmocka_unit_test(vdso_functions_before_init),
        cmocka_unit_test(vdso_functions_after_init),
        cmocka_unit_test(dbgHasNoUnexpectedFailures),
    };
    return cmocka_run_group_tests(tests, groupSetup, groupTeardown);
}
