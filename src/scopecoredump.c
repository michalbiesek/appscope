#define _GNU_SOURCE

#include "scopecoredump.h"
#include "scopestdlib.h"
#include "utils.h"
#include "google/coredumper.h"

// Prefix for core dump file
#define CORE_PREFIX "/tmp/scope_core."

#define UNW_REMOTE_ONLY
#include "libunwind.h"
#include "libunwind-coredump.h"

/*
 * Generates core dump in location based on pid
 * Return status of operation
 */
bool
scopeCoreDumpGenerate(pid_t pid) {
    char path[PATH_MAX] = {0};
    scope_memcpy(path, CORE_PREFIX, sizeof(CORE_PREFIX) - 1);
    char pidBuf[32] = {0};
    int msgLen = 0;
    sigSafeUtoa(pid, pidBuf, 10, &msgLen);
    scope_memcpy(path + sizeof(CORE_PREFIX) - 1, pidBuf, msgLen);
    return (WriteCoreDump(path) == 0);
}

bool
scopeUnwindCoreDumpGenerate(void) {
    bool res = FALSE;
    unw_addr_space_t as;
    struct UCD_info *ui;
    unw_cursor_t c;
    as = unw_create_addr_space(&_UCD_accessors, 0);
    if (!as) {
        goto end;
    }
    ui = _UCD_create("/proc/self/exe");
    if (!ui) {
        unw_destroy_addr_space(as);
        goto end;
    }

    int ret = unw_init_remote(&c, as, ui);
    if (ret < 0) {
        _UCD_destroy(ui);
        unw_destroy_addr_space(as);
        goto end;
    }
    unw_proc_info_t pi;
    unw_get_proc_info(&c, &pi);
    if (ret < 0) {
        _UCD_destroy(ui);
        unw_destroy_addr_space(as);
        goto end;
    }

    _UCD_destroy(ui);
    unw_destroy_addr_space(as);

    res = TRUE;

end:
    return res;
}