#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"

#include <functional>
#include <errno.h>
#include <sys/types.h>         
#include <sys/socket.h>
#include <strings.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <string>

static EventLoop* CheckLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d TcpConnection Loop is null! \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop, 
                const std::string &nameArg, 
                int sockfd,
                const InetAddress& localAddr,
                const InetAddress& peerAddr)
    : loop_(CheckLoopNotNull(loop))
    , name_(nameArg)
    , state_(kConnecting)
    , reading_(true)
    , socket_(new Socket(sockfd))
    , channel_(new Channel(loop, sockfd))
    , local_addr_(localAddr)
    , peer_addr_(peerAddr)
    , high_watermark_(64*1024*1024) // 64M
{
    // 下面给 channel 设置相应的回调函数，poller会回调相关事件
    channel_->set_read_callback(
        std::bind(&TcpConnection::HandleRead, this, std::placeholders::_1)
    );

    channel_->set_write_callback(
        std::bind(&TcpConnection::HandleWrite, this)
    );

    channel_->set_close_callback(
        std::bind(&TcpConnection::HandleClose, this)
    );

    channel_->set_error_callback(
        std::bind(&TcpConnection::HandleError, this)
    );

    LOG_INFO("TcpConnection::ctor[%s] at fd=%d\n", name_.c_str(), sockfd);

    socket_->SetKeepAlive(true);
}


TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d \n", 
        name_.c_str(), channel_->fd(), (int)state_);
}

void TcpConnection::Send(const std::string &buf)
{
    if (state_ == kConnected)
    {
        if (loop_->IsInLoopThread())
        {
            SendInLoop(buf.c_str(), buf.size());
        }
        else
        {
            loop_->RunInLoop(std::bind(
                &TcpConnection::SendInLoop,
                this,
                buf.c_str(),
                buf.size()
            ));
        }
    }
}

/**
 * 发送数据  应用写的快， 而内核发送数据慢， 需要把待发送数据写入缓冲区， 而且设置了水位回调
 */ 
void TcpConnection::SendInLoop(const void* data, size_t len)
{
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool fault_error = false;

    // 之前调用过该connection的 shutdown，不能再进行发送了
    if (state_ == kDisconnected)
    {
        LOG_ERROR("disconnected, give up writing!");
        return;
    }

    // 表示 channel_ 第一次开始写数据，而且缓冲区没有待发送数据
    if (!channel_->IsWriting() && output_buffer_.readableBytes() == 0)
    {
        nwrote = ::write(channel_->fd(), data, len);
        if (nwrote >= 0)
        {
            remaining = len - nwrote;
            if (remaining == 0 && write_complete_callback_)
            {
                // 数据全部发送完成，就不用再给 channel 设置 epollout 事件了
                loop_->QueueInLoop(
                    std::bind(write_complete_callback_, shared_from_this())
                );
            }
        }
        else    // nwrote < 0
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK)   // Operation would block
            {
                LOG_ERROR("TcpConnection::sendInLoop");
                if (errno == EPIPE || errno == ECONNRESET) // SIGPIPE  RESET
                {
                    fault_error = true;
                }
            }
        }
    }

    // 说明当前这一次write，并没有把数据全部发送出去，剩余的数据需要保存到缓冲区当中，然后给 channel
    // 注册 epollout 事件，当 poller 发现 tcp 的发送缓冲区有空间，会通知相应的sock-channel，调用 write callback 回调方法
    // 也就是调用 TcpConnection::HandleWrite 方法，把发送缓冲区中的数据全部发送完成
    if (!fault_error && remaining > 0) 
    {
        size_t old_len = output_buffer_.readableBytes();

        if (old_len + remaining >= high_watermark_
            && old_len < high_watermark_
            && high_watermark_callback_)
        {
            loop_->QueueInLoop(
                std::bind(high_watermark_callback_, shared_from_this(), old_len + remaining)
            );
        }

        output_buffer_.append((char*)data + nwrote, remaining);
        if (!channel_->IsWriting())
        {
            // 一定要注册 channel 的写事件，否则 poller 不会给 channel 通知 epollout
            channel_->EnableWriting(); 
        }
    }
}


void TcpConnection::Shutdown()
{
    if (state_ == kConnected)
    {
        set_state(kDisconnecting);
        loop_->RunInLoop(
            std::bind(&TcpConnection::ShutdownInLoop, this)
        );
    }
}

void TcpConnection::ShutdownInLoop()
{
    if (!channel_->IsWriting()) // 说明outputBuffer中的数据已经全部发送完成
    {
        socket_->ShutdownWrite();
    }
}

void TcpConnection::ConnectEstablished()
{
    set_state(kConnected);
    channel_->tie(shared_from_this());

    // 向 poller 注册channel的 epollin 事件
    channel_->EnableReading(); 

    connection_callback_(shared_from_this());
}

void TcpConnection::ConnectDestroyed()
{
    if (state_ == kConnected)
    {
        set_state(kDisconnected);

        // 把 channel 的所有感兴趣的事件，从 poller 中 del 掉
        channel_->DisableAll();         

        connection_callback_(shared_from_this());
    }

    // 把 channel 从 poller 中删除掉
    channel_->Remove();                 
}

void TcpConnection::HandleRead(Timestamp receive_time)
{
    int saved_errno = 0;
    ssize_t n = input_buffer_.readFd(channel_->fd(), &saved_errno);
    if (n > 0)
    {
        // 已建立连接的用户，有可读事件发生了，调用用户传入的回调操作 onMessage
        message_callback_(shared_from_this(), &input_buffer_, receive_time);
    }
    else if (n == 0)
    {
        HandleClose();
    }
    else
    {
        errno = saved_errno;
        LOG_ERROR("TcpConnection::handleRead");
        HandleError();
    }
}

void TcpConnection::HandleWrite()
{
    if (channel_->IsWriting())
    {
        int saved_errno = 0;
        ssize_t n = output_buffer_.writeFd(channel_->fd(), &saved_errno);
        if (n > 0)
        {
            output_buffer_.retrieve(n);
            if (output_buffer_.readableBytes() == 0)
            {
                channel_->DisableWriting();
                if (write_complete_callback_)
                {
                    // 唤醒 loop_对应的 thread 线程，执行回调
                    loop_->QueueInLoop(
                        std::bind(write_complete_callback_, shared_from_this())
                    );
                }
                if (state_ == kDisconnecting)
                {
                    ShutdownInLoop();
                }
            }
        }
        else
        {
            LOG_ERROR("TcpConnection::handleWrite");
        }
    }
    else
    {
        LOG_ERROR("TcpConnection fd=%d is down, no more writing \n", channel_->fd());
    }
}


// poller => channel::CloseCallback => TcpConnection::HandleClose
void TcpConnection::HandleClose()
{
    LOG_INFO("TcpConnection::handleClose fd=%d state=%d \n", channel_->fd(), (int)state_);
    set_state(kDisconnected);
    channel_->DisableAll();

    TcpConnectionPtr connPtr(shared_from_this());

    connection_callback_(connPtr);
    close_callback_(connPtr);        // 关闭连接的回调，执行 TcpServer::RemoveConnection 回调方法
}

void TcpConnection::HandleError()
{
    int optval;
    socklen_t optlen = sizeof(optval);
    int err = 0;
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        err = errno;
    }
    else
    {
        err = optval;
    }

    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d \n", name_.c_str(), err);
}