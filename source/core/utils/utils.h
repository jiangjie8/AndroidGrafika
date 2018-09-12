#ifndef UTILS_H_
#define UTILS_H_
#include <assert.h>
static inline size_t alignSize(size_t sz, int n)
{
    assert((n & (n - 1)) == 0); // n is a power of 2
    return (sz + n - 1) & -n;
}


#endif
