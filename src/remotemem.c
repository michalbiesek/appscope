#define _GNU_SOURCE

#include "remotemem.h"
#include "vector.h"
#include "scopeelf.h"

// memory library structure
typedef vector mem_lib_t;

typedef struct {
    uint64_t vAddrBegin;        // virtual address begin
    uint64_t vAddrSize;         // virtual address size
    void *   buf;               // buffer
} mem_region_t;

 /*
 * Creates the memory region structure based on begin and end arguments.
 *
 * Returns memory region, NULL on failure.
 */
static mem_region_t *
memRegionCreate(uint64_t begin, uint64_t end) {
    mem_region_t *region = scope_calloc(1, sizeof(mem_region_t));
    if (!region) {
        return NULL;
    }
    region->vAddrBegin = begin;
    region->vAddrSize = end - begin;
    region->buf = scope_malloc(sizeof(char) * (region->vAddrSize));
    if (!region->buf) {
        scope_free(region);
        return NULL;
    }

    return region;
}

 /*
 * Destroys memory region.
 */
static void
memRegionDestroy(mem_region_t *region) {
    scope_free(region->buf);
    scope_free(region);
}

 /*
 * Initialize memory library.
 */
static bool
memLibInit(mem_lib_t *memLib) {
    return vecInit(memLib);
}

 /*
 * Destroys memory library.
 */
static void
memLibMapDestroy(mem_lib_t *memLib) {
    for (unsigned i = 0; i < memLib->size; ++i) {
        memRegionDestroy(vecGet(memLib, i));
    }
    vecDelete(memLib);
}

 /*
 * Get specific region in the memory library.
 */
static mem_region_t *
memLibGetRegion(mem_lib_t *memLib, unsigned indx) {
    return vecGet(memLib, indx);
}

 /*
 * Append the memory region to library structure on begin and end arguments.
 *
 * Returns status of operation - TRUE on success, FALSE on failure.
 */
static bool
memLibAppendRegion(mem_lib_t *memLib, uint64_t regionBegin, uint64_t regionEnd) {
    // create new memory region 
    mem_region_t *region = memRegionCreate(regionBegin, regionEnd);
    if (!region) {
        return FALSE;
    }

    return vecAdd(memLib, region);
}

 /*
 * Parse specific memory map file and allocate memmory for library mapping.
 *
 * Returns status of operation - TRUE on success, FALSE on failure.
 */
static bool
memoryLibMap(pid_t pid, mem_lib_t *memLib) {
    char filename[PATH_MAX];
    char line[1024];
    FILE *fd;

    scope_snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);
    if ((fd = scope_fopen(filename, "r")) == NULL) {
        return FALSE;
    }

    while (scope_fgets(line, sizeof(line), fd)) {
        uint64_t vAddrBegin = 0;
        uint64_t vAddrEnd = 0;
        char pathFromLine[512] = {0};
        char perms[5] = {0};
        if ((scope_sscanf(line, "%lx-%lx %s %*x %*s %*d %512s", &vAddrBegin, &vAddrEnd, perms, pathFromLine) == 4)) {
            // scope_fprintf(scope_stderr, "\nBegin: %lx End: %lx lib: %s", vAddrBegin, vAddrEnd, pathFromLine);
            if ((!scope_strstr(pathFromLine, "/libc-")) && (!scope_strstr(pathFromLine, "/ld-musl-"))) {
                continue;
            }

            // Add only readable segment
            if (!scope_strstr(perms, "r")) {
                continue;
            }

            if (memLibAppendRegion(memLib, vAddrBegin, vAddrEnd) == FALSE) {
                scope_fclose(fd);
                return FALSE;
            }
        }
    }
    scope_fclose(fd);
    return TRUE;
}

 /*
 * Copy virtual mapping from remote process
 *
 * Returns status of operation - TRUE on success, FALSE on failure.
 */
static bool
memoryLibRead(pid_t pid, mem_lib_t *memLib) {
    bool status = FALSE;
    ssize_t vmReadBytes = 0;
    size_t libSizeBytes = 0;

    struct iovec *localIov = scope_calloc(memLib->size, sizeof(struct iovec));
    if (!localIov) {
        return status;
    }
    struct iovec *remoteIov = scope_calloc(memLib->size, sizeof(struct iovec));
    if (!remoteIov) {
        goto freeLocalIov;
    }

    for (unsigned i = 0; i < memLib->size; ++i) {
        mem_region_t *reg = memLibGetRegion(memLib, i);

        localIov[i].iov_base = reg->buf;
        localIov[i].iov_len = reg->vAddrSize;
        remoteIov[i].iov_base = (void*) reg->vAddrBegin;
        remoteIov[i].iov_len = reg->vAddrSize;
        libSizeBytes += reg->vAddrSize;
    }

    vmReadBytes = scope_process_vm_readv(pid, localIov, memLib->size, remoteIov, memLib->size, 0);
    if (vmReadBytes < 0) {
        goto freeRemoteIov;
    }

    if (vmReadBytes < 0) {
        goto freeRemoteIov;
    }

    if (libSizeBytes != vmReadBytes) {
        goto freeRemoteIov;
    }

    status = TRUE;

freeRemoteIov:
    scope_free(remoteIov);
   
freeLocalIov:
    scope_free(localIov);
    return status;
}

 /*
 * Resolve virtual address against the mapped library
 *
 * Returns status of operation - TRUE on success, FALSE on failure.
 */
static void*
memoryLibMapLocateVirtAddr(uint64_t vAddr, mem_lib_t *memLib) {
    for (unsigned i = 0; i < memLib->size; ++i) {
        mem_region_t *reg = memLibGetRegion(memLib, i);
        // check if virtual address belongs to the region
        if (reg->vAddrBegin <= vAddr && vAddr < (reg->vAddrBegin + reg->vAddrSize)) {
            return reg->buf + (vAddr - reg->vAddrBegin);
        }  
    }
    return NULL;
}

static uint64_t
findFunctionAddress(mem_lib_t *memLib) {
    uint64_t funcAddr = 0;
    // Find the dynamic section address
    mem_region_t *reg = memLibGetRegion(memLib, 0);
    const Elf64_Ehdr *elf = (Elf64_Ehdr *)reg->buf; 
    Elf64_Phdr *phead = (Elf64_Phdr *)&reg->buf[elf->e_phoff];
    // unsigned long count = elf->e_phnum;
    // Elf64_Dyn *dyn = NULL;

    // for (int i = 0; i < elf->e_phnum; ++i) {
    //     if ((phead[i].p_type == PT_DYNAMIC)) {
    //         dyn = phead[i].p_vaddr + reg->buf;
    //     }
    // }

    // // Dynamic section not found
    // if (!dyn) {
    //     return funcAddr;
    // }


    // funcAddr = symbols.st_value + reg->buf;


    return funcAddr;
}


 /*
 * Find the addresss of specific symbol in remote process
 *
 * Returns address of the symbol, NULL on failure.
 */
uint64_t
remoteProcSymbolAddr(pid_t pid, const char *symbolName) {
    uint64_t symAddr = 0;
    mem_lib_t memLib;

    if (memLibInit(&memLib) == FALSE) {
        return symAddr;
    }

    if (memoryLibMap(pid, &memLib) == FALSE) {
        scope_fprintf(scope_stderr, "\nmemoryLibMap failed");
        goto destroyLib;
    }

    if (memoryLibRead(pid, &memLib) == FALSE) {
        scope_fprintf(scope_stderr, "\nmemoryLibRead failed");
        goto destroyLib;
    }

    symAddr = findFunctionAddress(&memLib);

destroyLib:
    memLibMapDestroy(&memLib);
    return symAddr;
}
