//
// Created by xiaoq on 3/29/19.
//

#ifndef LEMOND_USER_H
#define LEMOND_USER_H
#include <string>
#include <vector>
struct user {
    uint uid;
    uint gid;
    std::string name;
};
std::vector<user> get_users();
std::vector<std::string> get_shells();
#endif //LEMOND_USER_H
