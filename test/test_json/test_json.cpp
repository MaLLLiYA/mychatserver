#include "json.hpp"
using json = nlohmann::json;
#include <string>
#include <map>
#include <vector>
#include <iostream>
using namespace std;

// json序列化/反序列化
string func1()
{
    json js;
    js["msg_type"] = 2;
    js["from"] = "zhang san";
    js["to"] = "li si";
    js["msg"] = "Hello World";

    cout << js << endl;

    string sendmsg = js.dump(); // json数据对象 序列化为json字符串
    // cout << sendmsg.c_str() << endl;
    return sendmsg;
}

// json序列化容器
string func2()
{
    vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(5);

    map<int, string> m;
    m.insert({7, "AAA"});
    m.insert({8, "BBB"});
    m.insert({9, "CCC"});

    json js;
    js["vector"] = v;
    js["map"] = m;

    // cout << js << endl;
    string js_str = js.dump();
    return js_str;
}

int main()
{
    string rcvmsg = func2();
    json jsbuf = json::parse(rcvmsg); // 反序列化 字符串==>数据对象

    // cout << jsbuf["msg_type"] << endl;
    // cout << jsbuf["from"] << endl;
    // cout << jsbuf["to"] << endl;
    // cout << jsbuf["msg"] << endl;
    vector<int> v = jsbuf["vector"];
    for (int p : v)
    {
        cout << p << endl;
    }

    map<int, string> m = jsbuf["map"];
    for (auto p : m)
    {
        cout << p.first << " " << p.second << endl;
    }
    return 0;
}