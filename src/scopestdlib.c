#define _GNU_SOURCE

#include "scopestdlib.h"

#include <limits.h>
#include <malloc.h>
#include <stdarg.h>
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

// File handling operations
extern FILE*   scopelibc_fopen(const char *, const char *);
extern int     scopelibc_fclose(FILE *);
extern FILE*   scopelibc_fdopen(int, const char *);
extern int     scopelibc_open(const char *, int, mode_t);
extern int     scopelibc_close(int);
extern size_t  scopelibc_fread(void *, size_t, size_t, FILE *);
extern size_t  scopelibc_fwrite(const void *, size_t, size_t, FILE *);
extern ssize_t scopelibc_getline(char **, size_t *, FILE *);
extern int     scopelibc_setvbuf(FILE *, char *, int, size_t);
extern int     scopelibc_fflush(FILE *);

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


// File handling operations

FILE*
scope_fopen( const char * filename, const char * mode) {
    return scopelibc_fopen(filename, mode);
}

int
scope_fclose(FILE * stream ) {
    return scopelibc_fclose(stream);
}

FILE*
scope_fdopen(int fd, const char *mode) {
    return scopelibc_fdopen(fd, mode);
}

int
scope_open(const char *pathname, int flags, mode_t mode) {
    return scopelibc_open(pathname, flags, mode);
}

int
scope_close(int fd) {
    return scopelibc_close(fd);
}

size_t
scope_fread(void *restrict ptr, size_t size, size_t nmemb, FILE *restrict stream) {
    return scopelibc_fread(ptr, size, nmemb, stream);
}

size_t
scope_fwrite(const void *restrict ptr, size_t size, size_t nmemb, FILE *restrict stream) {
    return scopelibc_fwrite(ptr, size, nmemb, stream);
}
ssize_t
scope_getline(char **restrict lineptr, size_t *restrict n, FILE *restrict stream) {
    return scopelibc_getline(lineptr, n, stream);
}

int
scope_setvbuf(FILE *restrict stream, char *restrict buf, int type, size_t size) {
    return scopelibc_setvbuf(stream, buf, type, size);
}

int
scope_fflush(FILE *stream) {
    return scopelibc_fflush(stream);
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

int
scope_vasprintf(char **strp, const char *fmt, va_list ap){
    return scopelibc_vasprintf(strp, fmt, ap);
}


// Other

size_t scope_total_alloc_size(void) {
    return alloc_size;
}
