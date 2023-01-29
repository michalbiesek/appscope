#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test.h"
#include "scopestdlib.h"

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
    scope_init_vdso_ehdr();
    struct timespec ts;
    scope_clock_gettime(CLOCK_MONOTONIC, &ts);
    scope_sched_getcpu();
}
void *p;
void *ps;

int
main(int argc, char* argv[])
{
    printf("running %s\n", argv[0]);
    p = malloc(7);
    p = 0; // The memory is leaked here.
    ps = scope_malloc(7);
    ps = 0; // The memory is leaked here.

    // const struct CMUnitTest tests[] = {
    //     cmocka_unit_test(vdso_functions_before_init),
    //     cmocka_unit_test(vdso_functions_after_init),
    //     cmocka_unit_test(dbgHasNoUnexpectedFailures),
    // };
    return 0;
}
