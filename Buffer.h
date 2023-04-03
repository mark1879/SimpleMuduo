#pragma once

#include <vector>
#include <string>
#include <algorithm>

class Buffer
{
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize)
        , reader_index_(kCheapPrepend)
        , writer_index_(kCheapPrepend)
    {}

    size_t ReadableBytes() const 
    {
        return writer_index_ - reader_index_;
    }

    size_t WritableBytes() const
    {
        return buffer_.size() - writer_index_;
    }

    size_t PrependableBytes() const
    {
        return reader_index_;
    }

    const char* Peek() const
    {
        return Begin() + reader_index_;
    }

    void Retrieve(size_t len)
    {
        if (len < ReadableBytes())
        {
            reader_index_ += len;
        }
        else
        {
            RetrieveAll();
        }
    }

    void RetrieveAll()
    {
        reader_index_ = writer_index_ = kCheapPrepend;
    }

    std::string RetrieveAllAsString()
    {
        return RetrieveAsString(ReadableBytes());
    }

    std::string RetrieveAsString(size_t len)
    {
        std::string result(Peek(), len);
        Retrieve(len); 

        return result;
    }

    void EnsureWriteableBytes(size_t len)
    {
        if (WritableBytes() < len)
        {
            MakeSpace(len);
        }
    }

    void Append(const char *data, size_t len)
    {
        EnsureWriteableBytes(len);
        std::copy(data, data+len, BeginWrite());
        writer_index_ += len;
    }

    char* BeginWrite()
    {
        return Begin() + writer_index_;
    }

    const char* BeginWrite() const
    {
        return Begin() + writer_index_;
    }

    ssize_t ReadFd(int fd, int* stored_errno);
    ssize_t WriteFd(int fd, int* stored_errno);
private:
    char* Begin()
    {
        return &*buffer_.begin();
    }

    const char* Begin() const
    {
        return &*buffer_.begin();
    }

    void MakeSpace(size_t len)
    {
        if (WritableBytes() + PrependableBytes() < len + kCheapPrepend)
        {
            buffer_.resize(writer_index_ + len);
        }
        else
        {
            size_t readalbe_bytes = ReadableBytes();
            std::copy(Begin() + reader_index_, Begin() + writer_index_, Begin() + kCheapPrepend);
            reader_index_ = kCheapPrepend;
            writer_index_ = reader_index_ + readalbe_bytes;
        }
    }

    std::vector<char> buffer_;
    size_t reader_index_;
    size_t writer_index_;
};