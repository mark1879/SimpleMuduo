#include "Buffer.h"

#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>


/**
 * 从 fd 上读取数据，Poller 工作在 LT 模式。
 * Buffer 缓冲区大小是固定的， 但是从 fd 上读数据的时候，
 * 最终的数据大小是未知的，所以要设计 buf。
 */ 
ssize_t Buffer::ReadFd(int fd, int* stored_errno)
{
    char extra_buf[65536] = {0};
    
    struct iovec vec[2];
    
    const size_t writable_bytes = WritableBytes();  // Buffer 底层缓冲区剩余的可写空间大小
    vec[0].iov_base = Begin() + writer_index_;
    vec[0].iov_len = writable_bytes;

    // 如果 vec[0] 空间够用，则不用考虑 vec[1]
    vec[1].iov_base = extra_buf;
    vec[1].iov_len = sizeof(extra_buf);
    
    // 一次最多读 65536 / 1024 = 64k Bytes
    const int iovcnt = (writable_bytes < sizeof(extra_buf)) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt);
    if (n < 0)
    {
        *stored_errno = errno;
    }
    else if (n <= writable_bytes) 
    {
        writer_index_ += n;
    }
    else 
    {
        writer_index_ = buffer_.size();
        Append(extra_buf, n - writable_bytes);
    }

    return n;
}

ssize_t Buffer::WriteFd(int fd, int* stored_errno)
{
    ssize_t n = ::write(fd, Peek(), ReadableBytes());
    if (n < 0)
    {
        *stored_errno = errno;
    }
    return n;
}