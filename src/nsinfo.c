#define _GNU_SOURCE

#include "nsinfo.h"


/* Return root uid inside the namespace for specified pid.
 *
 * Return root uid
 */
uid_t
nsGetRootUid(pid_t hostPid) {
    uid_t uid = 0;
    char uidPath[PATH_MAX] = {0};
    char buffer[4096] = {0};
    FILE *fd;

    if (scope_snprintf(uidPath, sizeof(uidPath), "/proc/%d/uid_map", hostPid) < 0) {
        scope_perror("scope_snprintf uid_map failed");
        return uid;
    }
    if ((fd = scope_fopen(uidPath, "r")) == NULL) {
        scope_perror("fopen(/proc/<PID>/uid_map) failed");
        return uid;
    }

    while (scope_fgets(buffer, sizeof(buffer), fd)) {
        const char delimiters[] = " \t";
        char *last;

        char *entry = scope_strtok_r(buffer, delimiters, &last);
        uid_t uidInsideNs = scope_atoi(entry);

        entry = scope_strtok_r(NULL, delimiters, &last);
        uid_t uidOutsideNs = scope_atoi(entry);
        if ((uidInsideNs == 0) && (uidInsideNs != uidOutsideNs)) {
            uid = uidOutsideNs;
            break;
        }
    }
    scope_fclose(fd);

    return uid;
}

/* Return root gid inside the namespace for specified pid.
 *
 * Return root gid
 */
gid_t
nsGetRootGid(pid_t hostPid) {
    gid_t gid = 0;
    char gidPath[PATH_MAX] = {0};
    char buffer[4096] = {0};
    FILE *fd;

    if (scope_snprintf(gidPath, sizeof(gidPath), "/proc/%d/gid_map", hostPid) < 0) {
        scope_perror("scope_snprintf gid_map failed");
        return gid;
    }
    if ((fd = scope_fopen(gidPath, "r")) == NULL) {
        scope_perror("fopen(/proc/<PID>/gid_map) failed");
        return gid;
    }

    while (scope_fgets(buffer, sizeof(buffer), fd)) {
        const char delimiters[] = " \t";
        char *last;

        char *entry = scope_strtok_r(buffer, delimiters, &last);
        gid_t gidInsideNs = scope_atoi(entry);

        entry = scope_strtok_r(NULL, delimiters, &last);
        gid_t gidOutsideNs = scope_atoi(entry);
        if ((gidInsideNs == 0) && (gidInsideNs != gidOutsideNs)) {
            gid = gidOutsideNs;
            break;
        }
    }
    scope_fclose(fd);

    return gid;
}

/*
 * Check for if the specified process exists in same mnt namespace as current process.
 *
 * Returns TRUE if specified process exists in same mnt namespace as current process, FALSE otherwise.
 */
bool
nsIsPidInSameMntNs(pid_t pid) {
    char path[PATH_MAX] = {0};
    struct stat selfStat = {0};
    struct stat targetStat = {0};

    if (scope_stat("/proc/self/ns/mnt", &selfStat) != 0) {
        scope_perror("scope_stat(/proc/self/ns/mnt) failed");
        return TRUE;
    }

    if (scope_snprintf(path, sizeof(path), "/proc/%d/ns/mnt", pid) < 0) {
        scope_perror("scope_snprintf(/proc/<pid>>/ns/mnt) failed");
        return TRUE;
    }

    if (scope_stat(path, &targetStat) != 0) {
        scope_perror("scope_stat(/proc/<pid>>/ns/mnt) failed");
        return TRUE;
    }

    /*
    * If two processes are in the same mount namespace, then the device IDs and
    * inode numbers of their /proc/[pid]/ns/mnt symbolic links will be the same.
    */
    return (selfStat.st_dev == targetStat.st_dev) && (selfStat.st_ino == targetStat.st_ino);
}

/*
 * Check for PID in the proces namespace.
 *
 * Returns TRUE if specific process contains two namespaces FALSE otherwise.
 */
bool
nsInfoIsPidInChildNs(pid_t pid, pid_t *nsPid) {
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
