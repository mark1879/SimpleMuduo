#include <TcpServer.h>
#include <Logger.h>

#include <string>
#include <functional>

class EchoServer
{
public:
    EchoServer(EventLoop *loop,
            const InetAddress &addr, 
            const std::string &name)
        : server_(loop, addr, name)
        , loop_(loop)
    {
        server_.set_connection_callback(
            std::bind(&EchoServer::OnConnection, this, std::placeholders::_1)
        );

        server_.set_message_callback(
            std::bind(&EchoServer::OnMessage, this,
                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
        );

        server_.SetThreadNum(3);
    }
    void Start()
    {
        server_.Start();
    }
private:
    void OnConnection(const TcpConnectionPtr &conn)
    {
        if (conn->connected())
        {
            LOG_INFO("Connection UP : %s", conn->peer_addr().ToIpPort().c_str());
        }
        else
        {
            LOG_INFO("Connection DOWN : %s", conn->peer_addr().ToIpPort().c_str());
        }
    }

    // 可读写事件回调
    void OnMessage(const TcpConnectionPtr &conn,
                Buffer *buf,
                Timestamp time)
    {
        std::string msg = buf->retrieveAllAsString();
        conn->Send(msg);
        // conn->Shutdown();               // 写端   EPOLLHUP =》 closeCallback_
    }

    EventLoop *loop_;
    TcpServer server_;
};

int main()
{
    EventLoop loop;
    InetAddress addr(8000);
    EchoServer server(&loop, addr, "EchoServer-01");    // Acceptor non-blocking listenfd  create bind 
    server.Start();                                     // listen  loopthread  listenfd => accept channel => main loop
    loop.Loop();                                        // 启动 main loop的底层 Poller

    return 0;
}