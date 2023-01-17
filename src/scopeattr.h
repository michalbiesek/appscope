#ifndef __SCOPEATTR_H__
#define __SCOPEATTR_H__

#ifndef SCOPE_UNREACHABLE
    #define SCOPE_UNREACHABLE() (__builtin_unreachable())
#endif

#ifndef SCOPE_ATTR_WEAK
    #define SCOPE_ATTR_WEAK     __attribute__((weak))
#endif

#ifndef SCOPE_UNUSED
    #define SCOPE_ATTR_UNUSED   __attribute__((unused))
#endif

#ifndef SCOPE_CONSTRUCTOR
    #define SCOPE_CONSTRUCTOR   __attribute__((constructor))
#endif

#ifndef SCOPE_ATTR_FORMAT
    #define SCOPE_ATTR_FORMAT(fmt_id, arg_id) __attribute__((format(printf, (fmt_id), (arg_id))))
#endif

#endif // __SCOPEATTR_H__

