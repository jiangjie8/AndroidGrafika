#ifndef CORE_UTILS_UTILS_H
#define CORE_UTILS_UTILS_H
#include "core/common.h"
#include <assert.h>
#include <inttypes.h>

static inline size_t alignSize(size_t sz, int n)
{
    assert((n & (n - 1)) == 0); // n is a power of 2
    return (sz + n - 1) & -n;
}


#endif
