#pragma once

#include <unistd.h>
#include <sys/syscall.h>

namespace CurrentThread
{
    // thread local
    extern __thread int cached_tid_;

    void CacheTid();

    inline int tid()
    {
        // cached_tid_ == 0 is more likely to be false
        if (__builtin_expect(cached_tid_ == 0, 0))
        {
            CacheTid();
        }
        return cached_tid_;
    }
}