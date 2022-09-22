#include "CurrentThread.h"

namespace CurrentThread {

    __thread int cached_tid_ = 0;

    void CachedTid() {
        if (cached_tid_ == 0) {
            cached_tid_ = static_cast<pid_t>(syscall(SYS_gettid));
        }
    }
}