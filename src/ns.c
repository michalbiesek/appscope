#define _GNU_SOURCE
#include <fcntl.h>
#include <stdlib.h>

#include "ns.h"
#include "libver.h"
#include "setup.h"
#include "scopestdlib.h"

#define SCOPE_CRONTAB "* * * * * root /tmp/scope_att.sh\n"
/*
 * SCOPE_CRON_SCRIPT string format specifier
 * First parameter - absolute path to scope executable file
 * Second parameter - absolute path to accessible filter file
 */
#define SCOPE_CRON_SCRIPT "#! /bin/bash\ntouch /tmp/scope_test\nrm /etc/cron.d/scope_cron\n%s start -f < %s\n"
#define SCOPE_CRON_PATH "/etc/cron.d/scope_cron"
#define SCOPE_SCRIPT_PATH "/tmp/scope_att.sh"

/*
 * Extract file from parent namespace into child namespace.
 *
 * Returns TRUE in case of success, FALSE otherwise.
 */
static bool
extractMemToChildNamespace(char *inputMem, size_t inputSize, const char *outFile, mode_t outPermFlag) {
    bool status = FALSE;
    int outFd;

    if ((outFd = scope_open(outFile, O_RDWR | O_CREAT, outPermFlag)) == -1) {
        scope_perror("scope_open failed");
        return status;
    }

    if (scope_ftruncate(outFd, inputSize) != 0) {
        goto cleanupDestFd;
    }

    char *dest = scope_mmap(NULL, inputSize, PROT_READ | PROT_WRITE, MAP_SHARED, outFd, 0);
    if (dest == MAP_FAILED) {
        goto cleanupDestFd;
    }

    scope_memcpy(dest, inputMem, inputSize);

    scope_munmap(dest, inputSize);

    status = TRUE;

cleanupDestFd:

    scope_close(outFd);

    return status;
}

/*
 * Reassociate process identified with pid with a specific namespace described by ns.
 *
 * Returns TRUE if operation was success, FALSE otherwise.
 */
static bool
setNamespace(pid_t pid, const char *ns) {
    char nsPath[PATH_MAX] = {0};
    int nsFd;
    if (scope_snprintf(nsPath, sizeof(nsPath), "/proc/%d/ns/%s", pid, ns) < 0) {
        scope_perror("scope_snprintf failed");
        return FALSE;
    }

    if ((nsFd = scope_open(nsPath, O_RDONLY)) == -1) {
        scope_perror("scope_open failed");
        return FALSE;
    }

    if (scope_setns(nsFd, 0) != 0) {
        scope_perror("setns failed");
        return FALSE;
    }

    return TRUE;
}

/*
 * Joins the child PID and mount namespace.
 *
 * Returns TRUE if operation was success, FALSE otherwise.
 */
static bool
joinChildNamespace(pid_t hostPid) {
    bool status = FALSE;
    size_t ldscopeSize = 0;
    size_t cfgSize = 0;

    char path[PATH_MAX] = {0};

    if (scope_readlink("/proc/self/exe", path, sizeof(path) - 1) == -1) {
        return status;
    }

    char *ldscopeMem = setupLoadFileIntoMem(&ldscopeSize, path);
    if (ldscopeMem == NULL) {
        return status;
    }

    // Configuration is optional
    char *scopeCfgMem = setupLoadFileIntoMem(&cfgSize, getenv("SCOPE_CONF_PATH"));

    /*
    * Reassociate current process to the "child namespace"
    * - PID namespace - allows to "child process" of the calling process 
    *   be created in separate namespace
    *   In other words the calling process will not change it's ownPID
    *   namespace
    * - mount namespace - allows to copy file(s) into a "child namespace"
    */
    if (setNamespace(hostPid, "pid") == FALSE) {
        goto cleanupMem;
    }
    if (setNamespace(hostPid, "mnt") == FALSE) {
        goto cleanupMem;
    }

    const char *loaderVersion = libverNormalizedVersion(SCOPE_VER);

    scope_snprintf(path, PATH_MAX, "/usr/lib/appscope/%s/", loaderVersion);
    mkdir_status_t res = libverMkdirNested(path);
    if (res == MKDIR_STATUS_OTHER_ISSUE) {
        scope_snprintf(path, PATH_MAX, "/tmp/appscope/%s/", loaderVersion);
        mkdir_status_t res = libverMkdirNested(path);
        if (res == MKDIR_STATUS_OTHER_ISSUE) {
            goto cleanupMem;
        }
    }

    scope_strncat(path, "ldscope", sizeof("ldscope"));

    status = extractMemToChildNamespace(ldscopeMem, ldscopeSize, path, 0775);

    if (scopeCfgMem) {
        char scopeCfgPath[PATH_MAX] = {0};

        // extract scope.yml configuration
        scope_snprintf(scopeCfgPath, sizeof(scopeCfgPath), "/tmp/scope%d.yml", hostPid);
        status = extractMemToChildNamespace(scopeCfgMem, cfgSize, scopeCfgPath, 0664);
        // replace the SCOPE_CONF_PATH with namespace path
        setenv("SCOPE_CONF_PATH", scopeCfgPath, 1);
    }   

cleanupMem:

    scope_munmap(ldscopeMem, ldscopeSize);

    if (scopeCfgMem) {
        scope_munmap(scopeCfgMem, cfgSize);
    }

    return status;
}

/*
 * Check for PID in the child namespace.
 *
 * Returns TRUE if specific process contains two namespaces FALSE otherwise.
 */
bool
nsIsPidInChildNs(pid_t pid, pid_t *nsPid) {
    const int validNsDepth = 2;
    char path[PATH_MAX] = {0};
    char buffer[4096];
    bool status = FALSE;
    int lastNsPid = 0;
    int nsDepth = 0;

    if (scope_snprintf(path, sizeof(path), "/proc/%d/status", pid) < 0) {
        return FALSE;
    }

    FILE *fstream = scope_fopen(path, "r");

    if (fstream == NULL) {
        return FALSE;
    }

    while (scope_fgets(buffer, sizeof(buffer), fstream)) {
        if (scope_strstr(buffer, "NSpid:")) {
            const char delimiters[] = ": \t";
            char *entry, *last;

            entry = scope_strtok_r(buffer, delimiters, &last);
            // Skip NsPid string
            entry = scope_strtok_r(NULL, delimiters, &last);
            // Iterate over NsPids values
            while (entry != NULL) {
                lastNsPid = scope_atoi(entry);
                entry = scope_strtok_r(NULL, delimiters, &last);
                nsDepth++;
            }
            break;
        }
    }

    /*
    * TODO: we currently tested nesting depth 
    * equals validNsDepth, check more depth level
    */
    if (nsDepth == validNsDepth) {
        status = TRUE;
        *nsPid = lastNsPid;
    }

    scope_fclose(fstream);

    return status;
}

 /*
 * Setup the service for specified child process
 * Returns status of operation SERVICE_STATUS_SUCCESS in case of success, other values in case of failure
 */
service_status_t
nsService(pid_t pid, const char *serviceName) {

    if (setNamespace(pid, "mnt") == FALSE) {
        return SERVICE_STATUS_ERROR_OTHER;
    }

    return setupService(serviceName);
}

 
 /*
 * Configure the child mount namespace
 * - switch the mount namespace to child
 * - configure the setup
 * Returns status of operation 0 in case of success, other values in case of failure
 */
int
nsConfigure(pid_t pid, void *scopeCfgFilterMem, size_t filterFileSize) {
    if (setNamespace(pid, "mnt") == FALSE) {
        scope_fprintf(scope_stderr, "setNamespace mnt failed\n");
        return EXIT_FAILURE;
    }

    if (setupConfigure(scopeCfgFilterMem, filterFileSize)) {
        scope_fprintf(scope_stderr, "setup child namespace failed\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

 /*
 * Check if libscope.so is loaded in specified PID
 * Returns TRUE if library is loaded, FALSE otherwise.
 */
static bool
isLibScopeLoaded(pid_t pid)
{
    char mapsPath[PATH_MAX] = {0};
    char buffer[9076];
    FILE *fd;
    bool status = FALSE;

    if (scope_snprintf(mapsPath, sizeof(mapsPath), "/proc/%d/maps", pid) < 0) {
        return status;
    }

    if ((fd = scope_fopen(mapsPath, "r")) == NULL) {
        return status;
    }

    while (scope_fgets(buffer, sizeof(buffer), fd)) {
        if (scope_strstr(buffer, "libscope.so")) {
            status = TRUE;
            break;
        }
    }

    scope_fclose(fd);
    return status;
}

 
 /*
 * Perform fork and exec which cause that direct children
 * effectively will join a new PID namespace.
 *
 * Reassociating the PID namespace (setns CLONE_NEWPID) has somewhat different from 
 * other namespace types. Reassociating the calling thread with a PID namespace
 * changes only the PID namespace that subsequently created child processes of
 * the caller will be placed in. It does not change the PID namespace of the caller itself.
 *
 * Returns status of operation
 */
int
nsForkAndExec(pid_t parentPid, pid_t nsPid, char attachType)
{
    char *opStatus = "Detach";
    char *childOp = "-d";
    bool libLoaded = isLibScopeLoaded(parentPid);

    if (attachType == 'a') {
        childOp = "-a";
        opStatus = (libLoaded == FALSE) ? "Attach" : "Reattach";
    } else if (libLoaded == FALSE) {
        scope_fprintf(scope_stderr, "error: PID: %d has never been attached\n", parentPid);
        return EXIT_FAILURE; 
    }
     /*
    * TODO In case of Reattach/Detach - when libLoaded = TRUE
    * We only need the mount namespace to /dev/shm but currently ldscopedyn
    * also check the pid namespace
    */

    if (joinChildNamespace(parentPid) == FALSE) {
        scope_fprintf(scope_stderr, "error: join_namespace failed\n");
        return EXIT_FAILURE; 
    }

    pid_t child = fork();
    if (child < 0) {
        scope_fprintf(scope_stderr, "error: fork() failed\n");
        return EXIT_FAILURE;
    } else if (child == 0) {
        char loaderInChildPath[PATH_MAX] = {0};

        // Child
        char *nsAttachPidStr = NULL;
        if (scope_asprintf(&nsAttachPidStr, "%d", nsPid) <= 0) {
            scope_perror("error: asprintf() failed\n");
            return EXIT_FAILURE;
        }
        int execArgc = 0;
        char **execArgv = scope_calloc(4, sizeof(char *));
        if (!execArgv) {
            scope_fprintf(scope_stderr, "error: calloc() failed\n");
            return EXIT_FAILURE;
        }

        const char *loaderVersion = libverNormalizedVersion(SCOPE_VER);
        scope_snprintf(loaderInChildPath, PATH_MAX, "/usr/lib/appscope/%s/ldscope", loaderVersion);
        if (scope_access(loaderInChildPath, R_OK)) {
            scope_snprintf(loaderInChildPath, PATH_MAX, "/tmp/appscope/%s/ldscope", loaderVersion);
            if (scope_access(loaderInChildPath, R_OK)) {
                scope_fprintf(scope_stderr, "error: access ldscope failed\n");
                return EXIT_FAILURE;
            }
        }

        execArgv[execArgc++] = loaderInChildPath;
        execArgv[execArgc++] = childOp;
        execArgv[execArgc++] = nsAttachPidStr;

        return execve(loaderInChildPath, execArgv, environ);
    }
    // Parent
    int status;
    scope_waitpid(child, &status, 0);
    if (WIFEXITED(status)) {
        int exitChildStatus = WEXITSTATUS(status);
        if (exitChildStatus == 0) {
            scope_fprintf(scope_stderr, "%s to process %d in child process succeeded\n", opStatus, parentPid);
        } else {
            scope_fprintf(scope_stderr, "%s to process %d in child process failed\n", opStatus, parentPid);
        }
        return exitChildStatus;
    }
    scope_fprintf(scope_stderr, "error: %s failed() failed\n", opStatus);
    return EXIT_FAILURE;
}

/* Create the cron file
 *
 * When the start command is executed within a container we can't
 * set ns to that of a host process. Therefore, start a process in the
 * host context using crond. This process will run a script which will
 * run the start command in the context of the host. It should run once and
 * then clean up after itself.
 *
 * This should be called after the fs namespace has been switched.
 */
static bool
createCron(void) {
    int outFd;
    char buf[1024];
    char path[PATH_MAX] = {0};
    char scopePath[PATH_MAX] = {0};
    char filterPath[PATH_MAX] = "/usr/lib/appscope/scope_filter";

    // Create the script to be executed by cron
    if (scope_snprintf(path, sizeof(path), SCOPE_SCRIPT_PATH) < 0) {
        scope_perror("createCron: script path: error: snprintf() failed\n");
        scope_fprintf(scope_stdout, "path: %s\n", path);
        return FALSE;
    }

    if ((outFd = scope_open(path, O_RDWR | O_CREAT, 0775)) == -1) {
        scope_perror("createCron: script path: scope_open failed");
        scope_fprintf(scope_stdout, "path: %s\n", path);
        return FALSE;
    }

    // Verify the path to a scope
    const char *loaderVersion = libverNormalizedVersion(SCOPE_VER);
    scope_snprintf(scopePath, PATH_MAX, "/usr/lib/appscope/%s/scope", loaderVersion);
    if (scope_access(scopePath, R_OK)) {
        scope_snprintf(scopePath, PATH_MAX, "/tmp/appscope/%s/scope", loaderVersion);
        if (scope_access(scopePath, R_OK)) {
            scope_fprintf(scope_stdout, "createCron: error access scope file: %s\n", scopePath);
            return FALSE;
        }
    }

    // Verify path to a filter file
    if (scope_access(filterPath, R_OK)) {
        scope_snprintf(filterPath, PATH_MAX, "/tmp/scope_filter");
        if (scope_access(filterPath, R_OK)) {
            scope_fprintf(scope_stdout, "createCron: error access filter file: %s\n", path);
            return FALSE;
        }
    }

    if (scope_snprintf(buf, sizeof(buf), SCOPE_CRON_SCRIPT, scopePath, filterPath) < 0) {
        scope_perror("createCron: script: error: snprintf() failed\n");
        scope_close(outFd);
        return FALSE;
    }

    if (scope_write(outFd, buf, scope_strlen(buf)) == -1) {
        scope_perror("createCron: script: scope_write failed");
        scope_fprintf(scope_stdout, "path: %s\n", path);
        scope_close(outFd);
        return FALSE;
    }

    if (scope_close(outFd) == -1) {
        scope_perror("createCron: script: scope_close failed");
        scope_fprintf(scope_stdout, "path: %s\n", path);
        return FALSE;
    }

    // Create the cron entry
    if (scope_snprintf(path, sizeof(path), SCOPE_CRON_PATH) < 0) {
        scope_perror("createCron: cron: error: snprintf() failed\n");
        scope_fprintf(scope_stdout, "path: %s\n", path);
        return FALSE;
    }

    if (scope_snprintf(buf, sizeof(buf), SCOPE_CRONTAB) < 0) {
        scope_perror("createCron: cron: error: snprintf() failed\n");
        scope_fprintf(scope_stdout, "path: %s\n", path);
        return FALSE;
    }

    if ((outFd = scope_open(path, O_RDWR | O_CREAT, 0775)) == -1) {
        scope_perror("createCron: cron: scope_open failed");
        scope_fprintf(scope_stdout, "path: %s\n", path);
        return FALSE;
    }

    // crond will detect this file entry and run on its' next cycle
    if (scope_write(outFd, buf, scope_strlen(buf)) == -1) {
        scope_perror("createCron: cron: scope_write failed");
        scope_fprintf(scope_stdout, "path: %s\n", path);
        scope_close(outFd);
        return FALSE;
    }

    if (scope_close(outFd) == -1) {
        scope_perror("createCron: cron: scope_close failed");
        scope_fprintf(scope_stdout, "path: %s\n", path);
        return FALSE;
    }

    return TRUE;

}

static bool
setHostNamespace(const char *ns) {
    int nsFd;
    char *nsPp;
    char procPath[64];
    char nsPath[PATH_MAX] = {0};

    // $CRIBL_EDGE_FS_ROOT else /hostfs
    if ((nsPp = getenv("CRIBL_EDGE_FS_ROOT"))) {
        scope_strncpy(nsPath, nsPp, sizeof(nsPath));
    } else {
        scope_strncpy(nsPath, "/hostfs", sizeof("/hostfs"));
    }

    if (scope_snprintf(procPath, sizeof(procPath), "/proc/1/ns/%s", ns) < 0) {
        scope_perror("setHostNamespace: scope_snprintf failed");
        return FALSE;
    }

    scope_strncat(nsPath, procPath, sizeof(procPath) - 1);

    if ((nsFd = scope_open(nsPath, O_RDONLY)) == -1) {
        scope_perror("setHostNamespace: scope_open failed: host fs is not mounted:");
        return FALSE;
    }

    if (scope_setns(nsFd, 0) != 0) {
        scope_perror("setHostNamespace: setns failed");
        return FALSE;
    }

    // TODO: should we close here?
    return TRUE;
}

static bool
joinHostNamespace(const char *filterPath) {
    bool status = FALSE;
    size_t ldscopeSize = 0;
    size_t filterCfgSize = 0;
    size_t scopeSize = 0;
    char path[PATH_MAX] = {0};

    if (scope_readlink("/proc/self/exe", path, sizeof(path) - 1) == -1) {
        return status;
    }

    // Load the "ldscope" into memory
    char *ldscopeMem = setupLoadFileIntoMem(&ldscopeSize, path);
    if (ldscopeMem == NULL) {
        return status;
    }

    // Load the filter configuration into memory
    char *scopeFilterCfgMem = setupLoadFileIntoMem(&filterCfgSize, filterPath);
    if (scopeFilterCfgMem == NULL) {
        goto cleanupMem;
    }

    // Find the scope binary on the container
    const char *loaderVersion = libverNormalizedVersion(SCOPE_VER);
    scope_snprintf(path, PATH_MAX, "/usr/lib/appscope/%s/scope", loaderVersion);
    if (scope_access(path, R_OK)) {
        scope_snprintf(path, PATH_MAX, "/tmp/appscope/%s/scope", loaderVersion);
        if (scope_access(path, R_OK)) {
            goto cleanupMem;
        }
    }

    // Load the "scope" into memory
    char *scopeMem = setupLoadFileIntoMem(&scopeSize, path);
    if (scopeMem == NULL) {
        goto cleanupMem;
    }

    /*
     * Reassociate current process to the host namespace
     * - mount namespace - allows to copy file(s) into the host fs
     */
    if (setHostNamespace("mnt") == FALSE) {
        goto cleanupMem;
    }

    /*
     * At this point we are using the host fs.
     * Ensure that we have the dest dir
     */

    // Ensure that we have base directory available on host
    scope_snprintf(path, PATH_MAX, "/usr/lib/appscope/%s/", loaderVersion);
    mkdir_status_t res = libverMkdirNested(path);
    if (res == MKDIR_STATUS_OTHER_ISSUE) {
        scope_snprintf(path, PATH_MAX, "/tmp/appscope/%s/", loaderVersion);
        mkdir_status_t res = libverMkdirNested(path);
        if (res == MKDIR_STATUS_OTHER_ISSUE) {
            goto cleanupMem;
        }
    }

    // Ensure that we have access on base directory to create a ldscope file
    scope_snprintf(path, PATH_MAX, "/usr/lib/appscope/%s", loaderVersion);
    if (scope_access(path, R_OK)) {
        scope_snprintf(path, PATH_MAX, "/tmp/appscope/%s", loaderVersion);
        if (scope_access(path, R_OK)) {
            scope_fprintf(scope_stderr, "error: access appscope directory failed\n");
            goto cleanupMem;
        }
    }
    scope_strncat(path, "ldscope", sizeof("ldscope"));
    if ((status = extractMemToChildNamespace(ldscopeMem, ldscopeSize, path, 0775)) == FALSE) {
        goto cleanupMem;
    }


    // Ensure that we have access on base directory to access filter file
    scope_snprintf(path, PATH_MAX, "/usr/lib/appscope/");
    if (scope_access(path, R_OK)) {
        scope_snprintf(path, PATH_MAX, "/tmp/appscope/");
        if (scope_access(path, R_OK)) {
            scope_fprintf(scope_stderr, "error: access appscope directory failed\n");
            goto cleanupMem;
        }
    }

    if ((status == extractMemToChildNamespace(scopeFilterCfgMem, filterCfgSize, path, 0664)) == FALSE) {
         goto cleanupMem;
    }

    scope_snprintf(path, PATH_MAX, "/usr/lib/appscope/%s/", loaderVersion);
    if (scope_access(path, R_OK)) {
        scope_snprintf(path, PATH_MAX, "/tmp/appscope/%s/", loaderVersion);
        if (scope_access(path, R_OK)) {
            scope_fprintf(scope_stderr, "error: access appscope directory failed\n");
            goto cleanupMem;
        }
    }
    scope_strncat(path, "scope", sizeof("scope"));
    status = extractMemToChildNamespace(scopeMem, scopeSize, path, 0775);

cleanupMem:

    scope_munmap(ldscopeMem, ldscopeSize);

    if (scopeFilterCfgMem) scope_munmap(scopeFilterCfgMem, filterCfgSize);

    if (scopeMem) scope_munmap(scopeMem, scopeSize);

    return status;
}

 /*
 * Perform ldsope host start operation - this operation begins from container namespace.
 *
 * - switch namespace to host
 * - create cron entry with filter file
 *
 * Returns exit code of operation
 */
int
nsHostStart(const char* filterPath) {
    scope_fprintf(scope_stdout, "Executing from a container, run the start command from the host\n");

    if (joinHostNamespace(filterPath) == FALSE) {
        scope_fprintf(scope_stderr, "error: joinHostNamespace failed\n");
        return EXIT_FAILURE;
    }

    createCron();

    return EXIT_SUCCESS;
}
