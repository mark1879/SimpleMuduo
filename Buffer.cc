#include "Buffer.h"

#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>


ssize_t Buffer::ReadFd(int fd, int* stored_errno)
{
    char extra_buf[65536] = {0};
    
    struct iovec vec[2];
    
    const size_t writable_bytes = WritableBytes();
    vec[0].iov_base = Begin() + writer_index_;
    vec[0].iov_len = writable_bytes;

    vec[1].iov_base = extra_buf;
    vec[1].iov_len = sizeof(extra_buf);
    
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