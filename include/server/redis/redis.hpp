#ifndef REDIS_H
#define REDIS_H

#include <hiredis/hiredis.h>
#include <thread>
#include <functional>

using namespace std;

class Redis
{
public:
    Redis();
    ~Redis();

    // 连接
    bool connect();

    // 向通道发布消息
    bool publish(int channel, string message);

    // 向通道订阅消息
    bool subscribe(int channel);

    // 取消订阅消息
    bool unsbsubscribe(int channel);

    // 在独立线程接收订阅通道中的消息
    void observer_channel_message();

    // 初始化向业务层上报消息的回调
    void init_notify_handler(function<void(int, string)> fn);

private:
    // 同步上下文对象 发布
    redisContext *_publish_context;

    // 同步上下文对象 订阅（阻塞）
    redisContext *_subscribe_context;

    // 回调
    function<void(int, string)> _notify_message_handler;
};

#endif