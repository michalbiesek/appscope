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

// Internal standard library references

// Memory management handling operations
extern void*  scopelibc_malloc(size_t);
extern void*  scopelibc_calloc(size_t, size_t);
extern void*  scopelibc_realloc(void *, size_t);
extern void   scopelibc_free(void *);
extern void*  scopelibc_mmap(void *, size_t, int, int, int, off_t);
extern int    scopelibc_munmap(void *, size_t);
extern size_t scopelibc_malloc_usable_size(void *);

// String handling operations
extern char*   scopelibc_realpath(const char *, char *);
extern ssize_t scopelibc_readlink(const char *, char *, size_t);
extern char*   scopelibc_strdup(const char *);
extern int     scopelibc_vasprintf(char **, const char *, va_list);

// Memory management handling operations

void*
scope_malloc(size_t size) {
    void *ptr = scopelibc_malloc(size);
    alloc_size += scopelibc_malloc_usable_size(ptr);
    return ptr;
}

void*
scope_calloc(size_t nmemb, size_t size) {
    void *ptr = scopelibc_calloc(nmemb, size);
    alloc_size += scopelibc_malloc_usable_size(ptr);
    return ptr;
}

void*
scope_realloc(void *ptr, size_t size) {
    alloc_size -= scopelibc_malloc_usable_size(ptr);
    void* new_ptr = scopelibc_realloc(ptr, size);
    alloc_size += scopelibc_malloc_usable_size(new_ptr);
    return new_ptr;
}

void
scope_free(void *ptr) {
    alloc_size -= scopelibc_malloc_usable_size(ptr);
    scopelibc_free(ptr);
}

void*
scope_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
    void* ptr = scopelibc_mmap(addr, length, prot, flags, fd, offset);
    if (ptr != MAP_FAILED) {
        alloc_size += length;
    }
    return ptr;
}

int
scope_munmap(void *addr, size_t length) {
    int res = scopelibc_munmap(addr, length);
    if (!res) {
        alloc_size -= length;
    }
    return res;
}


// String handling operations

char*
scope_realpath(const char *restrict path, char *restrict resolved_path) {
    char* new_path = scopelibc_realpath(path, resolved_path);
    if (!resolved_path) {
        alloc_size += scopelibc_malloc_usable_size(new_path);
    }
    return new_path;
}

ssize_t
scope_readlink(const char *restrict pathname, char *restrict buf, size_t bufsiz) {
    return scopelibc_readlink(pathname, buf, bufsiz);
}

char*
scope_strdup(const char *s) {
    char *ptr = scopelibc_strdup(s);
    alloc_size += scopelibc_malloc_usable_size(ptr);
    return ptr;
}

int
scope_asprintf(char **restrict strp, const char *restrict fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int res = scopelibc_vasprintf(strp, fmt, args);
    va_end(args);
    alloc_size += scopelibc_malloc_usable_size(strp);
    return res;
}


// Other

size_t scope_total_alloc_size(void) {
    return alloc_size;
}
