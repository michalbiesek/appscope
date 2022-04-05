#include "scopestdlib.h"
#include "scopetypes.h"
#include "strset.h"
#include <stdlib.h>
#include <string.h>

typedef struct _strset_t {
    const char **str;
    unsigned int count;
    unsigned int capacity;
} strset_t;

strset_t *
strSetCreate(unsigned int initialCapacity)
{
    strset_t *set = scope_malloc(sizeof(*set));
    const char **str = scope_malloc(initialCapacity * sizeof(char *));
    if (!set || !str) goto err;

    set->str = str;
    set->count = 0;
    set->capacity = initialCapacity;

    return set;

err:
    if (set) scope_free(set);
    if (str) scope_free(str);
    return NULL;
}

void
strSetDestroy(strset_t **set_ptr)
{
    if (!set_ptr || !*set_ptr) return;

    strset_t *set = *set_ptr;
    scope_free(set->str);
    scope_free(set);
    *set_ptr = NULL;
}

bool
strSetAdd(strset_t *set, const char *str)
{
    if (!set || !str) return FALSE;

    // enforce that no dup values are allowed
    if (strSetContains(set, str)) return FALSE;

    // grow if needed
    if (set->count >= set->capacity) {
        unsigned int new_capacity = (set->capacity) ? set->capacity * 4 : 2;
        const char **new_str = scope_realloc(set->str, new_capacity * sizeof(char *));
        if (!new_str) {
            return FALSE;
        }
        set->str = new_str;
        set->capacity = new_capacity;
    }

    // Add str to the set
    set->str[set->count++] = str;
    return TRUE;
}

bool
strSetContains(strset_t *set, const char *str)
{
    if (!set || !str) return FALSE;

    unsigned int i;
    for (i=0; i < set->count; i++) {
        if (scope_strcmp(set->str[i], str) == 0) {
            return TRUE;
        }
    }
    return FALSE;
}

unsigned int
strSetEntryCount(strset_t *set)
{
    return (set) ? set->count : 0;
}
