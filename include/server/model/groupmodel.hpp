#ifndef GROUPMODEL_H
#define GROUPMODEL_H
#include "group.hpp"
#include <string>
#include <vector>

class GroupModel
{
public:
    // 创建群组
    bool createGroup(Group &group);

    // 加入群组
    void addGroup(int userId, int groupId, string role);

    // 查询用户所在群组信息
    vector<Group> queryGroup(int userId);

    // 根据GROUPID查询用户ID列表，用于群发消息
    vector<int> queryGroupUser(int userId, int groupId);
};

#endif