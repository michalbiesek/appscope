
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <sys/user.h>
#include <unistd.h>
#include <string.h>
#include <link.h>
#include <errno.h>
#include <stdint.h>
#include <linux/limits.h>
#include <dlfcn.h>
#include <stddef.h>
#include "dbg.h"
#include "inject.h"

#define __RTLD_DLOPEN	0x80000000
#define PTR_ALIGN(val, align) (((val) + (align) - 1) & ~((align) - 1))

// Size of injected code segment
#define INJECTED_CODE_SIZE_LEN (512)

typedef struct {
    char *path;
    uint64_t addr;
} libdl_info_t;

#ifdef __x86_64__
    #define SP_REG regs.rsp
    #define IP_REG regs.rip
    #define FUNC_REG regs.rax
    #define FIRST_ARG_REG regs.rdi
    #define SECOND_ARG_REG regs.rsi
    #define RET_REG regs.rax
    #define DBG_TRAP "int $3 \n"
#elif defined(__aarch64__)
    #define SP_REG regs.sp
    #define IP_REG regs.pc
    #define FUNC_REG regs.regs[2]
    #define FIRST_ARG_REG regs.regs[0]
    #define SECOND_ARG_REG regs.regs[1]
    #define RET_REG regs.regs[0]
    #define DBG_TRAP "brk #0 \n"
#endif

static uint64_t
findLibrary(const char *library, pid_t pid)
{
    char filename[PATH_MAX];
    char buffer[9076];
    FILE *fd;
    uint64_t addr = 0;

    snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);
    if ((fd = fopen(filename, "r")) == NULL) {
        perror("fopen(/proc/PID/maps) failed");
        return 0;
    }

    while(fgets(buffer, sizeof(buffer), fd)) {
        if (strstr(buffer, library)) {
            addr = strtoull(buffer, NULL, 16);
            break;
        }
    }

    fclose(fd);
    return addr;
}

static uint64_t
freeSpaceAddr(pid_t pid)
{
    FILE *fd;
    char filename[PATH_MAX];
    char line[850];
    uint64_t addr;
    char str[20];
    char perms[5];

    sprintf(filename, "/proc/%d/maps", pid);
    if ((fd = fopen(filename, "r")) == NULL) {
        perror("fopen(/proc/PID/maps) failed");
        return 0;
    }

    while(fgets(line, 850, fd) != NULL) {
        sscanf(line, "%lx-%*x %s %*s %s %*d", &addr, perms, str);
        if ((strstr(perms, "x") != NULL) &&
            (strstr(perms, "w") == NULL)) {
            break;
        }
    }

    fclose(fd);
    return addr;
}

static int
ptraceRead(int pid, uint64_t addr, void *data, int len)
{
    int numRead = 0;
    int i = 0;
    long word = 0;
    long *ptr = (long *) data;

    while (numRead < len) {
        word = ptrace(PTRACE_PEEKTEXT, pid, addr + numRead, NULL);
        if(word == -1) {
            perror("ptrace(PTRACE_PEEKTEXT) failed");
            return EXIT_FAILURE;
        }
        numRead += sizeof(word);
        ptr[i++] = word;
    }

    return EXIT_SUCCESS;
}

static int
ptraceWrite(int pid, uint64_t addr, void *data, int len)
{
    long word = 0;
    int i = 0;

    for(i=0; i < len; i += sizeof(word), word=0) {
        memcpy(&word, data + i, sizeof(word));
        if (ptrace(PTRACE_POKETEXT, pid, addr + i, word) == -1) {
            perror("ptrace(PTRACE_POKETEXT) failed");
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

static int
ptraceAttach(pid_t target) {
    int waitpidstatus;

    if(ptrace(PTRACE_ATTACH, target, NULL, NULL) == -1) {
        perror("ptrace(PTRACE_ATTACH) failed");
        return EXIT_FAILURE;
    }

    if(waitpid(target, &waitpidstatus, WUNTRACED) != target) {
        perror("waitpid failed");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static void
call_dlopen(void)
{
#ifdef __x86_64__
    asm(
        "andq $0xfffffffffffffff0, %rsp \n" //align stack to 16-byte boundary
        "callq *%rax \n"
        DBG_TRAP
    );
#elif defined(__aarch64__)
    __asm__ volatile(
        "blr x2 \n"
        DBG_TRAP
    );
#endif
}

static void call_dlopen_end() {}

static int
inject(pid_t pid, uint64_t dlopenAddr, uint64_t otherAddr, char *path, int glibc)
{
    struct iovec my_iovec;
    struct user_regs_struct oldregs, regs;
    my_iovec.iov_len = sizeof(regs);
    unsigned char *oldcode;
    int status;
    pid_t pwait;
    uint64_t freeAddr, codeAddr;
    int libpathLen;

    if (ptraceAttach(pid)) {
        return EXIT_FAILURE;
    }

    // save registers
    my_iovec.iov_base = &oldregs;
    if (ptrace(PTRACE_GETREGSET, pid, (void*)NT_PRSTATUS, &my_iovec) == -1) {
        fprintf(stderr, "error: ptrace get register(), library could not be injected\n");
        return EXIT_FAILURE;
    }
    memcpy(&regs, &oldregs, sizeof(struct user_regs_struct));

    // find free space in text section
    freeAddr = freeSpaceAddr(pid);
    if (!freeAddr) {
        return EXIT_FAILURE;
    }
    
    void *ptr = NULL;
    posix_memalign(&ptr, 64, 64);
    memset(ptr, 0, 64);
    strcpy(ptr, "123");
    libpathLen = 64;
    oldcode = (unsigned char *)malloc(INJECTED_CODE_SIZE_LEN);
    if (ptraceRead(pid, freeAddr, oldcode, INJECTED_CODE_SIZE_LEN)) {
        return EXIT_FAILURE;
    }

    // write the puts param to the library 
    if (ptraceWrite(pid, freeAddr, ptr, libpathLen)) {
        return EXIT_FAILURE;
    }

    // inject the code right after the library path
    codeAddr = freeAddr + libpathLen + 1;
    if (ptraceWrite(pid, codeAddr, &call_dlopen, call_dlopen_end - call_dlopen)) {
        return EXIT_FAILURE;
    }

    // set instruction pointer to point to the injected code
    IP_REG = codeAddr;
    FUNC_REG = otherAddr;               // address of other - puts
    FIRST_ARG_REG = freeAddr;            // puts argument
#ifdef __aarch64__
    SP_REG = PTR_ALIGN(SP_REG, 64);
#endif


    my_iovec.iov_base = &regs;
    if (ptrace(PTRACE_SETREGSET, pid, (void *)NT_PRSTATUS, &my_iovec) == -1) {
        fprintf(stderr, "error: ptrace set register(), library could not be injected\n");
        return EXIT_FAILURE;
    }

    // continue execution and wait until the target process is stopped
    if (ptrace(PTRACE_CONT, pid, NULL, NULL) == -1) {
        fprintf(stderr, "error: ptrace continue(), library could not be injected\n");
        return EXIT_FAILURE;
    }
    waitpid(pid, &status, WUNTRACED);

    // if process has been stopped by SIGSTOP send SIGCONT signal along with PTRACE_CONT call
    if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGSTOP) {
        if (ptrace(PTRACE_CONT, pid, SIGCONT, NULL) == -1) {
            fprintf(stderr, "error: ptrace continue(), library could not be injected\n");
            return EXIT_FAILURE;
        }
        pwait = waitpid(pid, &status, WUNTRACED);
        fprintf(stderr, "waitpid return status %d\n", pwait);
    }

    // make sure the target process was stoppend by SIGTRAP triggered by DBG_TRAP
    if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP) {

        my_iovec.iov_base = &regs;
        // check if the library has been successfully injected
        if (ptrace(PTRACE_GETREGSET, pid, (void*)NT_PRSTATUS, &my_iovec) == -1) {
            fprintf(stderr, "error: ptrace get register(), library could not be injected\n");
            return EXIT_FAILURE;
        }
        if (RET_REG != 0x0) {
         //printf("Appscope library injected at %p\n", (void*)RET_REG);
        } else {
            fprintf(stderr, "error %p: dlopen() failed, library could not be injected\n", (void*)RET_REG);
        }
        //restore the app's state
        ptraceWrite(pid, freeAddr, oldcode, INJECTED_CODE_SIZE_LEN);
        my_iovec.iov_base = &oldregs;
        if (ptrace(PTRACE_SETREGSET, pid, (void *)NT_PRSTATUS, &my_iovec) == -1) {
            fprintf(stderr, "error: ptrace set register(), library could not be injected\n");
            return EXIT_FAILURE;
        }
        ptrace(PTRACE_DETACH, pid, NULL, NULL);

    } else {
        fprintf(stderr, "error: target process stopped with signal %d\n", WSTOPSIG(status));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int 
findLib(struct dl_phdr_info *info, size_t size, void *data)
{
    if (strstr(info->dlpi_name, "libc.so") != NULL ||
        strstr(info->dlpi_name, "ld-musl") != NULL) {
        char libpath[PATH_MAX];
        if (realpath(info->dlpi_name, libpath)) {
            ((libdl_info_t *)data)->path = libpath;
            ((libdl_info_t *)data)->addr = info->dlpi_addr;
            return 1;
        }
    }
    return 0;
}

int 
injectScope(int pid, char* path)
{
    uint64_t remoteLib, localLib;
    void *dlopenAddr = NULL;
    void *mallocAddr = NULL;
    void *raiseAddr = NULL;
    void *dupAddr = NULL;
    void *duptwoAddr = NULL;
    void *putcharAddr = NULL;
    void *putsAddr = NULL;

    libdl_info_t info;
    int glibc = TRUE;

    if (!dl_iterate_phdr(findLib, &info)) {
        fprintf(stderr, "error: failed to find local libc\n");
        return EXIT_FAILURE;
    }

    localLib = info.addr;
    dlopenAddr = dlsym(RTLD_DEFAULT, "__libc_dlopen_mode");
    if (dlopenAddr == NULL) {
        dlopenAddr = dlsym(RTLD_DEFAULT, "dlopen");
        glibc = FALSE;
    }

    if (dlopenAddr == NULL) {
        fprintf(stderr, "error: failed to find dlopen()\n");
        return EXIT_FAILURE;
    }

    mallocAddr = dlsym(RTLD_DEFAULT, "malloc");
    if (mallocAddr == NULL) {
        fprintf(stderr, "error: failed to find malloc()\n");
        return EXIT_FAILURE;
    }

    raiseAddr = dlsym(RTLD_DEFAULT, "raise");
    if (raiseAddr == NULL) {
        fprintf(stderr, "error: failed to find raise()\n");
        return EXIT_FAILURE;
    }

    dupAddr = dlsym(RTLD_DEFAULT, "dup");
    if (dupAddr == NULL) {
        fprintf(stderr, "error: failed to find dup()\n");
        return EXIT_FAILURE;
    }

    duptwoAddr = dlsym(RTLD_DEFAULT, "dup2");
    if (duptwoAddr == NULL) {
        fprintf(stderr, "error: failed to find dup2()\n");
        return EXIT_FAILURE;
    }

    putsAddr = dlsym(RTLD_DEFAULT, "puts");
    if (putsAddr == NULL) {
        fprintf(stderr, "error: failed to find puts()\n");
        return EXIT_FAILURE;
    }

    putcharAddr = dlsym(RTLD_DEFAULT, "putchar");
    if (putcharAddr == NULL) {
        fprintf(stderr, "error: failed to find putchar()\n");
        return EXIT_FAILURE;
    }

    // find the base address of libc in the target process
    remoteLib = findLibrary(info.path, pid);
    if (!remoteLib) {
        fprintf(stderr, "error: failed to find libc in target process\n");
        return EXIT_FAILURE;
    }

    // calculate the address of dlopen in the target process
    dlopenAddr = remoteLib + (dlopenAddr - localLib);
    mallocAddr = remoteLib + (mallocAddr - localLib);
    raiseAddr = remoteLib + (raiseAddr - localLib);
    dupAddr = remoteLib + (dupAddr - localLib);
    duptwoAddr = remoteLib + (duptwoAddr - localLib);
    putcharAddr = remoteLib + (putcharAddr - localLib);
    putsAddr = remoteLib + (putsAddr - localLib);
    // inject libscope.so into the target process
    return inject(pid, (uint64_t) dlopenAddr, (uint64_t) putsAddr, path, glibc);
}

