/**
 * Cribl AppScope - Library Directory Interface
 *
 * See docs/STARTUP.md
 */

#ifndef _SCOPE_LIBDIR_H
#define _SCOPE_LIBDIR_H 1

// File types
#define LIBRARY_FILE 0 // libscope.so
#define LOADER_FILE 1  // ldscopedyn

int         libdirSetBase(int, const char *);    // Override default base dir i.e. /tmp
int         libdirExtract(int);                  // Extracts file to default path
const char* libdirGetPath(int);                  // Get full path to existing file 

#endif // _SCOPE_LIBDIR_H
