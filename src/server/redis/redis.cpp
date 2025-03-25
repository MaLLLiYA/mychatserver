#include "redis.hpp"
#include <iostream>

using namespace std;

Redis::Redis() : _publish_context(nullptr), _subscribe_context(nullptr) {}
Redis::~Redis()
{
    if (_publish_context != nullptr)
    {
        redisFree(_publish_context);
    }
    if (_subscribe_context != nullptr)
    {
        redisFree(_subscribe_context);
    }
}

// 连接
bool Redis::connect()
{

    // 发布上下文
    _publish_context = redisConnect("127.0.0.1", 6379);
    if (_publish_context == nullptr)
    {
        cerr << "connect redis failed" << endl;
        return false;
    }

    // 订阅上下文
    _subscribe_context = redisConnect("127.0.0.1", 6379);
    if (_subscribe_context == nullptr)
    {
        cerr << "connect redis failed" << endl;
        return false;
    }

    // 在单独线程中监听
    thread t([&]()
             { observer_channel_message(); });

    t.detach();
    cout << "connect redis success" << endl;

    return true;
}

// 向通道发布消息
bool Redis::publish(int channel, string message)
{
    // publish不会阻塞
    redisReply *reply = (redisReply *)redisCommand(_publish_context, "PUBLISH %d %s", channel, message.c_str());

    if (reply == nullptr)
    {
        cerr << "publish command failed" << endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

// 向通道订阅消息
bool Redis::subscribe(int channel)
{
    if (redisAppendCommand(this->_subscribe_context, "SUBSCRIBE %d", channel) == REDIS_ERR)
    {
        cerr << "subscribe command failed" << endl;
        return false;
    }

    int done = 0;
    while (!done)
    {
        if (redisBufferWrite(this->_subscribe_context, &done) == REDIS_ERR)
        {
            cerr << "subscribe command failed" << endl;
            return false;
        }
    }
    return true;
}

// 取消订阅消息
bool Redis::unsbsubscribe(int channel)
{
    if (redisAppendCommand(this->_subscribe_context, "UNSUBSCRIBE %d", channel) == REDIS_ERR)
    {
        cerr << "unsubscribe command failed" << endl;
        return false;
    }

    int done = 0;
    while (!done)
    {
        if (redisBufferWrite(this->_subscribe_context, &done) == REDIS_ERR)
        {
            cerr << "unsubscribe command failed" << endl;
            return false;
        }
    }
    return true;
}

// 在独立线程接收订阅通道中的消息
void Redis::observer_channel_message()
{
    redisReply *reply = nullptr;
    while (redisGetReply(this->_subscribe_context, (void **)&reply) == REDIS_OK)
    {
        // 消息为3个元素的数组
        if (reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
        {
            // 消息正确
            _notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
        }
        freeReplyObject(reply);
    }
    cerr << ">>>>>>>>>>>>> observer_channel_message quit <<<<<<<<<<<<<" << endl;
}

// 初始化向业务层上报消息的回调
void Redis::init_notify_handler(function<void(int, string)> fn)
{
    this->_notify_message_handler = fn;
}