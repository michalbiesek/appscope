#define _GNU_SOURCE

#include "remotemem.h"
#include "vector.h"
#include "scopeelf.h"

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

static bool
memLibMapCreate(vector *memLibMap) {
    return vecInit(memLibMap);
}

static void
memLibMapDestroy(vector *memLibMap) {
    for (unsigned i = 0; i < memLibMap->size; ++i) {
        memRegionDestroy(vecGet(memLibMap, i));
    }

    vecDelete(memLibMap);
}

 /*
 * Append the memory region to library structure on begin and end arguments.
 *
 * Returns status of operation - TRUE on success, FALSE on failure.
 */
static bool
memLibAppendRegion(vector *memLibMap, uint64_t regionBegin, uint64_t regionEnd) {
    // create new memory region 
    mem_region_t *region = memRegionCreate(regionBegin, regionEnd);
    if (!region) {
        return FALSE;
    }

    return vecAdd(memLibMap, region);
}

 /*
 * Parse specific memory map file and allocate memmory for library mapping.
 *
 * Returns status of operation - TRUE on success, FALSE on failure.
 */
static bool
memoryLibMap(pid_t pid, vector *memLibMap) {
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

            if (memLibAppendRegion(memLibMap, vAddrBegin, vAddrEnd) == FALSE) {
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
memoryLibRead(pid_t pid, vector *memLibMap) {
    bool status = FALSE;
    ssize_t vmReadBytes = 0;
    size_t libSizeBytes = 0;

    struct iovec *localIov = scope_calloc(memLibMap->size, sizeof(struct iovec));
    if (!localIov) {
        return status;
    }
    struct iovec *remoteIov = scope_calloc(memLibMap->size, sizeof(struct iovec));
    if (!remoteIov) {
        goto freeLocalIov;
    }

    for (unsigned i = 0; i < memLibMap->size; ++i) {
        mem_region_t *reg = vecGet(memLibMap, i);

        localIov[i].iov_base = reg->buf;
        localIov[i].iov_len = reg->vAddrSize;
        remoteIov[i].iov_base = (void*) reg->vAddrBegin;
        remoteIov[i].iov_len = reg->vAddrSize;
        libSizeBytes += reg->vAddrSize;
    }

    vmReadBytes = scope_process_vm_readv(pid, localIov, memLibMap->size, remoteIov, memLibMap->size, 0);
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

static void
findDynamicSegment(vector *memLibMap) {
    mem_region_t *reg = vecGet(memLibMap, 0);
    Elf64_Ehdr *elf = (Elf64_Ehdr *)reg->buf;
    Elf64_Phdr *phead = (Elf64_Phdr *)&reg->buf[elf->e_phoff];

    return;
}

 /*
 * Find the addresss of specific symbol in remote process
 *
 * Returns address of the symbol, NULL on failure.
 */
uint64_t
remoteProcSymbolAddr(pid_t pid, const char *symbolName) {
    uint64_t symAddr = 0;
    vector memLibMap;

    if (memLibMapCreate(&memLibMap) == FALSE) {
        return symAddr;
    }

    if (memoryLibMap(pid, &memLibMap) == FALSE) {
        scope_fprintf(scope_stderr, "\nmemoryLibMap failed");
        goto destroyLib;
    }

    if (memoryLibRead(pid, &memLibMap) == FALSE) {
        scope_fprintf(scope_stderr, "\nmemoryLibRead failed");
        goto destroyLib;
    }

    findDynamicSegment(&memLibMap);

    // symbol_table_t *libSymTable = findDynamicSymbolTable(&memLibMap);
    // if (!libSymTable) {
    //     scope_fprintf(scope_stderr, "\nfindDynamicSymbolTable failed");
    //     goto destroyLib;
    // }

    // symAddr = resolveSymbol(symbolName, libSymTable);

    // if (symAddr == 0) {
    //     scope_fprintf(scope_stderr, "\nresolveSymbol failed");
    // }

    // free_symbol_table(libSymTable);

destroyLib:
    memLibMapDestroy(&memLibMap);
    return symAddr;
}
