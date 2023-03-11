#pragma once

#include "noncopyable.h"
#include "Timestamp.h"
#include <functional>
#include <memory>

class EventLoop;

enum class ChannelIndex
{
    kNew = -1,
    kAdded = 1,
    kDeleted = 2
};

class Channel : noncopyable
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(TimeStamp)>;

    Channel(EventLoop *loop, int fd)
        : loop_(loop), fd_(fd)
        , events_(0), revents_(0)
        , index_(-1), tied_(false)
    {}

    ~Channel()
    {}

    void HandleEvent(TimeStamp received_time);

    void set_read_callback_(ReadEventCallback cb)
    { read_callback_ = std::move(cb); }

    void set_write_callback(EventCallback cb)
    { write_callback_ = std::move(cb); }

    void set_close_callback(EventCallback cb)
    { close_callback_ = std::move(cb); }

    void set_error_allback_(EventCallback cb)
    { error_callback_ = std::move(cb); }
    
    // 防止当 channel 还在执行回调操作时，被手动remove掉
    void set_tie(const std::shared_ptr<void>&);

    int fd() const { return fd_; }
    int events() const { return events_; }
    void set_revents(int revents) { revents = revents; }

    void EnableReading() { events_ |= kReadEvent; Update(); }
    void DisableReading() { events_ &= ~kReadEvent; Update(); }
    void EnableWriting() { events_ |= kWriteEvent; Update(); }
    void DisableWriting() { events_ &= ~kWriteEvent; Update(); }
    void DisableAll() { events_ = kNoneEvent; Update(); }

    bool IsNoneEvent() const { return events_ == kNoneEvent; }
    bool IsWriting() const { return events_ & kWriteEvent; }
    bool IsReading() const { return events_ & kReadEvent; }

    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    EventLoop* OwnerLoop() { return loop_; }
    void Remove();

private:
    void Update();
    void HandleEventWithGuard(TimeStamp received_time);

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_;
    const int fd_;
    int events_;
    int revents_;
    int index_;

    std::weak_ptr<void> tie_;
    bool tied_;

    ReadEventCallback read_callback_;
    EventCallback write_callback_;
    EventCallback close_callback_;
    EventCallback error_callback_;
};