#ifndef __MM_H__
#define __MM_H__
#include <sys/mman.h>
#include <wchar.h>
#include <stdio.h>

FILE *mm_fopen(const char *restrict pathname, const char *restrict mode);
int mm_fclose(FILE *stream);
ssize_t mm_getline(char **restrict lineptr, size_t *restrict n, FILE *restrict stream);
void *mm_malloc(size_t );
void mm_free(void *);
void *mm_calloc(size_t, size_t);
void *mm_realloc(void *, size_t);
int mm_posix_memalign(void **, size_t, size_t);
char *mm_strdup(const char *);
char *mm_strndup(const char *, size_t);
wchar_t *mm_wcsdup(const wchar_t *);
void *mm_mmap(void *, size_t, int, int, int, off_t);
int mm_munmap(void *, size_t);
size_t mm_get_alloc_size(void);
char *mm_realpath(const char *);
int mm_asprintf(char **, const char *, ...);

#endif // __MM_H__
