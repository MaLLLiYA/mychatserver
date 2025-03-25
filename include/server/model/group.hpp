#ifndef GROUP_H
#define GROUP_H

#include <string>
#include <vector>
using namespace std;
#include "groupuser.hpp"

// Group的ORM类
class Group
{
public:
    Group(int id = -1, string name = "", string desc = "")
        : groupId(id), groupName(name), groupDesc(desc) {}

    void setId(int id)
    {
        groupId = id;
    }
    void setName(string name)
    {
        groupName = name;
    }
    void setDesc(string desc)
    {
        groupDesc = desc;
    }

    int getId()
    {
        return groupId;
    }
    string getName()
    {
        return groupName;
    }
    string getDesc()
    {
        return groupDesc;
    }

    vector<GroupUser>& getUsers()
    {
        return users;
    }

private:
    int groupId;
    string groupName;
    string groupDesc;
    vector<GroupUser> users;
};

#endif