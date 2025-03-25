#include <iostream>
#include <vector>
#include <unordered_map>
#include "json.hpp"
#include <thread>
#include <chrono>
using namespace std;
using json = nlohmann::json;

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include "group.hpp"
#include "user.hpp"
#include "public.hpp"

bool isMainMenuRunning = false;

// 当前用户
User g_currentUser;

// 当前用户好友列表
vector<User> g_currentUserFriendList;

// 当前用户群组列表
vector<Group> g_currentUserGroupList;

void showCurrentUserData();

// 接收线程
void readTaskHandler(int clientfd);

// 获取当前时间
string getCurrentTime();

// 主聊天页面
void mainMenu(int clientfd);

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        cerr << "command invalid! example: ./ChatClient 127.0.0.1 6000" << endl;
        exit(-1);
    }

    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    int clientfd = socket(PF_INET, SOCK_STREAM, 0);
    if (clientfd == -1)
    {
        cerr << "socket create error" << endl;
        exit(-1);
    }

    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    // 连接
    if (connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)) == -1)
    {
        cerr << "connect error" << endl;
        close(clientfd);
        exit(-1);
    }

    for (;;)
    {
        // 显示首页
        cout << "====================" << endl;
        cout << "1. login" << endl;
        cout << "2. register" << endl;
        cout << "3. quit" << endl;
        cout << "====================" << endl;
        cout << "choice: ";

        int choice = 0;
        cin >> choice;
        cin.get(); // 读取缓冲区的回车

        switch (choice)
        {
        case 1:
        {
            cout << "LOGIN" << endl;
            int id = 0;
            char pwd[50] = {0};
            cout << "userid: ";
            cin >> id;
            cin.get();
            cout << "password: ";
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = LOGIN_MSG;
            js["id"] = id;
            js["password"] = pwd;
            string request = js.dump();

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send lgoin msg error" << endl;
            }
            else
            {
                char buf[4096] = {0};
                len = recv(clientfd, buf, 4096, 0);
                if (len == -1)
                {
                    cerr << "recv login response error" << endl;
                }
                else
                {
                    // 反序列化
                    json responsejs = json::parse(buf);
                    if (responsejs["errno"].get<int>() != 0)
                    {
                        // 失败
                        cerr << responsejs["errmsg"] << endl;
                    }
                    else
                    {
                        // 成功
                        // 记录当前用户信息
                        g_currentUser.setId(responsejs["id"].get<int>());
                        g_currentUser.setName(responsejs["name"]);

                        // 记录好友列表
                        if (responsejs.contains("friends"))
                        {
                            // 初始化
                            g_currentUserFriendList.clear();
                            vector<string> vec = responsejs["friends"];
                            for (string &str : vec)
                            {
                                json js = json::parse(str);
                                User user;
                                user.setId(js["id"].get<int>());
                                user.setName(js["name"]);
                                user.setState(js["state"]);
                                g_currentUserFriendList.push_back(user);
                            }
                        }

                        // 记录群组列表
                        if (responsejs.contains("groups"))
                        {
                            // 初始化
                            g_currentUserGroupList.clear();
                            vector<string> vec1 = responsejs["groups"];
                            for (string &str : vec1)
                            {
                                json js = json::parse(str);
                                Group group;
                                group.setId(js["id"].get<int>());
                                group.setName(js["groupname"]);
                                group.setDesc(js["groupdesc"]);

                                vector<string> vec2 = js["users"];
                                for (string &userstr : vec2)
                                {
                                    GroupUser user;

                                    json js = json::parse(userstr);
                                    user.setId(js["id"].get<int>());
                                    user.setName(js["name"]);
                                    user.setState(js["state"]);
                                    user.setRole(js["role"]);
                                    group.getUsers().push_back(user);
                                }
                                g_currentUserGroupList.push_back(group);
                            }
                        }

                        // 显示登录用户的基本信息
                        showCurrentUserData();

                        // 显示用户离线信息
                        if (responsejs.contains("offlinemsg"))
                        {
                            vector<string> vec = responsejs["offlinemsg"];
                            for (string &str : vec)
                            {
                                json js = json::parse(str);
                                if (js["msgid"].get<int>() == ONE_CHAT_MSG)
                                {
                                    cout << js["time"].get<string>() << " [" << js["id"] << "] " << js["name"].get<string>() << " said: " << js["msg"].get<string>() << endl;
                                }
                                else
                                {
                                    cout << "GROUP MESSAGE [" << js["groupid"] << "] : " << js["time"].get<string>() << " [" << js["id"] << "] " << js["name"].get<string>() << " said: " << js["msg"].get<string>() << endl;
                                }
                            }
                        }

                        // 登录成功，启动接收线程接收数据
                        // 该线程之启动一次
                        static int readThreadnum = 0;
                        if (readThreadnum == 0)
                        {
                            std::thread readTask(readTaskHandler, clientfd);
                            readTask.detach();
                            readThreadnum++;
                        }

                        isMainMenuRunning = true;
                        mainMenu(clientfd);
                    }
                }
            }
        }
        break;
        case 2:
        {
            cout << "REGISTER" << endl;
            char name[50] = {0};
            char pwd[50] = {0};

            cout << "username: ";
            cin.getline(name, 50);
            cout << "password: ";
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = REG_MSG;
            js["name"] = name;
            js["password"] = pwd;

            // 序列化
            string request = js.dump();

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send reg msg error" << endl;
            }
            else
            {
                char buf[1024] = {0};
                len = recv(clientfd, buf, 1024, 0);
                if (len == -1)
                {
                    cerr << "recv reg response error" << endl;
                }
                else
                {
                    // 反序列化
                    json responsejs = json::parse(buf);
                    if (responsejs["errno"].get<int>() != 0)
                    {
                        // 注册失败
                        cerr << "name: " << name << " already exist" << endl;
                    }
                    else
                    {
                        // 注册成功
                        cout << "register success! " << "user id is " << responsejs["id"] << endl;
                    }
                }
            }
        }
        break;
        case 3:
        {
            close(clientfd);
            exit(0);
        }
        default:
        {
            cerr << "input invalid" << endl;
            break;
        }
        }
    }

    return 0;
}

void showCurrentUserData()
{

    cout << "====================login user====================" << endl;
    cout << "current login user: => id:" << g_currentUser.getId() << " name " << g_currentUser.getName() << endl;
    cout << "-------------------friend list-------------------" << endl;
    if (!g_currentUserFriendList.empty())
    {
        for (User &user : g_currentUserFriendList)
        {
            cout << user.getId() << " " << user.getName() << " " << user.getState() << endl;
        }
    }
    cout << "-------------------group list-------------------" << endl;
    if (!g_currentUserGroupList.empty())
    {
        for (Group &group : g_currentUserGroupList)
        {
            cout << group.getId() << " " << group.getName() << " " << group.getDesc() << endl;
            for (GroupUser &user : group.getUsers())
            {
                cout << user.getId() << " " << user.getName() << " " << user.getState() << " " << user.getRole() << endl;
            }
        }
    }
    cout << "==================================================" << endl;
}

void help(int, string);
void addfriend(int clientfd, string str);
void chat(int clientfd, string str);
void creategroup(int clientfd, string str);
void addgroup(int clientfd, string str);
void groupchat(int clientfd, string str);
void logout(int, string);

unordered_map<string, string> commandMap = {
    {"help", "查看所有命令，格式 help"},
    {"addfriend", "添加好友，格式 addfriend:friendid"},
    {"chat", "聊天，格式 chat:friendid:message"},
    {"creategroup", "创建群，格式 creategroup:groupname:groupdesc"},
    {"addgroup", "加入群，格式 addgroup:groupid"},
    {"groupchat", "群聊，格式 groupchat:groupid:msg"},
    {"logout", "注销，格式 logout"}};

unordered_map<string, function<void(int, string)>> commandHandlerMap = {
    {"help", help},
    {"addfriend", addfriend},
    {"chat", chat},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"logout", logout}

};

// 接送消息
void readTaskHandler(int clientfd)
{
    for (;;)
    {
        char buf[1024] = {0};
        int len = recv(clientfd, buf, 1024, 0);
        if (len == -1 || len == 0)
        {
            close(clientfd);
            exit(-1);
        }

        // 反序列化
        json js = json::parse(buf);
        if (js["msgid"].get<int>() == ONE_CHAT_MSG)
        {
            cout << js["time"].get<string>() << " [" << js["id"] << "] " << js["name"].get<string>() << " said: " << js["msg"].get<string>() << endl;

            continue;
        }
        if (js["msgid"].get<int>() == GROUP_CHAT_MSG)
        {
            cout << "GROUP MESSAGE [" << js["groupid"] << "] : " << js["time"].get<string>() << " [" << js["id"] << "] " << js["name"].get<string>() << " said: " << js["msg"].get<string>() << endl;

            continue;
        }
    }
}

void mainMenu(int clientfd)
{
    help(0, "");
    char buf[1024] = {0};

    while (isMainMenuRunning)
    {
        cin.getline(buf, 1024);
        string commandBuf(buf);
        string command;

        // 解析命令
        int idx = commandBuf.find(':');
        if (idx == -1)
        {
            // 无':'的命令
            command = commandBuf;
        }
        else
        {
            command = commandBuf.substr(0, idx);
        }
        auto it = commandHandlerMap.find(command);

        if (it == commandHandlerMap.end())
        {
            cerr << "invalid command input" << command << endl;
            continue;
        }
        // 调用相应的事件处理回调
        it->second(clientfd, commandBuf.substr(idx + 1, commandBuf.size() - idx));
    }
}

void help(int, string)
{
    cout << "command list:>>>" << endl;
    for (auto it : commandMap)
    {
        cout << it.first << " : " << it.second << endl;
    }
    cout << endl;
}

void addfriend(int clientfd, string str)
{
    int friendid = atoi(str.c_str());

    // 组装json对象
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;

    // 序列化为json字符串
    string buf = js.dump();

    int len = send(clientfd, buf.c_str(), strlen(buf.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send add friend msg err: " << buf << endl;
    }
}

// str friendid:message
void chat(int clientfd, string str)
{
    int idx = str.find(":");
    if (idx == -1)
    {
        cerr << "invalid chat command" << endl;
        return;
    }
    int toid = atoi(str.substr(0, idx).c_str());
    string msg = str.substr(idx + 1, str.size() - idx);
    cout << "toid: " << toid << ", msg: " << msg << endl;
    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["time"] = getCurrentTime();
    js["to"] = toid;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["msg"] = msg;

    // 序列化为json字符串
    string buf = js.dump();

    int len = send(clientfd, buf.c_str(), strlen(buf.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send chat msg err: " << buf << endl;
    }
}
// str groupname:groupdesc
void creategroup(int clientfd, string str)
{
    cout << "str in creategroup " << str << endl;
    int idx = str.find(":");
    cout << "idx in creategroup " << idx << endl;
    if (idx == -1)
    {
        cerr << "invalid creategroup command" << endl;
        return;
    }

    string groupname = str.substr(0, idx);
    string groupdesc = str.substr(idx + 1, str.size() - idx);
    cout << "groupname: " << groupname << ", groupdesc: " << groupdesc << endl;

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;

    // 序列化为json字符串
    string buf = js.dump();

    int len = send(clientfd, buf.c_str(), strlen(buf.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send creategroup msg err: " << buf << endl;
    }
}

void addgroup(int clientfd, string str)
{
    int groupid = atoi(str.c_str());

    // 组装json对象
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;

    // 序列化为json字符串
    string buf = js.dump();

    int len = send(clientfd, buf.c_str(), strlen(buf.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send add group msg err: " << buf << endl;
    }
}

// str groupid:msg
void groupchat(int clientfd, string str)
{
    int idx = str.find(":");
    if (idx == -1)
    {
        cerr << "invalid groupchat command" << endl;
        return;
    }
    int groupid = atoi(str.substr(0, idx).c_str());
    string msg = str.substr(idx + 1, str.size() - idx);
    cout << "toid: " << groupid << ", msg: " << msg << endl;
    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["time"] = getCurrentTime();
    js["groupid"] = groupid;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["msg"] = msg;

    // 序列化为json字符串
    string buf = js.dump();

    int len = send(clientfd, buf.c_str(), strlen(buf.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send groupchat msg err: " << buf << endl;
    }
}

void logout(int clientfd, string)
{
    // 组装json对象
    json js;
    js["msgid"] = LOGOUT_MSG;
    js["id"] = g_currentUser.getId();

    // 序列化为json字符串
    string buf = js.dump();

    int len = send(clientfd, buf.c_str(), strlen(buf.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send logout msg err: " << buf << endl;
    }
    else
    {
        isMainMenuRunning = false;
    }
}

string getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);

    return std::string(date);
}