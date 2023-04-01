#pragma once

#include "Timestamp.h"
#include "noncopyable.h"
#include <functional>
#include <memory>

class EventLoop;

enum class ChannelState
{
    kNew = -1,
    kAdded = 1,
    kDeleted = 2
};

class Channel : noncopyable
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop *loop, int fd);
    ~Channel();

    void HandleEvent(Timestamp received_time);

    void set_read_callback(ReadEventCallback cb)
    { read_callback_ = std::move(cb); }

    void set_write_callback(EventCallback cb) 
    { write_callback_ = std::move(cb); }

    void set_close_callback(EventCallback cb)
    { close_callback_ = std::move(cb); } 

    void set_error_allback(EventCallback cb)
    { error_callback_ = std::move(cb); }
    
    // Tie this channel to the owner object managed by shared_ptr
    // prevent the ower object being destroyed in HandleEvent
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

    ChannelState state() { return state_; }
    void set_state(ChannelState state) { state_ = state; }

    EventLoop* OwnerLoop() { return loop_; }
    void Remove();

private:
    void Update();
    void HandleEventWithGuard(Timestamp received_time);

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_;
    const int fd_;
    int events_;    // The events interested by fd_
    int revents_;   // it's the received event types of epoll or poll
    ChannelState state_;     // used by Poller

    std::weak_ptr<void> tie_;
    bool tied_;

    ReadEventCallback read_callback_;
    EventCallback write_callback_;
    EventCallback close_callback_;
    EventCallback error_callback_;
};