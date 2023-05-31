#include "Acceptor.h"
#include "Logger.h"
#include "InetAddress.h"

#include <sys/types.h>    
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>


static int CreateNonblocking()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (sockfd < 0) 
    {
        LOG_FATAL("%s:%s:%d listen socket create err:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
    }

    return sockfd;
}


Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport)
    : loop_(loop)
    , accept_socket_(CreateNonblocking())
    , accept_channel_(loop, accept_socket_.fd())
    , listenning_(false)
{
    accept_socket_.SetReuseAddr(true);
    accept_socket_.SetReusePort(true);

    // 绑定服务器端 ip:port
    accept_socket_.BindAddress(listenAddr);     

    // TcpServer::Start() Acceptor.Listen  
    // 有新用户的连接
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


// 有新用户连接
void Acceptor::HandleRead()
{
    InetAddress peer_addr;
    int connfd = accept_socket_.Accept(&peer_addr);
    if (connfd >= 0)
    {
        if (new_connection_callback_)
        {
            new_connection_callback_(connfd, peer_addr); // 轮询找到subLoop，唤醒，分发当前的新客户端的Channel
        }
        else
        {
            ::close(connfd);
        }
    }
    else
    {
        LOG_ERROR("%s:%s:%d accept err:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
        if (errno == EMFILE)
        {
            LOG_ERROR("%s:%s:%d sockfd reached limit! \n", __FILE__, __FUNCTION__, __LINE__);
        }
    }
}