#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <functional>
#include <string>
using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders;

// 基于muduo开发服务器程序
/*
1. 组合TcpServer对象
2. 创建EventLoop事件循环对象的指针
3. 明确TcpServer的构造函数需要哪些参数，输出ChatServer的构造函数
4. 在当前服务器进程类的构造函数中注册用户连接的创建与断开回调和用户读写事件回调
5. 设置服务端线程数量
*/
class ChatServer
{
public:
    ChatServer(EventLoop *loop,               // 事件循环
               const InetAddress &listenAddr, // IP + PORT
               const string &nameArg)         // 服务器名
        : _server(loop, listenAddr, nameArg), _loop(loop)
    {
        // 给服务器注册用户连接的创建与断开回调
        // 使用绑定器使参数的个数匹配
        _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));
        // 给服务器注册用户读写事件回调
        _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

        // 1个I/O线程和3个worker线程
        _server.setThreadNum(4);
    }

    // 开启事件循环
    void start()
    {
        _server.start();
    }

private:
    // 处理用户连接的创建与断开
    // 成员方法，包含this指针
    // void setConnectionCallback(const ConnectionCallback& cb)
    // typedef std::function<void (const TcpConnectionPtr&)> ConnectionCallback;
    void onConnection(const TcpConnectionPtr &conn)
    {
        // 连接成功
        if (conn->connected())
        {
            cout << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort()
                 << " state:online " << endl;
        }
        else
        {
            cout << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort()
                 << " state:offline " << endl;
            conn->shutdown();
            // _loop->quit();
        }
    }

    // 处理用户读写事件
    /*
    void setMessageCallback(const MessageCallback& cb)
    */
    void onMessage(const TcpConnectionPtr &conn, // 连接
                   Buffer *buffer,               // 缓冲区
                   Timestamp time)               // 收到数据的时间
    {
        string buf = buffer->retrieveAllAsString();
        cout << "recv: " << buf << " time: " << time.toString() << endl;
        conn->send(buf);
    }
    TcpServer _server; // #1
    EventLoop *_loop;  // #2 epoll
};

int main(){
    EventLoop loop; // epoll
    InetAddress addr("127.0.0.1",6000);
    ChatServer server(&loop,addr,"ChatServer");

    server.start();
    loop.loop();    // epoll_wait以阻塞方式等待用户链接或已连接用户的读写事件
    return 0;
}