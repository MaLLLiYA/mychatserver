#ifndef OFFMSGMODEL_H
#define OFFMSGMODEL_H

#include <string>
#include <vector>
using namespace std;

class OffMsgModel
{
public:
    // 保存离线消息
    void insert(int userid, string msg);

    // 删除离线消息
    void remove(int userid);

    // 查询离线消息
    vector<string> query(int userid);
};

#endif