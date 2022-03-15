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
//TODO handle different category split - memory, file, time 
static uint64_t alloc_size;

extern void* scopelibc_malloc(size_t size);
extern void* scopelibc_calloc(size_t nmemb, size_t size);
extern void* scopelibc_realloc(void *ptr, size_t size);
extern char* scopelibc_strdup(const char *s);
extern char* scopelibc_strndup(const char *s, size_t n);
extern wchar_t* scopelibc_wcsdup(const wchar_t *s);
extern int scopelibc_posix_memalign(void **memptr, size_t alignment, size_t size);
extern char* scopelibc_realpath(const char *restrict path, char *restrict resolved_path);
extern void* scopelibc_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
extern int scopelibc_munmap(void *addr, size_t length);
extern void  scopelibc_free(void *ptr);
extern size_t scopelibc_malloc_usable_size(void* ptr);
extern ssize_t scopelibc_getline(char **restrict lineptr, size_t *restrict n, FILE *restrict stream);
extern FILE* scopelibc_fopen(const char *restrict pathname, const char *restrict mode);
extern int scopelibc_fclose(FILE *stream);
extern int scopelibc_open(const char *pathname, int flags, mode_t mode);
extern int scopelibc_close(int fd);
extern size_t scopelibc_fread(void *restrict ptr, size_t size, size_t nmemb, FILE *restrict stream);
extern size_t scopelibc_fwrite(const void *restrict ptr, size_t size, size_t nmemb, FILE *restrict stream);
extern FILE* scopelibc_fdopen(int fd, const char *mode);
extern int scopelibc_setvbuf(FILE *restrict f, char *restrict buf, int type, size_t size);
extern struct tm* scopelibc_localtime_r(const time_t *restrict timer, struct tm *restrict result);

FILE *mm_fopen(const char *restrict pathname, const char *restrict mode) {
    return scopelibc_fopen(pathname, mode);
}

int mm_fclose(FILE *stream) {
    return scopelibc_fclose(stream);
}

FILE *mm_fdopen(int fd, const char *mode) {
    return scopelibc_fdopen(fd, mode);
}

struct tm *mm_localtime_r(const time_t *restrict timer, struct tm *restrict result) {
    return scopelibc_localtime_r(timer, result);
}

int mm_setvbuf(FILE *restrict f, char *restrict buf, int type, size_t size) {
    return scopelibc_setvbuf(f, buf, type, size);
}

int mm_open(const char *pathname, int flags, mode_t mode) {
    return scopelibc_open(pathname, flags, mode);
}

int mm_close(int fd) {
    return scopelibc_close(fd);
}

size_t mm_fread(void *restrict ptr, size_t size, size_t nmemb, FILE *restrict stream) {
    return scopelibc_fread(ptr, size, nmemb, stream);
}

size_t mm_fwrite(const void *restrict ptr, size_t size, size_t nmemb, FILE *restrict stream) {
    return scopelibc_fwrite(ptr, size, nmemb, stream);
}

ssize_t mm_getline(char **restrict lineptr, size_t *restrict n,
                       FILE *restrict stream){
    return scopelibc_getline(lineptr, n, stream);
}

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
    void* ptr = scopelibc_mmap(addr, length, prot, flags, fd, offset);
    if (ptr != MAP_FAILED) {
        alloc_size += length;
    }
    return ptr;
}

int mm_munmap(void *addr, size_t length) {
    int res = scopelibc_munmap(addr, length);
    alloc_size -= length;
    return res;
}

size_t mm_get_alloc_size(void) {
    return alloc_size;
}
