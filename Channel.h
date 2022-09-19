#pragma once

#include <functional>
#include <memory>
#include "noncopyable.h"
#include "Timestamp.h"
#include "EventLoop.h"

class Channel : noncopyable {
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(TimeStamp)>;

    Channel(EventLoop *loop, int fd);
    ~Channel();

    void HandleEvent(TimeStamp recv_time);

    void set_read_callback(ReadEventCallback cb) { read_callback_ = std::move(cb); }
    void set_write_callback(EventCallback cb) { write_callback_ = std::move(cb); }
    void set_close_callback(EventCallback cb) { close_callback_ = std::move(cb); }
    void set_error_callback(EventCallback cb) { error_callback_ = std::move(cb); }

    void set_tie(const std::shared_ptr<void>&);

    int fd() const { return fd_; }
    int events() const { return events_; }
    int set_revents(int revts) { revents_ = revts; }

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

    EventLoop* OwerLoop() { return loop_; }
    void Remove();
private:

    void Update();
    void HandleEventWithGuard(TimeStamp recv_time);

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
