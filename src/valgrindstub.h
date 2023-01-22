#ifndef __VALGRINDSTUB_H__
#define __VALGRINDSTUB_H__

#if defined __has_include
    #if __has_include (<valgrind/valgrind.h>)
        #include <valgrind/valgrind.h>
        #define SCOPE_VALGRIND_MALLOCLIKE_BLOCK(addr, sizeB, rzB, is_zeroed)      VALGRIND_MALLOCLIKE_BLOCK(addr,sizeB, rzB, is_zeroed)
        #define SCOPE_VALGRIND_FREELIKE_BLOCK(addr, rzB)                          VALGRIND_FREELIKE_BLOCK (addr, rzB)
    #else
        #error "Missing valgrind on system"
    #endif
#else
    #error "__has_include is not supported on system"
#endif

#endif // __VALGRINDSTUB_H__
