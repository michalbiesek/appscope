#define _GNU_SOURCE

#include "scopestdlib.h"

#include <limits.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

//TODO make this atomic
static uint64_t alloc_size;

// Memory management handling operations

// Memory management handling operations

void*
scope_malloc(size_t size) {
    void *ptr = malloc(size);
    alloc_size += malloc_usable_size(ptr);
    return ptr;
}

void*
scope_calloc(size_t nmemb, size_t size) {
    void *ptr = calloc(nmemb, size);
    alloc_size += malloc_usable_size(ptr);
    return ptr;
}

void*
scope_realloc(void *ptr, size_t size) {
    alloc_size -= malloc_usable_size(ptr);
    void* new_ptr = realloc(ptr, size);
    alloc_size += malloc_usable_size(new_ptr);
    return new_ptr;
}

void
scope_free(void *ptr) {
    alloc_size -= malloc_usable_size(ptr);
    free(ptr);
}

void*
scope_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
    void* ptr = mmap(addr, length, prot, flags, fd, offset);
    if (ptr != MAP_FAILED) {
        alloc_size += length;
    }
    return ptr;
}

int
scope_munmap(void *addr, size_t length) {
    int res = munmap(addr, length);
    if (!res) {
        alloc_size -= length;
    }
    return res;
}


// String handling operations

char*
scope_realpath(const char *restrict path, char *restrict resolved_path) {
    char* new_path = realpath(path, resolved_path);
    if (!resolved_path) {
        alloc_size += malloc_usable_size(new_path);
    }
    return new_path;
}

ssize_t
scope_readlink(const char *restrict pathname, char *restrict buf, size_t bufsiz) {
    return readlink(pathname, buf, bufsiz);
}

char*
scope_strdup(const char *s) {
    char *ptr = strdup(s);
    alloc_size += malloc_usable_size(ptr);
    return ptr;
}

int
scope_asprintf(char **restrict strp, const char *restrict fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int res = vasprintf(strp, fmt, args);
    va_end(args);
    alloc_size += malloc_usable_size(strp);
    return res;
}


// Other

size_t scope_total_alloc_size(void) {
    return alloc_size;
}
