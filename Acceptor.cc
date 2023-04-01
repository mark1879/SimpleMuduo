#include "Acceptor.h"
#include "Logger.h"
#include "InetAddress.h"
#include <sys/types.h>    
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>


static int CreateSocketWithNonBlocking()
{
    int sock_fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (sock_fd < 0) 
    {
        LOG_FATAL("%s:%s:%d listen socket create errono:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
    }

    return sock_fd;
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listen_addr, bool reuse_port)
    : loop_(loop)
    , accept_socket_(CreateSocketWithNonBlocking())
    , accept_channel_(loop, accept_socket_.fd())
    , listenning_(false)
{
    accept_socket_.SetReuseAddr(true);
    accept_socket_.SetReusePort(reuse_port);
    accept_socket_.BindAddress(listen_addr); 
    accept_channel_.set_read_callback(std::bind(&Acceptor::HandleRead, this));
}

Acceptor::~Acceptor()
{
    accept_channel_.DisableAll();
    accept_channel_.Remove();
}

void Acceptor::Listen()
{
    listenning_ = true;
    accept_socket_.Listen();
    accept_channel_.EnableReading();
}

void Acceptor::HandleRead()
{
    InetAddress peer_addr;
    int conn_fd = accept_socket_.Accept(&peer_addr);
    if (conn_fd >= 0)
    {
        if (new_connection_callback_)
        {
            new_connection_callback_(conn_fd, peer_addr);
        }
        else
        {
            ::close(conn_fd);
            LOG_WARNING("%s:%s:%d close fd:%d \n", __FILE__, __FUNCTION__, __LINE__, conn_fd);
        }
    }
    else
    {
        LOG_ERROR("%s:%s:%d accept error:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
        if (errno == EMFILE)
        {
            ::close(conn_fd);
            LOG_ERROR("%s:%s:%d socket fd reached limit! \n", __FILE__, __FUNCTION__, __LINE__);

            // may need to adjust the upper limit of the process descriptor or expand the capacity
        }
    }
}