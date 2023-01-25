#ifndef __SCOPEATTR_H__
#define __SCOPEATTR_H__

#define UNREACHABLE() (__builtin_unreachable())

#ifdef static_assert
#define SCOPE_BUILD_ASSERT(cond, msg) ({static_assert(cond, msg);})
#else
#define SCOPE_BUILD_ASSERT(cond, msg)
#endif

/*
* Size of array
*/
#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))

/*
* Constant String length
*/
#define C_STRLEN(a)  (sizeof(a) - 1)


#ifdef __GNUC__

/*
* Copy paste from typeclass.h
*/
#define POINTER_TYPE_CLASS 5

/*
* Determine whether two types are the same 
* returns 1 if types are compatible, 0 otherwise
*/
#define IS_SAME_TYPE(a, b)  __builtin_types_compatible_p(typeof(a), typeof(b))

/*
* Determine if data types is poitner or array (pointer class)
* returns 1 if p is pointer class, 0 otherwise
*/
#define IS_PTR_OR_ARRAY(p)  (__builtin_classify_type(p) == POINTER_TYPE_CLASS)

/*
* Ternary expression
* returns p if p belongs to pointer class, NULL otherwise
*/
#define CONDITIONAL(p)  (&*__builtin_choose_expr(IS_PTR_OR_ARRAY(p), p, NULL))

#define IS_PTR(p)  IS_SAME_TYPE(p, CONDITIONAL(p))
#else


#define IS_PTR(p)

#endif

#endif // __SCOPEATTR_H__
