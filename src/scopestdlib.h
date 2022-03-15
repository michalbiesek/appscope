#ifndef __SCOPE_STDLIB_H__
#define __SCOPE_STDLIB_H__

#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>

// Memory management handling operations
void* scope_malloc(size_t );
void* scope_calloc(size_t, size_t);
void* scope_realloc(void *, size_t);
void  scope_free(void *);
void* scope_mmap(void *, size_t, int, int, int, off_t);
int   scope_munmap(void *, size_t);

// File handling operations
FILE*   scope_fopen(const char *, const char *);
int     scope_fclose(FILE *);
FILE*   scope_fdopen(int, const char *);
int     scope_open(const char *, int, mode_t);
int     scope_close(int);
size_t  scope_fread(void *, size_t, size_t, FILE *);
size_t  scope_fwrite(const void *, size_t, size_t, FILE *);
ssize_t scope_getline(char **, size_t *, FILE *);
int     scope_setvbuf(FILE *, char *, int, size_t);
int     scope_fflush(FILE *);

// String handling operations
char*   scope_realpath(const char *, char *);
ssize_t scope_readlink(const char *, char *, size_t);
char*   scope_strdup(const char *);
int     scope_asprintf(char **, const char *, ...);

// Other
size_t scope_total_alloc_size(void);

#endif // __SCOPE_STDLIB_H__
