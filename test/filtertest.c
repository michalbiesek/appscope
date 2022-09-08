#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>

#include "filter.h"
#include "scopestdlib.h"
#include "dbg.h"
#include "test.h"

static char dirPath[PATH_MAX];

static int
appPath(char *path, const char *argv0) {
    char buf[PATH_MAX];
    if (argv0[0] == '/') {
        scope_strcpy(buf, argv0);
    } else {
        if (scope_getcwd(buf, PATH_MAX) == NULL) {
            scope_perror("getcwd error");
            return -1;
        }
        scope_strcat(buf, "/");
        scope_strcat(buf, argv0);
    }

    if (scope_realpath(buf, path) == NULL) {
        scope_perror("scope_realpath error");
        return -1;
    }
    // /<dir>/appscope/test/linux/filtertest
    path = scope_dirname(path);
    if (path == NULL) {
        scope_perror("scope_dirname error");
        return -1;
    }
    // Remove os directory (linux)
    path = scope_dirname(path);
    if (path == NULL) {
        scope_perror("scope_dirname error");
        return -1;
    }
    return 0;
}

static void
filterEmptyProc(void **state) {
    char path[PATH_MAX] = {0};
    scope_snprintf(path, sizeof(path), "%s/data/filter_0.yml", dirPath);
    bool res = filterVerifyProc(NULL, path);
    assert_false(res);
    dbgInit(); // reset dbg for the rest of the tests
}

static void
filterNullFilterPath(void **state) {
    bool res = filterVerifyProc("foo", NULL);
    assert_true(res);
}

static void
filterNonExistingFilterFile(void **state) {
    char path[PATH_MAX] = {0};
    scope_snprintf(path, sizeof(path), "%s/data/filter_non_existing.yml", dirPath);
    bool res = filterVerifyProc("foo", path);
    assert_true(res);
}

static void
filterProcNameMissing(void **state) {
    char path[PATH_MAX] = {0};
    scope_snprintf(path, sizeof(path), "%s/data/filter_0.yml", dirPath);
    bool res = filterVerifyProc("foo", path);
    assert_false(res);
}

static void
filterProcNameAllowListPresent(void **state) {
    char path[PATH_MAX] = {0};
    scope_snprintf(path, sizeof(path), "%s/data/filter_0.yml", dirPath);
    bool res = filterVerifyProc("redis", path);
    assert_true(res);
}

static void
filterProcNameDenyListPresent(void **state) {
    char path[PATH_MAX] = {0};
    scope_snprintf(path, sizeof(path), "%s/data/filter_0.yml", dirPath);
    bool res = filterVerifyProc("git", path);
    assert_false(res);
}

static void
filterArgAllowListPresent(void **state) {
    char path[PATH_MAX] = {0};
    scope_snprintf(path, sizeof(path), "%s/data/filter_0.yml", dirPath);
    bool res = filterVerifyProc("redis", path);
    assert_true(res);
}

static void
filterArgDenyListPresent(void **state) {
    char path[PATH_MAX] = {0};
    scope_snprintf(path, sizeof(path), "%s/data/filter_0.yml", dirPath);
    bool res = filterVerifyProc("git", path);
    assert_false(res);
}

static void
filterArgAllowListPartFindPresent(void **state) {
    char path[PATH_MAX] = {0};
    scope_snprintf(path, sizeof(path), "%s/data/filter_0.yml", dirPath);
    bool res = filterVerifyProc(" redis-server", path);
    assert_false(res);
    scope_memset(path, 0, sizeof(path));
    scope_snprintf(path, sizeof(path), "%s/data/filter_1.yml", dirPath);
    res = filterVerifyProc(" redis-server", path);
    assert_true(res);
}

int
main(int argc, char* argv[])
{
    printf("running %s\n", argv[0]);
    if (appPath(dirPath, argv[0])) {
        return EXIT_FAILURE;
    }
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(filterEmptyProc),
        cmocka_unit_test(filterNullFilterPath),
        cmocka_unit_test(filterNonExistingFilterFile),
        cmocka_unit_test(filterProcNameMissing),
        cmocka_unit_test(filterProcNameAllowListPresent),
        cmocka_unit_test(filterProcNameDenyListPresent),
        cmocka_unit_test(filterArgAllowListPresent),
        cmocka_unit_test(filterArgDenyListPresent),
        cmocka_unit_test(filterArgAllowListPartFindPresent),
        cmocka_unit_test(dbgHasNoUnexpectedFailures),
    };
    return cmocka_run_group_tests(tests, groupSetup, groupTeardown);
}
