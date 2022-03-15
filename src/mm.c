#define _GNU_SOURCE

#include "mm.h"
#include <limits.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//TODO make this atomic
static uint64_t alloc_size;

extern void* scopelibc_malloc(size_t size);
extern void* scopelibc_calloc(size_t nmemb, size_t size);
extern void* scopelibc_realloc(void *ptr, size_t size);
extern char* scopelibc_strdup(const char *s);
extern char* scopelibc_strndup(const char *s, size_t n);
extern wchar_t* scopelibc_wcsdup(const wchar_t *s);
extern int scopelibc_posix_memalign(void **memptr, size_t alignment, size_t size);
extern char* scopelibc_realpath(const char *restrict path, char *restrict resolved_path);

extern void  scopelibc_free(void *ptr);
extern size_t scopelibc_malloc_usable_size(void* ptr);

void *mm_malloc(size_t size) {
    void *ptr = scopelibc_malloc(size);
    alloc_size += scopelibc_malloc_usable_size(ptr);
    return ptr;
}

void mm_free(void *ptr) {
    alloc_size -= scopelibc_malloc_usable_size(ptr);
    scopelibc_free(ptr);
}

void *mm_calloc(size_t nmemb, size_t size) {
    void *ptr = scopelibc_calloc(nmemb, size);
    alloc_size += scopelibc_malloc_usable_size(ptr);
    return ptr;
}

void *mm_realloc(void *ptr, size_t size) {
    alloc_size -= scopelibc_malloc_usable_size(ptr);
    void* new_ptr = scopelibc_realloc(ptr, size);
    alloc_size += scopelibc_malloc_usable_size(new_ptr);
    return new_ptr;
}

int mm_posix_memalign(void **memptr, size_t alignment, size_t size) {
    int res = scopelibc_posix_memalign(memptr, alignment, size);
    alloc_size += scopelibc_malloc_usable_size(*memptr);
    return res;
}

char *mm_strdup(const char *s) {
    char *ptr = scopelibc_strdup(s);
    alloc_size += scopelibc_malloc_usable_size(ptr);
    return ptr;
}

char *mm_strndup(const char *s, size_t n) {
    char *ptr = scopelibc_strndup(s, n);
    alloc_size += scopelibc_malloc_usable_size(ptr);
    return ptr;
}

wchar_t *mm_wcsdup(const wchar_t *s) {
    wchar_t *res = scopelibc_wcsdup(s);
    alloc_size += scopelibc_malloc_usable_size(res);
    return res;
}

char *mm_realpath(const char *restrict path) {
    char* new_path = scopelibc_realpath(path, NULL);
    alloc_size += scopelibc_malloc_usable_size(new_path);
    return new_path;
}

int mm_asprintf(char **restrict strp, const char *restrict fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int res = vasprintf(strp, fmt, args);
    va_end(args);
    alloc_size += scopelibc_malloc_usable_size(strp);
    return res;
}

void *mm_mmap(void *addr, size_t length, int prot, int flags,
                  int fd, off_t offset) {
    void* ptr = mmap(addr, length, prot, flags, fd, offset);
    if (ptr != MAP_FAILED) {
        alloc_size += length;
    }
    return ptr;
}

int mm_munmap(void *addr, size_t length) {
    int res = munmap(addr, length);
    alloc_size -= length;
    return res;
}

size_t mm_get_alloc_size(void) {
    return alloc_size;
}
