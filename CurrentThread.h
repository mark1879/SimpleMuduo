#pragma once

#include <unistd.h>
#include <sys/syscall.h>

namespace CurrentThread {
    extern __thread int cached_tid_;

    void CachedTid();

    inline int tid() {
        if (__builtin_expect(cached_tid_ == 0, 0)) {
            CachedTid();
        }

        return cached_tid_;
    }
}