//
// Created by xiaoq on 3/26/19.
//

#ifndef LEMOND_JSON_H
#define LEMOND_JSON_H

#include <string>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <iomanip>

using namespace std;

class json_generator {
private:
    ostringstream oss;

    static string escape_json(const string &s) {
        ostringstream o;
        for (char c : s) {
            switch (c) {
                case '"':
                    o << "\\\"";
                    break;
                case '\\':
                    o << "\\\\";
                    break;
                case '\b':
                    o << "\\b";
                    break;
                case '\f':
                    o << "\\f";
                    break;
                case '\n':
                    o << "\\n";
                    break;
                case '\r':
                    o << "\\r";
                    break;
                case '\t':
                    o << "\\t";
                    break;
                default:
                    if ('\x00' <= c && c <= '\x1f') {
                        o << "\\u"
                          << std::hex << std::setw(4) << std::setfill('0') << (int) c;
                    } else {
                        o << c;
                    }
            }
        }
        return o.str();
    }

    string delim;
public:
    json_generator() : delim("") {}

    void start() {
        oss << '{';
        delim = "";
    }

    void end() {
        oss << '}';
        delim = ",";
    }

    void list_start() {
        oss << '[';
        delim = "";
    }

    void list_end() {
        oss << ']';
        delim = ",";
    }

    void list_entry() {
        oss << delim;
        delim = ",";
    }

    void key(const string& key) {
        oss << delim;
        delim = ",";
        oss << '"' << key << "\":";
    }

    void kv(const string& key, long v) {
        oss << delim;
        delim = ",";
        oss << '"' << key << "\":" << v;
    }

    void kv(const string& key, ulong v) {
        oss << delim;
        delim = ",";
        oss << '"' << key << "\":" << v;
    }

    void kv(const string& key, int v) {
        oss << delim;
        delim = ",";
        oss << '"' << key << "\":" << v;
    }

    void kv(const string& key, uint v) {
        oss << delim;
        delim = ",";
        oss << '"' << key << "\":" << v;
    }

    void list_entry(uint v) {
        oss << delim;
        delim = ",";
        oss << v;
    }

    void kv(const string& key, float v) {
        oss << delim;
        delim = ",";
        oss << '"' << key << "\":" << fixed << setprecision(8) << v;
    }

    void kv(const string& key, const string& v) {
        oss << delim;
        delim = ",";
        oss << '"' << key << "\":" << '"' << escape_json(v) << '"';
    }

    void list_entry(const string &v) {
        oss << delim;
        delim = ",";
        oss << '"' << escape_json(v) << '"';
    }

    void raw_value(const string &v) {
        oss << v;
        delim = ",";
    }

    string str() {
        return "{" + oss.str() + "}";
    }

    string raw_str() {
        return oss.str();
    }
};

#endif //LEMOND_JSON_H
