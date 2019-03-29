//
// Created by xiaoq on 3/29/19.
//

#include <iostream>
#include <sys/types.h>
#include <pwd.h>
#include <vector>
#include <fstream>
#include <filesystem>
#include "user.h"

using namespace std;
namespace fs = std::filesystem;
vector<user> get_users() {
    vector<user> users;
    while (true) {
        errno = 0; // so we can distinguish errors from no more entries
        passwd* entry = getpwent();
        if (!entry) {
            if (errno) {
                std::cerr << "Error reading password database\n";
                return users;
            }
            break;
        }
        users.push_back(user{uid: entry->pw_uid, name: entry->pw_name});
    }
    endpwent();
    return users;
}

vector<string> get_shells() {
    string path = "/etc/shells";
    ifstream file(path);
    if(!fs::exists(path)) {
        return vector<string>();
    }

    vector<string> shells;
    string line;
    while (getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        shells.push_back(line);
    }
    return shells;
}